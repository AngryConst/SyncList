#ifndef TSETTINGSWINDOW_H
#define TSETTINGSWINDOW_H

#include <QDialog>
#include <QSettings>
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
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

    QString getFullProgramPath();
    QString getParsedArgs();
    QString getExitCodes();

signals:
	void signalSettingsChanged();		   //!< Сигнал, излучающийся при изменении настроек.
    void signalExitCodesChanged(const QString &codes);

private slots:
	void writeWindowSetting();
	void writeProgramSetting();
    void slotClearDiffFile();

    void on_findProgramButton_clicked();

    void on_pushButtonAddArgs_clicked();

    void slotOnDeletePushButton(QPushButton *button);

    void slotOnClickArgsLabel(QLabel *label);

    void on_lineEditArgumentsOrder_textChanged(const QString &);

    void slotOnArgsDescription_Changed();
//    void slotExitCodeChanged(const QString &codes);

private:
	void readSettings();

	void resizeEvent(QResizeEvent *);
    void moveEvent(QMoveEvent *);

    bool checkAndCorrectArgs(QString &args);
    void createDynamicInterface(QString nameArg, QString descArg);

    QString parseArguments();

	Ui::tSettingsWindow *ui;

	QSettings *settWindow;

    QWidget *centralW;

    QVector< QLabel* > mLabelArgs;
    QVector< QPlainTextEdit* > mArgsDescription;
    QVector< QPushButton* > mDeleteArgsButtons;
    QVector< QVBoxLayout* > mVBoxLayouts;
    QVector< QHBoxLayout* > mHBoxLayouts;
    QVector< QWidget*     > mWigets;

    bool eventFilter(QObject *obj, QEvent *evt);
};

#endif // TSETTINGSWINDOW_H
