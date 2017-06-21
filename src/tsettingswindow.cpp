#include "tsettingswindow.h"
#include "ui_tsettingswindow.h"
#include "tlog.h"
#include "logcategories.h"

#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QtAlgorithms>

//******************************************************************************
tSettingsWindow::tSettingsWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::tSettingsWindow)
{
	settWindow = new QSettings("CrocodileSoft", "SyncList");

	ui->setupUi(this);
	this->setLayout(ui->verticalLayout);
	ui->tab->setLayout(ui->verticalLayout_2);
	ui->tab_2->setLayout(ui->verticalLayout_5);

    centralW = new QWidget(this);
    centralW->setLayout(ui->verticalLayoutUserUsage);
    ui->scrollArea->setWidget(centralW);
//    ui->scrollArea->installEventFilter(this);

	connect(this, SIGNAL(signalSettingsChanged()), SLOT(writeWindowSetting()) );
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(writeProgramSetting()) );

	readSettings();
}

//******************************************************************************
tSettingsWindow::~tSettingsWindow()
{
	delete ui;
}

//******************************************************************************
void tSettingsWindow::writeWindowSetting()
{
	settWindow->beginGroup("SettingsWindow");
		settWindow->setValue("Size", size());
		settWindow->setValue("Position",  pos());
	settWindow->endGroup();
}

//******************************************************************************
void tSettingsWindow::writeProgramSetting()
{
	settWindow->beginGroup("SettingsProgram");
		settWindow->setValue("ProgramProcessorPath", ui->pathProgramLineEdit->text());
//		settWindow->setValue("ProgramProcessorArgs", ui->argumentsProgramTextEdit->toPlainText());

	settWindow->endGroup();
}

//******************************************************************************
void tSettingsWindow::readSettings()
{
	settWindow->beginGroup("SettingsWindow");
		resize(settWindow->value("Size",   QSize(480, 370)).toSize());
		move(settWindow->value("Position", QPoint(200, 200)).toPoint());
	settWindow->endGroup();

	settWindow->beginGroup("SettingsProgram");
		ui->pathProgramLineEdit->setText(settWindow->value("ProgramProcessorPath", QString()).toString());
//		ui->argumentsProgramTextEdit->setPlainText(settWindow->value("ProgramProcessorArgs", QString()).toString());

	settWindow->endGroup();
}

//******************************************************************************
void tSettingsWindow::resizeEvent(QResizeEvent *)
{
	emit signalSettingsChanged();
}

//******************************************************************************
void tSettingsWindow::moveEvent(QMoveEvent *)
{
	emit signalSettingsChanged();
}

//******************************************************************************
void tSettingsWindow::on_argumentsProgramTextEdit_textChanged()
{
//	ui->lineArgumentsEdit->setText(ui->argumentsProgramTextEdit->toPlainText().replace("\n", " ").trimmed());
}

//******************************************************************************
void tSettingsWindow::on_findProgramButton_clicked()
{
    QFileDialog dlg( this );
    dlg.setFileMode( QFileDialog::ExistingFile );
    dlg.exec();

    ui->pathProgramLineEdit->setText( dlg.selectedFiles().first() );
    emit signalSettingsChanged();
}

