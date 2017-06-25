#include "tsettingswindow.h"
#include "ui_tsettingswindow.h"
#include "tlog.h"
#include "logcategories.h"

#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
//#include <QtAlgorithms>
#include <QInputDialog>
#include <QMessageBox>

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

    ui->labelDestination->setMouseTracking(true);
    ui->labelDestination->setAttribute(Qt::WA_Hover);
    ui->labelDestination->installEventFilter(this);

    ui->labelSource->setMouseTracking(true);
    ui->labelSource->setAttribute(Qt::WA_Hover);
    ui->labelSource->installEventFilter(this);

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
bool tSettingsWindow::checkAndCorrectArgs(QString &args)
{
    // Поищем символы '{' и '}' и удалим их, если они были введены пользователем
    QChar leftBrake  = '{';
    QChar rightBrake = '}';
    args.remove(leftBrake,  Qt::CaseInsensitive);
    args.remove(rightBrake, Qt::CaseInsensitive);

    //Теперь добавим скобки в начало и конец
    args.push_front(leftBrake);
    args.push_back(rightBrake);

    args = args.toUpper();

    // Проверим нет ли уже таких аргументов
    bool isExist = false;

    isExist = isExist || ( args == ui->labelDestination->text() );
    if(isExist)
        return false;

    isExist = isExist || ( args == ui->labelSource->text() );
    if(isExist)
        return false;

    if( !mLabelArgs.isEmpty() )
    {
        for(auto itLabel = mLabelArgs.cbegin(); itLabel!=mLabelArgs.cend(); ++itLabel)
        {
            isExist = isExist || ( args == (*itLabel)->text() );
            if(isExist)
                return false;
        }
    }

    return true;
}

