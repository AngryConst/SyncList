#ifndef TLOG_H
#define TLOG_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QDateTime>
#include <QMutex>

#include "tsingleton.h"

class tLog : public QObject/*, public tSingleton<tLog>*/
{
	Q_OBJECT
public:

	static tLog &instance();


	void append(QtMsgType &type, const QMessageLogContext &context, const QString &msg);


signals:
	void signalLogChanged(QString &log_);

public slots:
	void slotNeedRecord(bool rec = false);
	void slotClearLog();
	void slotShowDebug(	  bool show_);
	void slotShowInfo(	  bool show_);
	void slotShowWarning( bool show_);
	void slotShowCritical(bool show_);

private:
	tLog(QObject *parent = nullptr);
	Q_DISABLE_COPY(tLog)

	QList<QDateTime>	logDateTime;
	QList<QtMsgType>	logType;
	QStringList			logContext;
	QStringList			logMessage;

	bool record;

	QMutex mutex;

	bool showDebug;
	bool showInfo;
	bool showCritical;
	bool showWarning;

	void showNeedTypeMsg();
	void showOneMessage(int i);
};


////! Перехватчик сообщений для записи в лог.
//static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
//{
//	tLog::instance().append(type, context, msg);
//}


#endif // TLOG_H
