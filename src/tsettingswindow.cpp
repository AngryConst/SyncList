#include "tsettingswindow.h"
#include "ui_tsettingswindow.h"
#include "tlog.h"
#include "logcategories.h"

#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>

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
		settWindow->setValue("ProgramProcessorArgs", ui->argumentsProgramTextEdit->toPlainText());

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
		ui->argumentsProgramTextEdit->setPlainText(settWindow->value("ProgramProcessorArgs", QString()).toString());

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
	ui->lineArgumentsEdit->setText(ui->argumentsProgramTextEdit->toPlainText().replace("\n", " ").trimmed());
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
