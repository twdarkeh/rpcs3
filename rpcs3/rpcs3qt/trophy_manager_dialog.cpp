#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QTableWidget>

#include "rpcs3/Emu/VFS.h"

#include "trophy_manager_dialog.h"

trophy_manager_dialog::trophy_manager_dialog() : QWidget()
{
	// Buttons
	QCheckBox* butt_lock_trophy = new QCheckBox(tr("Show Locked Trophies"));
	butt_lock_trophy->setCheckable(true);
	butt_lock_trophy->setChecked(true);

	QCheckBox* butt_unlock_trophy = new QCheckBox(tr("Show Unlocked Trophies"));
	butt_unlock_trophy->setCheckable(true);
	butt_unlock_trophy->setChecked(true);

	QGroupBox* settings = new QGroupBox(tr("Trophy View Options"), this);

	// Trophy Table
	m_trophy_table = new QTableWidget(this);

	// LAYOUTS
	QVBoxLayout* settings_layout = new QVBoxLayout(this);
	settings_layout->addWidget(butt_lock_trophy);
	settings_layout->addWidget(butt_unlock_trophy);
	settings_layout->addStretch(1);
	settings->setLayout(settings_layout);

	QHBoxLayout* settings_boxes_layout = new QHBoxLayout(this);
	settings_boxes_layout->addWidget(settings);
	settings_boxes_layout->addStretch();

	QVBoxLayout* all_layout = new QVBoxLayout(this);
	all_layout->addLayout(settings_boxes_layout);
	all_layout->addWidget(m_trophy_table);
	all_layout->addStretch(1); // why doesn't this stretch make everything fine?
	setLayout(all_layout);

	resize(600, 800);
}

void trophy_manager_dialog::InitTrophyTable()
{
	std::string trophyPath = "/dev_hdd0/home/00000001/trophy/NPWR02097_00/"; // + ctxt->trp_name;
	trophyPath = vfs::get(trophyPath);

}
