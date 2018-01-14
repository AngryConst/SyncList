#include "mainwindow.h"
#include "tcoresync.h"
#include "tlog.h"
#include <QApplication>
#include <QObject>
#include <iostream>

//! Перехватчик сообщений для записи в лог.
static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    std::cerr << context.category <<  msg.toLocal8Bit().toStdString().c_str();
	tLog::instance().append(type, context, msg);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qInstallMessageHandler( messageHandler );

    tCoreSync mCore;
    MainWindow w;

	qRegisterMetaType< tDiffTable >("tDiffTable&");
	qRegisterMetaType< size_t >(	"size_t");
	qRegisterMetaType< QString >(	"QString&");

	QObject::connect( &w,	  SIGNAL(signalReadXml()),				 &mCore, SLOT(slotReadList()) );
	QObject::connect( &w,	  SIGNAL(signalSynchonize(tDiffTable*)), &mCore, SLOT(synchronization(tDiffTable*)) );
	QObject::connect( &w,	  SIGNAL(signalSaveXml(tDiffTable*)),	 &mCore, SLOT(slotSaveMapToList(tDiffTable*)) );
	QObject::connect( &mCore, SIGNAL(signalAddToTable(tDiffTable*,size_t,size_t,size_t)),
					  &w,     SLOT  (slotAddDataToTable(tDiffTable*,size_t,size_t,size_t)) );
	QObject::connect( &w,	  SIGNAL(signalMainDirs(QStringList&,QString)), &mCore, SLOT(slotSetMainDirs(QStringList&,QString)) );
	QObject::connect(&mCore,  SIGNAL(signalShowProgress(int,int,int)),		&w,		SLOT(slotShowProgress(int,int,int)));

	QObject::connect( &w,	  SIGNAL(signalPause(bool)),  &mCore, SLOT(slotPauseSync(bool)) );
	QObject::connect( &w,	  SIGNAL(signalCancelSync()), &mCore, SLOT(slotCancelSync()) );
    QObject::connect( w.getSettings(), SIGNAL(signalExitCodesChanged(const QString &)), &mCore, SLOT(slotSetExitCodes(const QString &)) );

	QObject::connect(&mCore,  SIGNAL(signalSyncFinished()),	&w,	  SLOT(slotSyncFinished()) );

    w.show();

    return a.exec();
}
