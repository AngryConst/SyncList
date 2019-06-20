#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QFileDialog>
#include <QCheckBox>
#include <QStringList>
#include <QElapsedTimer>
#include <QDebug>
#include <QMouseEvent>
#include <QClipboard>
#include "tlog.h"
#include "logcategories.h"

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif


//******************************************************************************
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), countEq(0), countNew(0), countLate(0),
    commonSettings(nullptr)
{
	mSplitter = new QSplitter(Qt::Vertical, this);

    ui->setupUi(this);
    ui->centralWidget->setLayout(ui->verticalLayout);

    sett = new QSettings("CrocodileSoft", "SyncList");

	ui->tableOfDifference->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(signalSettingsChanged()),  this,				SLOT(writeSetting()));
	connect(this, SIGNAL(signalShowTable()),		this,				SLOT(slotShowData()) );
	connect(ui->tableOfDifference, SIGNAL(cellClicked(int,int)), this,  SLOT(slotCellClicked(int,int)));
	connect(this, SIGNAL(signalClearLog()),			this,				SLOT(slotClearLogTextEdit()) );

	connect(this, SIGNAL(signalNeedRecord(bool)),  &tLog::instance(),	SLOT(slotNeedRecord(bool)));
	connect(this, SIGNAL(signalClearLog()),		   &tLog::instance(),	SLOT(slotClearLog()) );

	connect(ui->dbgButton, SIGNAL(clicked()),	    ui->logText,		SLOT(clear()) );
	connect(ui->dbgButton, SIGNAL(clicked(bool)),  &tLog::instance(),	SLOT(slotShowDebug(bool)) );

	connect(ui->infoButton, SIGNAL(clicked()),		ui->logText,		SLOT(clear()) );
	connect(ui->infoButton, SIGNAL(clicked(bool)), &tLog::instance(),	SLOT(slotShowInfo(bool)) );

	connect(ui->critButton, SIGNAL(clicked()),		ui->logText,		SLOT(clear()) );
	connect(ui->critButton, SIGNAL(clicked(bool)), &tLog::instance(),	SLOT(slotShowCritical(bool)) );

	connect(ui->warnButton, SIGNAL(clicked()),		ui->logText,		SLOT(clear()) );
	connect(ui->warnButton, SIGNAL(clicked(bool)), &tLog::instance(),	SLOT(slotShowWarning(bool)) );

	connect(&tLog::instance(),  SIGNAL(signalLogChanged(QString&)),this,SLOT(slotShowLog(QString&)));

	connect(this->mSplitter, SIGNAL(splitterMoved(int,int)),this, SIGNAL(signalSettingsChanged()) );

	connect(ui->tableOfDifference, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotTableWidgetContextMenu(QPoint)) );
	connect(ui->pauseButton,  SIGNAL(clicked(bool)), this, SIGNAL(signalPause(bool)) );
	connect(ui->cancelButton, SIGNAL(clicked()),	 this, SIGNAL(signalCancelSync()) );

	ui->tableOfDifference->setColumnCount(8);
	ui->tableOfDifference->setEditTriggers( QAbstractItemView::NoEditTriggers );
	makeHeaderTable();

	ui->logWidget->setLayout(ui->logLayout);


	ui->verticalLayout->addWidget(mSplitter);

	mSplitter->addWidget(ui->tableOfDifference);
	mSplitter->addWidget(ui->logWidget);

//	ui->logText->installEventFilter(this); //FIXME с этой строчкой не происходит ресайз QTextEdit logText вместе с изменением размера формы
ui->tableOfDifference->installEventFilter(this);
	readSettings();

	emit signalNeedRecord( ui->recordLogButton->isChecked() );

    // Создадим окно настроек, но не будем показывать его
    if( !commonSettings )
        commonSettings = new tSettingsWindow(this);


#ifdef Q_OS_WIN
    mTaskButton = new QWinTaskbarButton(this);
    mTaskButton->setWindow(this->windowHandle());

    mTaskProgress = mTaskButton->progress();
    mTaskProgress->setVisible(true);
