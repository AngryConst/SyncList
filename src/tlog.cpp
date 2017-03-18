#include <QMutexLocker>
#include "tlog.h"
#include "logcategories.h"

//******************************************************************************
tLog::tLog(QObject *parent):QObject(parent)
{
	showDebug    = true;
	showInfo	 = true;
	showCritical = true;
	showWarning  = true;
}

//******************************************************************************
tLog &tLog::instance()
{
	static tLog log;
	return log;
}

//******************************************************************************
void tLog::append(QtMsgType &type, const QMessageLogContext &context, const QString &msg)
{
	QMutexLocker locker(&mutex);

	if(record)
	{
		QDateTime currentTime = QDateTime::currentDateTime();
		logDateTime.push_back( currentTime );
		logType.push_back(	   type	   );
		logContext.push_back(  context.category );
		logMessage.push_back(  msg	   );

		QString messageToLog = context.category + QString(" ") +
							   currentTime.toString("yyyy.MM.dd hh:mm:ss.zzz ") +
							   msg;
		emit signalLogChanged(messageToLog);
	}
}

//******************************************************************************
void tLog::slotNeedRecord(bool rec)
{
	record = rec;
}

//******************************************************************************
void tLog::slotClearLog()
{
	QMutexLocker locker(&mutex);

	logDateTime.clear();
	logType.clear();
	logContext.clear();
	logMessage.clear();
}

//******************************************************************************
void tLog::slotShowDebug(bool show_)
{
	showDebug = show_;

	showNeedTypeMsg();
}

//******************************************************************************
void tLog::slotShowInfo(bool show_)
{
	showInfo = show_;

	showNeedTypeMsg();
}

//******************************************************************************
void tLog::slotShowWarning(bool show_)
{
	showWarning = show_;

	showNeedTypeMsg();
}

//******************************************************************************
void tLog::slotShowCritical(bool show_)
{
	showCritical = show_;

	showNeedTypeMsg();
}

//******************************************************************************
void tLog::showOneMessage(int i)
{


	QString messageToLog = logContext.at(i) + QString(" ") +
						   logDateTime.at(i).toString("yyyy.MM.dd hh:mm:ss.zzz ") +
						   logMessage.at(i);
	emit signalLogChanged(messageToLog);
}

//******************************************************************************
void tLog::showNeedTypeMsg()
{
	// Текстовое поле для вывода лога на экран должно быть предварительно очищено
	QMutexLocker locker(&mutex);

	int size = logType.size();

	for(int i=0; i<size; ++i)
	{
		if(showDebug && logType.at(i)==QtMsgType::QtDebugMsg)
			showOneMessage(i);

		if(showCritical && logType.at(i)==QtMsgType::QtCriticalMsg)
			showOneMessage(i);

		if(showWarning && logType.at(i)==QtMsgType::QtWarningMsg)
			showOneMessage(i);

		if(showInfo && logContext.at(i)==logInfo().categoryName())
			showOneMessage(i);
	}
}
