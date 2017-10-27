#ifndef TROPHY_MANAGER_DIALOG
#define TROPHY_MANAGER_DIALOG

#include "stdafx.h"
#include "rpcs3/Loader/TROPUSR.h"

#include "Utilities/rXml.h"

#include <QWidget>
#include <QTreeWidget>

struct GameTrophiesData
{
	std::unique_ptr<TROPUSRLoader> trop_usr;
	std::shared_ptr<rXmlNode> trop_config; // I'd like to use unique but the protocol inside of the function passes around shared pointers..
	std::string game_name;
};

class trophy_manager_dialog : public QWidget
{
public:
	explicit trophy_manager_dialog();
private:
	/** Loads a trophy folder. 
	Returns true if successful.  Does not attempt to install if failure occurs, like sceNpTrophy.
	*/
	bool LoadTrophyFolderToDB(const std::string& trop_name, const std::string& game_name);

	/** Fills UI with information.
	Takes results from LoadTrophyFolderToDB and puts it into the UI.
	*/
	void PopulateUI();

	std::vector<std::unique_ptr<GameTrophiesData>> m_trophies_db; //! Holds all the trophy information.
	QTreeWidget* m_trophy_tree; //! UI element to display trophy stuff.
};

#endif
