#ifndef TFILEINFO_H
#define TFILEINFO_H

//#include <list>

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QMap>

class tFileInfo
{
public:
	tFileInfo();

	//! Название корневой директории без пути.
	inline QString mainDir() {if(!path.isEmpty()) return path.first(); else return QString();}
	inline QString absoluteMainDirPath(){ return mainDirPath + "/" + mainDir(); }

	QString dir();				//!< Относительный путь до текущей директории без корневого каталога.

	QString relatePath();				//!< Путь от корневого каталога, не включая путь до корня.
    QString relatePathReduced();        //!< То же что и relatePath, но без имени текущей папки синхронизации.
	QString relateFilePath();			//!< путь до файла от корневого каталога, не включая путь до корня.

	QString absolutePath();		//!< Абсолютный путь до папки, включая её саму.
	QString absoluteFilePath(); //!< Абсолютный путь до файла от корневого каталога.

	//! Оператор сравнения для поиска по списку с помощью indexOf().
	bool operator== (const tFileInfo &other);

public:
	QString mainDirPath; //!< Путь до корневого каталога поиска, включая букву диска. Не включает само имя корневого каталога.
	QStringList path;	 //!< Относительный путь, начинающийся с имени корневой	дирекории без пути до неё.
	QString     name;	 //!< Имя файла.
	size_t      size;	 //!< Размер файла.
	QDateTime   dateTime;//!< Дата последнего изменения.
};

typedef std::list<tFileInfo> tFilesInfoList; //!< Список файлов.
/**
 * Список файлов. Первый Ключ (QString) - это относительный путь,
 * второй ключ - это имя файла.*/
typedef QMap< QString, QMap<QString,tFileInfo> > tFilesInfo;

#endif // TFILEINFO_H
