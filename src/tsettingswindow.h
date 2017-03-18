#ifndef TSETTINGSWINDOW_H
#define TSETTINGSWINDOW_H

#include <QDialog>
#include <QSettings>
#include "tcoresync.h"

namespace Ui {
class tSettingsWindow;
}

class tSettingsWindow : public QDialog
{
	Q_OBJECT

public:
	explicit tSettingsWindow(QWidget *parent = 0);
	~tSettingsWindow();

signals:
	void signalSettingsChanged();		   //!< Сигнал, излучающийся при изменении настроек.

private slots:
	void writeWindowSetting();
	void writeProgramSetting();

	void on_argumentsProgramTextEdit_textChanged();

    void on_findProgramButton_clicked();

private:
	void readSettings();

	void resizeEvent(QResizeEvent *);
	void moveEvent(QMoveEvent *);

	Ui::tSettingsWindow *ui;

	QSettings *settWindow;
};

#endif // TSETTINGSWINDOW_H
