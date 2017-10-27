#include "trophy_manager_dialog.h"

#include "stdafx.h"

#include "Utilities/Log.h"
#include "Utilities/StrUtil.h"
#include "rpcs3/Emu/VFS.h"
#include "Emu/System.h"

#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QPixmap>
#include <QTableWidget>
#include <QDesktopWidget>
#include <QSlider>
#include <QLabel>

static const int m_TROPHY_ICON_HEIGHT = 75;

namespace
{
	constexpr auto qstr = QString::fromStdString;
}

trophy_manager_dialog::trophy_manager_dialog() : QWidget()
{
	// Nonspecific widget settings
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setWindowTitle(tr("Trophy Manager"));
	setWindowIcon(QIcon(":/rpcs3.ico"));

	// Trophy Tree
	m_trophy_tree = new QTreeWidget();
	m_trophy_tree->setColumnCount(5);

	QStringList column_names;
	column_names << tr("Icon") << tr("Name") << tr("Description") << tr("Type") << tr("Status");
	m_trophy_tree->setHeaderLabels(column_names);
	m_trophy_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	LoadTrophyFolderToDB("NPWR02097_00", "ARKEDO SERIES -2 SWAP"); // arkedo
	PopulateUI();

	// Checkboxes to control dialog
	QCheckBox* check_lock_trophy = new QCheckBox(tr("Show Locked Trophies"));
	check_lock_trophy->setCheckable(true);
	check_lock_trophy->setChecked(true);

	QCheckBox* check_unlock_trophy = new QCheckBox(tr("Show Unlocked Trophies"));
	check_unlock_trophy->setCheckable(true);
	check_unlock_trophy->setChecked(true);

	QCheckBox* check_hidden_trophy = new QCheckBox(tr("Show Hidden Trophies"));
	check_unlock_trophy->setCheckable(true);
	check_unlock_trophy->setChecked(true);

	QLabel* slider_label = new QLabel();
	slider_label->setText(QString("Icon Size: %0").arg(m_TROPHY_ICON_HEIGHT));

	QSlider* icon_slider = new QSlider(Qt::Horizontal);
	icon_slider->setRange(25, 225);
	icon_slider->setValue(m_TROPHY_ICON_HEIGHT);

	// Other button(s)
	QPushButton* butt_close = new QPushButton(tr("Close"));

	// LAYOUTS
	QGroupBox* settings = new QGroupBox(tr("Trophy View Options"));
	settings->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	QVBoxLayout* settings_layout = new QVBoxLayout();
	settings_layout->addWidget(check_lock_trophy);
	settings_layout->addWidget(check_unlock_trophy);
	settings_layout->addWidget(check_hidden_trophy);
	QHBoxLayout* slider_layout = new QHBoxLayout();
	slider_layout->addWidget(slider_label);
	slider_layout->addWidget(icon_slider);
	settings_layout->addLayout(slider_layout);
	settings_layout->addStretch(0);
	settings->setLayout(settings_layout);

	QHBoxLayout* settings_boxes_layout = new QHBoxLayout();
	settings_boxes_layout->addWidget(settings);
	settings_boxes_layout->addStretch();

	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addStretch();
	buttons_layout->addWidget(butt_close);

	QVBoxLayout* all_layout = new QVBoxLayout(this);
	all_layout->addLayout(settings_boxes_layout);
	all_layout->addWidget(m_trophy_tree);
	all_layout->addLayout(buttons_layout);
	setLayout(all_layout);

	QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
	resize(defaultSize);

	// Make connects
	connect(butt_close, &QAbstractButton::pressed, [this]() {
		close();
	});
}