void tSettingsWindow::on_pushButtonAddArgs_clicked()
{

    //Сделаем в окне настроек подвиждет с тремя элементами и двумя layout-ами

    /* |-----------------------------------------------------------------------|
     * | Вертикальный vlayout                                                   |
     * | |-------------------------------------------------------------------| |
     * | | Горизонтальный hlayout                                             | |
     * | | --------------------------------- ------------------------------  | |
     * | | |   Метка                       | |    Кнопка                  |  | |
     * | | |                               | |                            |  | |
     * | | --------------------------------- ------------------------------  | |
     * | |-------------------------------------------------------------------| |
     * |                                                                       |
     * | --------------------------------------------------------------------- |
     * | |                                                                   | |
     * | |   Текстовое поле                                                  | |
     * | |                                                                   | |
     * | |                                                                   | |
     * | --------------------------------------------------------------------- |
     * |-----------------------------------------------------------------------|
     * */

    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();

    // Привяжем каждый создаваемый layout к главному layout
    ui->verticalLayoutUserUsage->addLayout(vlayout);

    // Создадим вспомогательный виджет, разместим в нем layout и добавим в область прокрутки
//    QWidget *centralW = new QWidget();
//    centralW->setLayout(ui->verticalLayoutUserUsage);
//    ui->scrollArea->setWidget(centralW);

    vlayout->addLayout(hlayout);

    QLabel *labelArg = new QLabel(/*hlayout*/);
    labelArg->setText("{wifi}");

    QPushButton *deleteArgButton = new QPushButton(/*hlayout*/);
    deleteArgButton->setText("delete");
    deleteArgButton->installEventFilter(this);

    QPlainTextEdit *argDescription = new QPlainTextEdit(/*vlayout*/);

    hlayout->addWidget(labelArg);
    hlayout->addWidget(deleteArgButton);
    vlayout->addWidget(argDescription);

    // Все объекты созданы и размещены,Сохраним их в векторы
    mLabelArgs.push_back(labelArg);
    mArgsDescription.push_back(argDescription);
    mDeleteArgsButtons.push_back(deleteArgButton);
    mVBoxLayouts.push_back(vlayout);
    mHBoxLayouts.push_back(hlayout);
    mWigets.push_back(centralW);

//    mCurrentDeleteArgsButton = deleteArgButton;
//    connect(this, SIGNAL(signalOnDeletePushButton(QPushButton*)), this, SLOT(slotOnDeletePushButton(deleteArgButton)) ) ;
}

//******************************************************************************
void tSettingsWindow::slotOnDeletePushButton(QPushButton *button)
{
    if(!button)
    {
        qWarning() << __PRETTY_FUNCTION__ << "Передан нулевой указатель на button";
        return;
    }

    int index = mDeleteArgsButtons.indexOf( button );

    if(index == -1)
    {
        qWarning() << __PRETTY_FUNCTION__ << "Переданный указатель не найден в векторе";
        return;
    }



    delete mLabelArgs.at(index);
    mLabelArgs.remove(index);

    delete mArgsDescription.at(index);
    mArgsDescription.remove(index);

    delete mDeleteArgsButtons.at(index);
    mDeleteArgsButtons.remove(index);

    delete mHBoxLayouts.at(index);
    mHBoxLayouts.remove(index);

    delete mVBoxLayouts.at(index);
    mVBoxLayouts.remove(index);

//    delete ui->scrollArea->takeWidget( mWigets.at(index) );
    mWigets.remove(index);

}

//******************************************************************************
bool tSettingsWindow::eventFilter(QObject *obj, QEvent *evt)
{
    if( evt->type() == QEvent::KeyPress || evt->type() == QEvent::MouseButtonRelease )
    {
        qDebug() << "Here1";
        qDebug() << obj;
        int index = mDeleteArgsButtons.indexOf( dynamic_cast< QPushButton* >(obj) );
        qDebug() << index;
    //    if( index != -1 )
    //    {
            if( evt->type() == QEvent::KeyPress )
            {
                qDebug() << "Here2";
                QKeyEvent *keyEvent = dynamic_cast< QKeyEvent* >(evt);
                switch(keyEvent->key())
                {
                    case Qt::Key_Enter :
                    case Qt::Key_Space :
                        slotOnDeletePushButton( mDeleteArgsButtons.at(index) );
                }
            }
            else if( evt->type() == QEvent::MouseButtonRelease )
            {
                qDebug() << "Here3";
                QMouseEvent *mouseEvent = dynamic_cast< QMouseEvent* >(evt);
                if(mouseEvent->button() == Qt::LeftButton)
                    slotOnDeletePushButton( mDeleteArgsButtons.at(index) );

            }
    }
    else
    {
        // pass the event on to the parent class
        return QDialog::eventFilter(obj, evt);
    }

    return false;
}