#endif
}

//******************************************************************************
MainWindow::~MainWindow()
{
	delete ui;
}

//******************************************************************************
void MainWindow::makeHeaderTable()
{
	QStringList headerOfTable;
	headerOfTable << ""
				  << "Источник" << "Дата изменения" << "Размер"
				  << "<==>"
				  << "Приёмник" << "Дата изменения" << "Размер";
	ui->tableOfDifference->setHorizontalHeaderLabels(headerOfTable);
}

//******************************************************************************
void MainWindow::slotAddDataToTable(tDiffTable *table, size_t eq, size_t newest, size_t late)
{
	if(table==nullptr)
	{
		qWarning() << "MainWindow::" << __FUNCTION__ << "Передан нулевой указатель";
		return;
	}

	mTable.reset( table );
    ui->progressBar->setRange(0,100);
	ui->progressBar->setValue(100);
#ifdef Q_OS_WIN
    mTaskProgress->setRange(0,100);
    mTaskProgress->setValue(100);
#endif

	countEq = eq;
	countNew = newest;
	countLate = late;
	slotShowData();
}

//******************************************************************************
void MainWindow::makeHeaderPath(unsigned countRow, QTableWidgetItem *item, tDiffTable::iterator iter)
{
	item = new QTableWidgetItem();
	item->setBackgroundColor(QColor::fromRgb(220,220,220)); // Серый цвет
	item->setCheckState(Qt::Checked);
	ui->tableOfDifference->setItem(countRow, 0, item);

	item = new QTableWidgetItem();
	item->setData(Qt::DisplayRole, iter.value()[0].source.absolutePath());
	item->setBackgroundColor(QColor::fromRgb(220,220,220)); // Серый цвет
	ui->tableOfDifference->setItem(countRow, 1, item);

	item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, iter.value()[0].destination.relatePath());
	item->setBackgroundColor(QColor::fromRgb(220,220,220)); // Серый цвет
	ui->tableOfDifference->setItem(countRow, 5, item);
}

//******************************************************************************
void MainWindow::makeGreyRow(unsigned countRow, QTableWidgetItem *item)
{
	for(int c = 0; c < ui->tableOfDifference->columnCount(); ++c )
	{
		item = new QTableWidgetItem();
		item->setBackgroundColor(QColor::fromRgb(220,220,220)); // Серый цвет
		ui->tableOfDifference->setItem(countRow, c, item);
	}
}

