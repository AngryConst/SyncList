#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTableWidgetItem>
#include <QSplitter>
#include <memory>
#include "tcoresync.h"

#include "tsettingswindow.h"

namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
	//! Сигнал передачи корневых директорий ядру, задания папки - приёмника и запуска обработки в отдельном потоке.
	void signalMainDirs(QStringList &listSrc, QString dst);
	void signalReadXml();				   //!< Сигнал для начала чтения файла с записями.
	void signalSettingsChanged();		   //!< Сигнал, излучающийся при изменении настроек.
	void signalSynchonize(tDiffTable */*table*/);			   //!< Сигнал для запуска синхронизации каталогов.
	void signalShowTable();				   //!< Сигнал для отображения содержимого таблицы
	void signalSaveXml(tDiffTable */*table*/);

	void signalNeedRecord(bool /*record*/); //! Сигнал, включающий или отключающий запись лога.
	void signalClearLog(); //! Очистка журнала и текстового вывода на экран.
	void signalPause(bool p_);
	void signalCancelSync();

public slots:
	/** \brief Добавляет данные в табличный виджет.
	 *
	 * \param table  таблица с результатами сравнения директорий источника и приёмника.
	 * \param eq     количество одинаковых файлов в таблице.
	 * \param newest количество обновленных файлов в таблице.
	 * \param late   количество отсутствующих (или устаревших) файлов.*/
	void slotAddDataToTable(tDiffTable *table, size_t eq, size_t newest, size_t late);

	void slotShowData(); //!< Отображает заполненную таблицу с результатами сравнения.

	void slotShowProgress(int min, int max, int current);

	void slotSyncFinished();

private slots:
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_diffButton_clicked();
    void on_syncButton_clicked();

    void writeSetting();

	void on_equalButton_clicked();
	void on_newestButton_clicked();
	void on_latestButton_clicked();

	void slotCellClicked(int row, int col);
	void slotClearLogTextEdit();

	void on_chooseButton_clicked();

	void slotShowLog(QString &log_);
	void on_recordLogButton_clicked();
	void on_clearLogButton_clicked();

	void slotTableWidgetContextMenu(QPoint pos);
	void slotCopyToClipboard();

	void on_actionShowSettingsWindow_triggered();

	void on_cancelButton_clicked();

	void on_pauseButton_clicked(bool checked);

private:
    void readSettings();
	void resizeEvent(QResizeEvent *);
	void moveEvent(QMoveEvent *);
	void keyPressEvent(QKeyEvent * event);
	bool eventFilter(QObject *, QEvent *evt);

	void makeHeaderTable();
	void makeHeaderPath(unsigned countRow, QTableWidgetItem *item, tDiffTable::iterator iter);
	void makeGreyRow(unsigned countRow, QTableWidgetItem *item);

	void setEnableButtons(bool enable);

//	void synchronization();

    Ui::MainWindow *ui;
	QSplitter *mSplitter;
	QMenu *menu;

    QSettings *sett;
	std::unique_ptr<tDiffTable> mTable;
	size_t countEq;
	size_t countNew;
	size_t countLate;

	tSettingsWindow *commonSettings;
};

#endif // MAINWINDOW_H
