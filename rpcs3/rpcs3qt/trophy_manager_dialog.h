#ifndef TROPHY_MANAGER_DIALOG
#define TROPHY_MANAGER_DIALOG

#include <QWidget>
#include <QTableWidget>

class trophy_manager_dialog : public QWidget
{
public:
	explicit trophy_manager_dialog();
private:
	void InitTrophyTable();

	QTableWidget* m_trophy_table;
};

#endif