bool trophy_manager_dialog::LoadTrophyFolderToDB(const std::string& trop_name, const std::string& game_name)
{
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/" + trop_name;

	// HACK: dev_hdd0 must be mounted for vfs to work so I can load trophies. Consider moving mounting to init and load for redundancy w/o hack?
	const std::string emu_dir_ = g_cfg.vfs.emulator_dir;
	const std::string emu_dir = emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
	vfs::mount("dev_hdd0", fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir));

	// Populate GameTrophiesData
	std::unique_ptr<GameTrophiesData> game_trophy_data = std::make_unique<GameTrophiesData>();

	game_trophy_data->game_name = game_name;
	game_trophy_data->path = vfs::get(trophyPath + "/");
	game_trophy_data->trop_usr.reset(new TROPUSRLoader());
	std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	bool success = game_trophy_data->trop_usr->Load(trophyUsrPath, trophyConfPath);

	fs::file config(vfs::get(trophyConfPath));

	if (!success || !config)
	{
		LOG_ERROR(GENERAL, "Failed to load trophy database for %s", trop_name);
		return false;
	}

	for (int trophy_id = 0; trophy_id < game_trophy_data->trop_usr->GetTrophiesCount(); ++trophy_id)
	{
		// Figure out how many zeros are needed for padding.  (either 0, 1, or 2)
		QString padding = "";
		if (trophy_id < 10)
		{
			padding = "00";
		}
		else if (trophy_id < 100)
		{
			padding = "0";
		}

		QPixmap trophy_icon;
		QString path = qstr(game_trophy_data->path) + "TROP" + padding + QString::number(trophy_id) + ".PNG";
		if (!trophy_icon.load(path))
		{
			LOG_ERROR(GENERAL, "Failed to load trophy icon for trophy %n %s", trophy_id, game_trophy_data->path);
		}
		game_trophy_data->trophy_images.emplace_back(std::move(trophy_icon));
	}

	game_trophy_data->trop_config.Read(config.to_string());
	m_trophies_db.push_back(std::move(game_trophy_data));
	config.release();
	return true;
}

void trophy_manager_dialog::PopulateUI()
{
	for (auto& data : m_trophies_db)
	{
		std::shared_ptr<rXmlNode> trophy_base = data->trop_config.GetRoot();
		if (trophy_base->GetChildren()->GetName() == "trophyconf")
		{
			trophy_base = trophy_base->GetChildren();
		}
		else
		{
			LOG_ERROR(GENERAL, "Root name does not match trophyconf in trophy. Name received: %s", trophy_base->GetChildren()->GetName());
			continue;
		}

		QTreeWidgetItem* game_root = new QTreeWidgetItem(m_trophy_tree);
		game_root->setText(0, qstr(data->game_name));
		game_root->setExpanded(true);
		m_trophy_tree->addTopLevelItem(game_root);

		for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
		{		
			// Only show trophies.
			if (n->GetName() != "trophy")
			{
				continue;
			}

			s32 trophy_id = atoi(n->GetAttribute("id").c_str());

			// Don't show hidden trophies
			bool hidden = n->GetAttribute("hidden")[0] == 'y' && !data->trop_usr->GetTrophyUnlockState(trophy_id);

			// Get data (stolen graciously from sceNpTrophy.cpp)
			SceNpTrophyDetails details;
			details.trophyId = trophy_id;
			QString trophy_type = "";
			switch (n->GetAttribute("ttype")[0]) {
			case 'B': details.trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   trophy_type = "Bronze";   break;
			case 'S': details.trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   trophy_type = "Silver";   break;
			case 'G': details.trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     trophy_type = "Gold";     break;
			case 'P': details.trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; trophy_type = "Platinum"; break;
			}

			switch (n->GetAttribute("hidden")[0]) {
			case 'y': details.hidden = true;  break;
			case 'n': details.hidden = false; break;
			}

			for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext()) 
			{
				if (n2->GetName() == "name")
				{
					std::string name = n2->GetNodeContent();
					memcpy(details.name, name.c_str(), std::min((size_t)SCE_NP_TROPHY_NAME_MAX_SIZE, name.length() + 1));
				}
				if (n2->GetName() == "detail")
				{
					std::string detail = n2->GetNodeContent();
					memcpy(details.description, detail.c_str(), std::min((size_t)SCE_NP_TROPHY_DESCR_MAX_SIZE, detail.length() + 1));
				}
			}

			QTreeWidgetItem* trophy_item = new QTreeWidgetItem(game_root);
			trophy_item->setData(0, Qt::DecorationRole, data->trophy_images[trophy_id].scaledToHeight(m_TROPHY_ICON_HEIGHT));
			trophy_item->setSizeHint(0, QSize(-1, m_TROPHY_ICON_HEIGHT));
			trophy_item->setText(1, qstr(details.name));
			trophy_item->setText(2, qstr(details.description));
			trophy_item->setText(3, trophy_type);
			trophy_item->setText(4, data->trop_usr->GetTrophyUnlockState(trophy_id) ? "Unlocked" : "Locked");
			trophy_item->setData(5, Qt::UserRole, trophy_id);
			trophy_item->setHidden(hidden);

			game_root->addChild(trophy_item);
		}
	}
}