//******************************************************************************
QString tSettingsWindow::parseArguments()
{
    QString textArgs = ui->lineEditArgumentsOrder->text();

    for(int i=0; i<mLabelArgs.size() && !mLabelArgs.isEmpty(); ++i)
    {
        textArgs.replace(mLabelArgs.at(i)->text(),
                         mArgsDescription.at(i)->toPlainText().replace("\n", " ").trimmed(),
                         Qt::CaseSensitive);
    }

    return textArgs;
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

//******************************************************************************
void tSettingsWindow::on_pushButtonAddArgs_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Создание группы аргументов"),
                                       tr("Введите название:"), QLineEdit::Normal,
                                       QString("ARG"), &ok);
    if (ok && !text.isEmpty())
    {
        if( !checkAndCorrectArgs(text) )
        {
            QMessageBox msgBox;
            msgBox.setText("Введённый аргумент уже существует");
            msgBox.exec();

            return;
        }

    }
    else
        return;

    //Сделаем в окне настроек подвиждет с тремя элементами и двумя layout-ами

    /* |-----------------------------------------------------------------------|
     * | Вертикальный vlayout                                                  |
     * | |-------------------------------------------------------------------| |
     * | | Горизонтальный hlayout                                            | |
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
    labelArg->setText(text);
    labelArg->setAutoFillBackground(true);
    labelArg->setMouseTracking(true);
    labelArg->setAttribute(Qt::WA_Hover);
    labelArg->installEventFilter(this);

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

    connect(argDescription, SIGNAL(textChanged()), this, SLOT(slotOnArgsDescription_Changed()));
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


    // Удалим упоминания имени в строке аргументов при удалении
    QString textLabel = mLabelArgs.at(index)->text();
    QString textOfArgumentsOrder = ui->lineEditArgumentsOrder->text();
    textOfArgumentsOrder.remove(textLabel, Qt::CaseInsensitive);
    ui->lineEditArgumentsOrder->setText(textOfArgumentsOrder);

    mArgsDescription.at(index)->disconnect();

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


void tSettingsWindow::slotOnClickArgsLabel(QLabel *label)
{
    if(!label)
    {
        qWarning() << __PRETTY_FUNCTION__ << "Передан нулевой указатель на label";
        return;
    }

    int cursorPos = ui->lineEditArgumentsOrder->cursorPosition();
    ui->lineEditArgumentsOrder->insert(label->text());

    ui->lineEditArgumentsOrder->setCursorPosition(cursorPos+label->text().length());
}

//******************************************************************************
bool tSettingsWindow::eventFilter(QObject *obj, QEvent *evt)
{
    if( evt->type() == QEvent::KeyPress )
    {
        // Здесь обработаем только события клика по кнопке delete
        int index = mDeleteArgsButtons.indexOf( dynamic_cast< QPushButton* >(obj) );

        if( index != -1 )
        {
            QKeyEvent *keyEvent = dynamic_cast< QKeyEvent* >(evt);
            switch(keyEvent->key())
            {
                case Qt::Key_Enter :
                case Qt::Key_Space :
                    slotOnDeletePushButton( mDeleteArgsButtons.at(index) );
            }
        }
    }
    else if( evt->type() == QEvent::MouseButtonRelease )
    {

        if( mDeleteArgsButtons.indexOf( dynamic_cast< QPushButton* >(obj) ) != -1 )
        {
            // Обработка событий для кнопок delete
            QMouseEvent *mouseEvent = dynamic_cast< QMouseEvent* >(evt);
            if(mouseEvent->button() == Qt::LeftButton)
                slotOnDeletePushButton( static_cast< QPushButton* >(obj) );
        }
        else if( mLabelArgs.indexOf( dynamic_cast< QLabel* >(obj) ) != -1 )
        {
            // Обработка событий для клика по названию аргумента
            QMouseEvent *mouseEvent = dynamic_cast< QMouseEvent* >(evt);
            if(mouseEvent->button() == Qt::LeftButton)
                slotOnClickArgsLabel(static_cast< QLabel* >(obj));
        }
        else if( dynamic_cast< QLabel* >(obj) == ui->labelDestination )
        {
            // Обработка событий для клика по ссылке {DESTINATION}
            QMouseEvent *mouseEvent = dynamic_cast< QMouseEvent* >(evt);
            if(mouseEvent->button() == Qt::LeftButton)
                slotOnClickArgsLabel(static_cast< QLabel* >(obj));
        }
        else if( dynamic_cast< QLabel* >(obj) == ui->labelSource )
        {
            // Обработка событий для клика по ссылке {SOURCE}
            QMouseEvent *mouseEvent = dynamic_cast< QMouseEvent* >(evt);
            if(mouseEvent->button() == Qt::LeftButton)
                slotOnClickArgsLabel(static_cast< QLabel* >(obj));
        }

    }
    else if(evt->type() == QEvent::HoverEnter || evt->type() == QEvent::HoverLeave)
    {
        if( dynamic_cast< QLabel* >(obj) == ui->labelDestination )
        {
            if( evt->type() == QEvent::HoverEnter )
            {
                QFont font = ui->labelDestination->font();
                font.setBold(true);
                ui->labelDestination->setFont(font);
            }
            else if( evt->type() == QEvent::HoverLeave )
            {
                QFont font = ui->labelDestination->font();
                font.setBold(false);
                ui->labelDestination->setFont(font);
            }
        }
        else if( dynamic_cast< QLabel* >(obj) == ui->labelSource )
        {
            if( evt->type() == QEvent::HoverEnter )
            {
                QFont font = ui->labelSource->font();
                font.setBold(true);
                ui->labelSource->setFont(font);
            }
            else if( evt->type() == QEvent::HoverLeave )
            {
                QFont font = ui->labelSource->font();
                font.setBold(false);
                ui->labelSource->setFont(font);
            }
        }
        else
        {
            int index = mLabelArgs.indexOf( dynamic_cast< QLabel* >(obj) );

            if( index != -1 )
            {
                if( evt->type() == QEvent::HoverEnter )
                {
                    QFont font = mLabelArgs.at(index)->font();
                    font.setBold(true);
                    mLabelArgs.at(index)->setFont(font);
                }
                else if( evt->type() == QEvent::HoverLeave )
                {
                    QFont font = mLabelArgs.at(index)->font();
                    font.setBold(false);
                    mLabelArgs.at(index)->setFont(font);
                }
            }
        }
    }
    else
    {
        // pass the event on to the parent class
        return QDialog::eventFilter(obj, evt);
    }

    return false;
}

void tSettingsWindow::on_lineEditArgumentsOrder_textChanged(const QString &/*arg1*/)
{
    ui->lineEditArgumentsOrder->setToolTip( parseArguments() );
}

void tSettingsWindow::slotOnArgsDescription_Changed()
{
    on_lineEditArgumentsOrder_textChanged("");
}