//******************************************************************************
void MainWindow::slotShowData()
{
	ui->tableOfDifference->clear();
	ui->tableOfDifference->setRowCount( 0 );
	makeHeaderTable();

	// Пропускаем цикл, если ни одна кнопка не выбрана
	if(!ui->equalButton->isChecked() &&
	   !ui->newestButton->isChecked() &&
	   !ui->latestButton->isChecked()	  )
		return;

	size_t tableSize=0;

	// Меняем количество строк в таблице в зависимости от выбранных настроек
	if(ui->equalButton->isChecked() )
		tableSize += countEq;

	if(ui->newestButton->isChecked() )
		tableSize += countNew;

	if(ui->latestButton->isChecked() )
		tableSize += countLate;

	if(tableSize == 0)
	{
		ui->tableOfDifference->clear();
		ui->tableOfDifference->setRowCount( tableSize );
		makeHeaderTable();
		return;
	}

	// Здесь mTable.size() - это количество уникальных папок
	ui->tableOfDifference->setRowCount( tableSize + mTable->size() );

    ui->progressBar->setRange(0, mTable->size() - 1);
#ifdef Q_OS_WIN
    mTaskProgress->setRange(0 ,mTable->size() - 1);
#endif

	QTableWidgetItem *item = nullptr;
	unsigned countRow=0, countCol=0;
	size_t realCountDir=0, progressValue=0;
QElapsedTimer timer;
timer.start();
	// Цикл по ключам словаря(Относительный путь)
	for(tDiffTable::iterator currentDir=mTable->begin(); currentDir != mTable->end(); ++currentDir)
	{
        ui->progressBar->setValue( progressValue );
#ifdef Q_OS_WIN
        mTaskProgress->setValue( progressValue );
#endif
        ++progressValue;
		// Для проверки первый ли это заход в цикл
		bool isFirstLoop = true;

		// Цикл по значениям словаря(Вектор с именами файлов)
		for(QList<tDiffItem>::iterator currentFile =  currentDir.value().begin();
									   currentFile != currentDir.value().end();
									   ++currentFile)
		{
			// Пропускаем элемент, если кнопка не выбрана
			if(!ui->equalButton->isChecked()  && currentFile->direction == Equal)
				continue;
			if(!ui->newestButton->isChecked() && currentFile->direction == Newest)
				continue;
			if(!ui->latestButton->isChecked() && currentFile->direction == Latest)
				continue;

			// Если зашли в цикл первый раз, то создадим заголовок
			if(isFirstLoop)
			{
				++realCountDir;
				makeGreyRow(countRow, item);
				makeHeaderPath(countRow++, item, currentDir);
				isFirstLoop = false;
			}

			// В первый столбец поместим checkBox и указатель на элемент таблицы
			QVariant var;
			var.setValue<tDiffItemPtr>(&(*currentFile));
			item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, var);
			item->setCheckState(Qt::Checked);
			currentFile->sync = true;
			ui->tableOfDifference->setItem(countRow, countCol++, item);

			item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, "  " + currentFile->source.name);
			ui->tableOfDifference->setItem(countRow, countCol++, item);

			item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, currentFile->source.dateTime.toString("yyyy.MM.dd-hh:mm:ss.zzz"));
			ui->tableOfDifference->setItem(countRow, countCol++, item);

			if( !currentFile->source.name.isEmpty() )
			{
				item = new QTableWidgetItem();
				item->setData(Qt::DisplayRole, currentFile->source.size);
				item->setTextAlignment(Qt::AlignRight);
				ui->tableOfDifference->setItem(countRow, countCol, item);
			}

			countCol++;

			item = new QTableWidgetItem();
			QString s;
			switch (currentFile->direction)
			{
				case Equal:
					s = EqualStr;
					break;
				case Newest:
					s = NewestStr;
					break;
				case Latest:
					s = LatestStr;
					break;
				default:
					break;
			}
			item->setData(Qt::DisplayRole, s);
			item->setTextAlignment(Qt::AlignCenter);
			ui->tableOfDifference->setItem(countRow, countCol++, item);

			item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, "  " + currentFile->destination.name);
			ui->tableOfDifference->setItem(countRow, countCol++, item);

			item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, currentFile->destination.dateTime.toString("yyyy.MM.dd-hh:mm:ss.zzz"));
			ui->tableOfDifference->setItem(countRow, countCol++, item);

			if( !currentFile->destination.name.isEmpty() )
			{
				item = new QTableWidgetItem();
				item->setData(Qt::DisplayRole, currentFile->destination.size);
				item->setTextAlignment(Qt::AlignRight);
				ui->tableOfDifference->setItem(countRow, countCol++, item);
			}

			countCol = 0;
			++countRow;
		}
	}

	// Отрезаем лишние строчки, выделенные ранее
	ui->tableOfDifference->setRowCount(
				ui->tableOfDifference->rowCount() - (mTable->size() - realCountDir) );

	qDebug(logDebug()) << "Сортировка и вывод в таблицу на экран:" << timer.elapsed() << "ms";
}

//******************************************************************************
void MainWindow::slotShowProgress(int min, int max, int current)
{
    ui->progressBar->setRange(min,max);
	ui->progressBar->setValue( current );
#ifdef Q_OS_WIN
    mTaskProgress->setRange(min,max);
    mTaskProgress->reset();
    mTaskProgress->setValue( current );
    mTaskProgress->setVisible(true);
    mTaskProgress->show();
#endif
}

