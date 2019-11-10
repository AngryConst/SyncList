#ifndef CORESYNC_H
#define CORESYNC_H

#include <QFileInfoList>
#include <QDir>
#include <QMap>
#include <QXmlStreamWriter>
#include <QVector>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QWaitCondition>
#include <QMutex>


#include "tfileinfo.h"

static const QString EqualStr  = "==";
static const QString NewestStr = ">>";
static const QString LatestStr = "<<";
enum tDirectSync { Equal, Newest, Latest };
typedef QMap<tDirectSync, QString> tDirectSyncMap;


// Элемент для сравнения
struct tDiffItem
{
    bool sync; // Требует ли синхронизации
    bool isSync; // Был ли синхронизирован

	tFileInfo source;// Файл - источник
	tFileInfo destination; // Файл - приёмник
	tDirectSync direction; // Направление синхронизации

	// Функция установки всех параметров
	void setData(tFileInfo &source_, tFileInfo &destination_, const tDirectSync &direct_)
	{
		source = source_;
		destination = destination_;
		direction = direct_;

		sync = (direction == Equal) ? true : false;
        isSync = false;
	}
};
Q_DECLARE_METATYPE(tDiffItem)
typedef tDiffItem* tDiffItemPtr ;
Q_DECLARE_METATYPE(tDiffItemPtr)

// Таблица сравнения со всеми элементами
typedef QMap< QString, QList<tDiffItem> > tDiffTable;
Q_DECLARE_METATYPE(tDiffTable)


//******************************************************************************
class tCoreSync : public QObject
{
    Q_OBJECT
public:
	tCoreSync(); //!< Конструктор класса.
	~tCoreSync();//!< Деструктор класса.

	//! Псевдоним для пары (итератор, элемент таблицы).
	typedef QPair<tFilesInfo::iterator, tDiffItem> tDiffPair;

signals:
	/** \brief Сигнал, излучаемый когда данные собраны в таблицу и подсчитаны
	 * количества вхождений всех элементов.
	 * \param table  ссылка на таблицу.
	 * \param eq     количество одинаковых элементов.
	 * \param newest количество новых элементов.
	 * \param late   количество старых элементов.*/
	void signalAddToTable(tDiffTable *table, size_t eq, size_t newest, size_t late);

	void signalShowProgress(int min, int max, int current);

	void signalSyncFinished();

public slots:
	void slotSetMainDirs(QStringList &mainDirs, QString dstDir); //!< Принимает на вход список корневых директорий.
	void slotReadList();		  //!< Читает сохранённый ранее список на диске.
	void slotSynchroize();		  //!< Синхронизирует источник и приёмник.
	void slotSaveMapToList(tDiffTable *table);

	void slotPauseSync(bool pause);
	void slotCancelSync();
    void slotSetExitCodes(const QString &errorCodes);

private slots:
	void synchronization(tDiffTable *table);

private:
	QStringList mMainDirs;		  //!< Список корневых директорий - источников.
	QString		mDstDir;		  //!< Папка - приёмник данных.
	tFilesInfo filesInfoFromReal; //!< Рекурсивный список файлов из всех корневых директорий.
	tFilesInfo filesInfoFromXml;  //!< Список файлов, прочитанный из сохранённого файла.

	volatile size_t countEq;	  //!< Подсчёт общего количества одинаковых элементов.
	volatile size_t countNew;	  //!< Подсчёт общего количества новых элементов.
	volatile size_t countLate;	  //!< Подсчёт общего количества старых элементов.

	volatile bool mPause;
	volatile bool mCancel;


	QString mProgramProcessor;
	QString mArgsProgram;

    QSet<int> mExitCodes;

	/** \brief Добавляет все найденные в подкаталогах файлы в общий список.
	 *
	 * Осуществляет рекурсивный спуск. Выход из рекурсии - это отсутствие
	 * подкаталогов ниже проверяемого каталога.
	 * \param srcDir ссылка на текущую директорию.
	 * \param mainDir_ ссылка на имя текущего корневого каталога*/
	void getFileInfoList(const QDir &srcDir, const QString &mainDir_);

	/** \brief Ищет совпадения текущего пути до файла и пути до корневого каталога.
	 *
	 * Возвращает индекс, с которого начинается имя подкаталога (без имени файла
	 * и пути корневого каталога. Например, "path" - без слеша в начале и конце)
	 *
	 * \retval -1, если совпадение не найдено, в противном случае индекс*/
	int  findCoincidence(const QFileInfo &elem, const QString &mDir);

	void saveList();


	//! Записывает параметры текущего файла в поток xml.
	void writeFileToXml(QXmlStreamWriter &stream, const tFileInfo &elem);

	//! Выбор корневых каталогов для анализа файлов и сбор информации о файлах.
	void collectSourceDir();

	//! Выполняет запуск функций для поиска совпадений элементов в несколько потоков.
	void compareSrcAndDst();

	//! Функция для сравнения файлов).
	tDiffItem compareFiles(tFileInfo &real_, tFileInfo & xmlFile);

	void syncInThread(tDiffTable *table);
    void readProcProgSettings();
    void processOneFile(QList<tDiffItem>::iterator currentFile);

    QWaitCondition poolNotFull;
    QMutex mutex;
    int numUsedThreads = 0;
    const int MaxConcurrent = 3;
};

#endif // CORESYNC_H