//******************************************************************************
void MainWindow::slotSyncFinished()
{
	setEnableButtons(true);
}

//******************************************************************************
void MainWindow::on_addButton_clicked()
{
    QFileDialog dlg( this );
    dlg.setFileMode( QFileDialog::Directory );
    dlg.exec();

    ui->sourceListCmbBox->addItem( dlg.directory().absolutePath() );
    emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::on_deleteButton_clicked()
{
    ui->sourceListCmbBox->removeItem( ui->sourceListCmbBox->currentIndex() );
    emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::on_diffButton_clicked()
{
	int count = ui->sourceListCmbBox->count();

    QStringList listOfSourceDir;
    for(int index=0; index<count; ++index)
        listOfSourceDir << ui->sourceListCmbBox->itemText( index );

	ui->tableOfDifference->clear();
	makeHeaderTable();

    ui->progressBar->setRange(0,0);
	ui->progressBar->setValue(0);
#ifdef Q_OS_WIN
    mTaskProgress->setRange(0,0);
    mTaskProgress->setValue( 0 );
#endif

	emit signalMainDirs(listOfSourceDir, ui->dstDirEdit->text());
}

//******************************************************************************
void MainWindow::setEnableButtons(bool enable)
{
	ui->addButton->setEnabled(enable);
	ui->deleteButton->setEnabled(enable);
	ui->chooseButton->setEnabled(enable);
	ui->sourceListCmbBox->setEnabled(enable);
	ui->dstDirEdit->setEnabled(enable);
	ui->diffButton->setEnabled(enable);
	ui->syncButton->setEnabled(enable);
}

//******************************************************************************
void MainWindow::on_syncButton_clicked()
{
	emit signalSynchonize( mTable.get() );

	setEnableButtons(false);
}

//******************************************************************************
void MainWindow::readSettings()
{

    sett->beginGroup("MainWindow");
		resize(sett->value("Size",   QSize(480, 370)).toSize());
        move(sett->value("Position", QPoint(200, 200)).toPoint());
    sett->endGroup();

	sett->beginGroup("CheckButtons");
		ui->equalButton->setChecked( sett->value("Equal",  false).toBool() );
		ui->newestButton->setChecked(sett->value("Newest", false).toBool() );
		ui->latestButton->setChecked(sett->value("Latest", false).toBool() );
	sett->endGroup();

    int size = sett->beginReadArray("MainDirs");
    QStringList listOfDirs;
        for (int i = 0; i < size; ++i)
        {
            sett->setArrayIndex(i);
            listOfDirs << sett->value("Directory", QStringList()).toString();
        }
    sett->endArray();
    ui->sourceListCmbBox->insertItems(0, listOfDirs);

	ui->dstDirEdit->setText(sett->value("DestinationDirectory", "").toString());

	sett->beginGroup("LogGroup");
		ui->recordLogButton->setChecked(sett->value("RecordLogButton",  true).toBool());

		ui->dbgButton->setChecked(sett->value( "DbgLogButton",		true).toBool());
		ui->infoButton->setChecked(sett->value("InfoLogButton",		true).toBool());
		ui->warnButton->setChecked(sett->value("WarningLogButton",  true).toBool());
		ui->critButton->setChecked(sett->value("CriticalLogButton", true).toBool());

		mSplitter->restoreState(sett->value("SizeSplitWidget").toByteArray());
	sett->endGroup();
}

//******************************************************************************
void MainWindow::writeSetting()
{
    sett->beginGroup("MainWindow");
         sett->setValue("Size", size());
         sett->setValue("Position",  pos());
    sett->endGroup();

	sett->beginGroup("CheckButtons");
		sett->setValue("Equal",  ui->equalButton->isChecked());
		sett->setValue("Newest", ui->newestButton->isChecked());
		sett->setValue("Latest", ui->latestButton->isChecked());
	sett->endGroup();

    sett->beginWriteArray("MainDirs");
        for (int i = 0; i < ui->sourceListCmbBox->count(); ++i)
        {
            sett->setArrayIndex(i);
            sett->setValue("Directory", ui->sourceListCmbBox->itemText(i));
        }
    sett->endArray();

	sett->setValue("DestinationDirectory",  ui->dstDirEdit->text());

	sett->beginGroup("LogGroup");
		sett->setValue("RecordLogButton",   ui->recordLogButton->isChecked());

		sett->setValue("DbgLogButton",		ui->dbgButton->isChecked());
		sett->setValue("InfoLogButton",		ui->infoButton->isChecked());
		sett->setValue("WarningLogButton",	ui->warnButton->isChecked());
		sett->setValue("CriticalLogButton", ui->critButton->isChecked());

		sett->setValue("SizeSplitWidget", mSplitter->saveState());
	sett->endGroup();
}

//******************************************************************************
void MainWindow::resizeEvent(QResizeEvent */*re*/)
{
    emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::moveEvent(QMoveEvent */*me*/)
{
	emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::keyPressEvent(QKeyEvent *event)
{
//	qDebug() << "Key pressed";
//	if(event)
}

//******************************************************************************
bool MainWindow::eventFilter(QObject *obj, QEvent *evt)
{
	/*if (obj == ui->logText)
	{
		if( evt->type() == QEvent::Resize)
		{//qDebug() << "resize()";
			emit signalSettingsChanged();
			return true;
		}
		else
		{
			return false;
		}
	}
	else if( obj == ui->tableOfDifference->viewport() )
	{
		if( evt->type() == QEvent::MouseButtonPress )
		{qDebug(logDebug()) << "eventFilter";
			QMouseEvent *mouseEvent = dynamic_cast< QMouseEvent* >(evt);
			if( mouseEvent->buttons() & Qt::RightButton)
				menu->exec(QCursor::pos());
		}
	}
	else*/ if( obj == ui->tableOfDifference )
	{
		if( evt->type() == QEvent::KeyPress )
		{
			QKeyEvent *keyEvent = dynamic_cast< QKeyEvent* >(evt);
			switch(keyEvent->key())
			{
				case Qt::Key_C :
					if(keyEvent->modifiers() & Qt::CTRL) { slotCopyToClipboard(); }
			}
		}
	}
	else
	{
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, evt);
	}

    return false;
}

void MainWindow::showEvent(QShowEvent *event)
{
    mTaskButton->setWindow(this->windowHandle());
    event->accept();
}

//******************************************************************************
void MainWindow::on_equalButton_clicked()
{
	emit signalShowTable();
	emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::on_newestButton_clicked()
{
	emit signalShowTable();
	emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::on_latestButton_clicked()
{
	emit signalShowTable();
	emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::slotCellClicked(int row, int col)
{
//    qDebug() << "Clicked. Row:" << row << "Column:" << col;
	if(col!=0)
		return;

	// Ставим или убираем галочки на всех файлах внутри одной папки в таблице
	QTableWidgetItem *pathItem = ui->tableOfDifference->item(row,col);
	if( pathItem != nullptr)
    {
        const Qt::CheckState &checkState = pathItem->checkState();

		// Если серый цвет, значит элемент таблицы - это путь до папки с файлами
		if(pathItem->backgroundColor() == QColor::fromRgb(220,220,220) )
        {

			for(int i=row+1; i<ui->tableOfDifference->rowCount(); ++i)
			{
				QTableWidgetItem *fileItem = ui->tableOfDifference->item(i,col);

				if( fileItem == nullptr )
					return;

				if(fileItem->backgroundColor() == QColor::fromRgb(220,220,220))
					break;

				tDiffItemPtr itemOfTable = (fileItem->data(Qt::DisplayRole).value<tDiffItemPtr>());

				// Ставим или убираем признак синхронизации.
				itemOfTable->sync = (checkState == Qt::Checked) ? true : false;
				fileItem->setCheckState( checkState );
			}
		}

        // Получим список выделенных
        const QList< QTableWidgetItem* > &selectedItems = ui->tableOfDifference->selectedItems();

        if( !selectedItems.isEmpty() )
        {
            for(auto item : selectedItems)
            {
                // По текущей строке элемента выберем нулевой столбец вместо текущего
                // Чтобы получить доступ к указателю на элемент в памяти
                QTableWidgetItem *zeroColItem = ui->tableOfDifference->item( item->row(),0 );

                zeroColItem->setCheckState( checkState );
                tDiffItemPtr itemOfTable = (zeroColItem->data(Qt::DisplayRole).value<tDiffItemPtr>());

                // Ставим или убираем признак синхронизации.
                if( itemOfTable != nullptr )
                    itemOfTable->sync = (checkState == Qt::Checked) ? true : false;
            }
        }

	}

}

//******************************************************************************
void MainWindow::slotClearLogTextEdit()
{
	ui->logText->clear();
}

//******************************************************************************
void MainWindow::on_chooseButton_clicked()
{
	QFileDialog dlg( this );
	dlg.setFileMode( QFileDialog::Directory );
	dlg.exec();

	ui->dstDirEdit->setText( dlg.directory().absolutePath() );
	emit signalSettingsChanged();
}

//******************************************************************************
void MainWindow::slotShowLog(QString &log_)
{
	ui->logText->append( log_ );
}

//******************************************************************************
void MainWindow::on_recordLogButton_clicked()
{
	emit signalSettingsChanged();
	emit signalNeedRecord( ui->recordLogButton->isChecked() );
}

//******************************************************************************
void MainWindow::on_clearLogButton_clicked()
{
	emit signalClearLog();
}

//******************************************************************************
void MainWindow::slotTableWidgetContextMenu(QPoint pos)
{
	menu = new QMenu(this); // Создаём контекстное меню

	// Создаём действие
	QAction  *copyCells = new QAction("Копировать, Ctrl+C", this); // FIXME Добавить иконку.

	// Устанавливаем действия в меню
	menu->addAction(copyCells);

	//Вызываем меню
	menu->popup(ui->tableOfDifference->viewport()->mapToGlobal(pos));

	// Подключаем обработчик
	connect(copyCells, SIGNAL(triggered()), this, SLOT(slotCopyToClipboard()) );
}

//******************************************************************************
void MainWindow::slotCopyToClipboard()
{
	QList< QTableWidgetItem* > selectedItems = ui->tableOfDifference->selectedItems();

	if(selectedItems.isEmpty())
		return;

	QString copiedText = QString();

	int lastRow = selectedItems.first()->row();

	foreach(QTableWidgetItem  *item, selectedItems)
	{
		if(item->row() != lastRow)
			copiedText.append("\n");

		copiedText.append(item->text());
		copiedText.append(" ");

		lastRow = item->row();
	}

	qDebug(logDebug()) << "Выделенные ячейки скопированы в буфер обмена.";
	QApplication::clipboard()->setText(copiedText);
}

//******************************************************************************
void MainWindow::on_actionShowSettingsWindow_triggered()
{
	// Если вызвали настройки в меню, то покажем окно
    if( !commonSettings )
        commonSettings = new tSettingsWindow(this);

	commonSettings->show();
}

//******************************************************************************
void MainWindow::on_cancelButton_clicked()
{
	setEnableButtons(true);

    ui->progressBar->setRange(0,100);
	ui->progressBar->setValue(0);
#ifdef Q_OS_WIN
    mTaskProgress->setRange(0,100);
    mTaskProgress->setValue( 0 );
#endif

	ui->pauseButton->setChecked(false);
	qDebug(logInfo()) << "Синхронизация прервана пользователем";
}

//******************************************************************************
void MainWindow::on_pauseButton_clicked(bool checked)
{
	if(checked)
		qDebug(logInfo()) << "Синхронизация приостановлена пользователем";
	else
        qDebug(logInfo()) << "Синхронизация продолжена";
}

//******************************************************************************]
const tSettingsWindow *MainWindow::getSettings()
{
    return commonSettings;
}
