#include "tcoresync.h"
#include "logcategories.h"

#include <QtGlobal>
#include <QtDebug>
#include <QFile>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QtConcurrent/QtConcurrent>
#include <QProcess>
#include <QElapsedTimer>
#include <QSettings>
#include <QSet>
#include <functional>

#include "tsettingswindow.h"

//******************************************************************************
tCoreSync::tCoreSync() : QObject( nullptr ), mPause(false), mCancel(false)
{
    readProcProgSettings();
}

//******************************************************************************
tCoreSync::~tCoreSync()
{

}

//******************************************************************************
void tCoreSync::readProcProgSettings()
{
    tSettingsWindow sett; // В конструкторе автоматически считаются настройки
    mProgramProcessor = sett.getFullProgramPath();
    mArgsProgram      = sett.getParsedArgs();
    mPartialMatch     = sett.getPartialMatch();
    mDirectComparison = sett.getDirectComparison();
    mSkipCharNumber   = sett.getSkipCharNumber();
    slotSetExitCodes(sett.getExitCodes());
}

//******************************************************************************
// Выбор корневых каталогов для анализа файлов
void tCoreSync::collectSourceDir()
{
    filesInfoFromDst.clear();
    // Запускаем функцию чтения списка из XML файла (или реального каталога) в отдельном потоке.
	// при большом количестве записей в файле даёт значительное преимущество в скорости
    QFuture<void> readingList;
    if(mDirectComparison)
    {
        QDir currentDirr( QDir::toNativeSeparators( mDstDir ) );
        readingList = QtConcurrent::run(this, &tCoreSync::getFileInfoList, currentDirr, mDstDir, std::ref(filesInfoFromDst));
    }
    else
        readingList = QtConcurrent::run(this, &tCoreSync::slotReadList);

	// В это же время осуществляем получение списка файлов с реального диска
    // Выбираем одну директорию из списка за раз
    filesInfoFromSrc.clear();

    for (auto oneDir = mMainDirs.begin(); oneDir != mMainDirs.end(); ++oneDir)
    {
        // Считываем всё содержимое без подкаталогов
        QDir currentDir( QDir::toNativeSeparators( *oneDir ) );

        getFileInfoList( currentDir, *oneDir, filesInfoFromSrc);
    }

	//Дождёмся завершения чтения файла
    readingList.waitForFinished();

	compareSrcAndDst();
}

//******************************************************************************
void tCoreSync::compareSrcAndDst()
{

	QElapsedTimer timer;
	tDiffTable *diffTable = new tDiffTable;

	timer.start();

	countEq=0;
	countNew=0;
	countLate=0;

	// Получаем список всех директорий с файламми
    const auto &relateDirs = filesInfoFromSrc.keys();

	// Проходим циклом по каждой директории
	for(auto relDir=relateDirs.begin(); relDir != relateDirs.end(); ++relDir)
	{
        // Проверяем есть ли в папке назначения папка, соответствующая папке на диске
        if(	filesInfoFromDst.contains( *relDir ) )
		{
            auto &currenSrcDir = filesInfoFromSrc.value(*relDir);
            auto &currenDstDir = filesInfoFromDst.value(*relDir);
            const auto &fnames = currenSrcDir.keys();
			for(auto fname=fnames.begin(); fname != fnames.end(); ++fname)
			{
				// Проверяем есть ли в XML файле файл, соответствующий файлу на диске
                if(	currenDstDir.contains( *fname ) )
				{
                    tFileInfo srcFile = currenSrcDir.value(*fname);
                    tFileInfo dstFile = currenDstDir.value(*fname);
					// Обрабатываем результаты
                    tDiffItem result = compareFiles(srcFile,dstFile);

					switch (result.direction)
					{
						case Equal:
							++countEq;
							break;
						case Newest:
							++countNew;
							break;
						case Latest:
							++countLate;
							break;
						default:
							break;
					}

					if( !result.source.relatePath().isEmpty() )
						(*diffTable)[result.source.relatePath()].push_back( result );
					else
						(*diffTable)[result.destination.relatePath()].push_back( result );
				}
				else
				{
                    // Если файл отсутствует в XML файле, то создадим пустую запись для него
                    tDiffItem tableItem;
                    tFileInfo left = currenSrcDir.value(*fname);
                    tFileInfo right;

                    tableItem.setData( left, right, Newest );
                    ++countNew;
                    tableItem.destination.mainDirPath = tableItem.source.relatePath();
                    tableItem.destination.path << tableItem.source.path;
                    (*diffTable)[tableItem.source.relatePath()].push_back( tableItem );
				}
			}
		}
        else
        {
            auto &currenRealDir = filesInfoFromSrc.value(*relDir);
            const auto &fnames = currenRealDir.keys();
            for(auto fname=fnames.begin(); fname != fnames.end(); ++fname)
            {
                // Если файл отсутствует в XML файле, то создадим пустую запись для него
                tDiffItem tableItem;
                tFileInfo left = currenRealDir.value(*fname);
                tFileInfo right;

                tableItem.setData( left, right, Newest );
                ++countNew;
                tableItem.destination.mainDirPath = tableItem.source.relatePath();
                tableItem.destination.path << tableItem.source.path;
                (*diffTable)[tableItem.source.relatePath()].push_back( tableItem );
            }
        }
	}// for relDir

	/* Если после прохода по всем элементам filesInfoFromReal в filesInfoFromXml
	 * остались данные, то необходимо вывести их в правый столбец*/

	// Проверяем есть ли директории в XML файле, которые отсутствуют на диске,
	// и если есть, то помещаем их все в результирующую таблицу

    // Получаем 2 множества из ключей таблиц
    const QSet<QString> &realDirectories = QSet<QString>::fromList( filesInfoFromSrc.keys() );
    const QSet<QString> &inXmlDirectories = QSet<QString>::fromList(filesInfoFromDst.keys() );

    // Вычитаем из одного множества другое и получаем те элементы, которые есть
    // в первом, но нет во втором
    QSet<QString> subtractSets = inXmlDirectories - realDirectories;

    for(auto unusedDirs=subtractSets.begin(); unusedDirs != subtractSets.end(); ++unusedDirs)
    {
        auto &currentDir = filesInfoFromDst.value(*unusedDirs);
        for(auto unusedFile=currentDir.begin();
                 unusedFile != currentDir.end();
               ++unusedFile)
        {
            tDiffItem tableItem;
            tFileInfo left;
            tFileInfo right = *unusedFile;

            tableItem.setData( left, right, Latest );
            ++countLate;
            tableItem.source.mainDirPath = tableItem.destination.mainDirPath;
            tableItem.source.path = tableItem.destination.path;
            (*diffTable)[tableItem.destination.relatePath()].push_back( tableItem );
        }
    }

    // Проверяем есть ли файлы в XML файле, которые отсутствуют на диске,
    // и если есть, то помещаем их все в результирующую таблицу
    const auto &restRelateDirs = filesInfoFromDst.keys();
    for(auto relDir=restRelateDirs.begin(); relDir != restRelateDirs.end(); ++relDir)
    {
        if(	filesInfoFromSrc.contains( *relDir ) )
        {
            // Получаем 2 множества из ключей таблиц
            auto &currentRealFiles = filesInfoFromSrc.value(*relDir);
            const QSet<QString> realFiles = QSet<QString>::fromList( currentRealFiles.keys() );

            auto &currentXmlFiles = filesInfoFromDst.value(*relDir);
            const QSet<QString> inXmlFiels = QSet<QString>::fromList( currentXmlFiles.keys() );

            // Вычитаем из одного множества другое и получаем те элементы, которые есть
            // в первом, но нет во втором
            QSet<QString> subtractSets = inXmlFiels - realFiles;

            for(auto unusedFile=subtractSets.begin();
                     unusedFile != subtractSets.end();
                   ++unusedFile)
            {
                if(	!currentRealFiles.contains( *unusedFile ) )
                {
                    tDiffItem tableItem;
                    tFileInfo left;
                    tFileInfo right = currentXmlFiles.value(*unusedFile);

                    tableItem.setData( left, right, Latest );
                    ++countLate;
                    tableItem.source.mainDirPath = tableItem.destination.mainDirPath;
                    tableItem.source.path = tableItem.destination.path;
                    (*diffTable)[tableItem.destination.relatePath()].push_back( tableItem );
                }
            }
        }
    }// for relDir

	qDebug(logDebug()) << "Сравнение списков и заполнение таблицы:" << timer.elapsed() << "ms";

    filesInfoFromSrc.clear();
    filesInfoFromDst.clear();

	// Сообщаем, что таблица готова к отображению
	emit signalAddToTable(diffTable, countEq, countNew, countLate);
}

//******************************************************************************
tDiffItem tCoreSync::compareFiles(tFileInfo &src, tFileInfo &dst)
{
	tDiffItem tableItem;

    if(mDirectComparison && mPartialMatch)
    {
        tableItem.setData( src, dst, Equal );
    }
    else
    {
        if( src.size == dst.size)
        {
            if( src.dateTime == dst.dateTime )
            {
                tableItem.setData( src, dst, Equal );
            }
            else if( src.dateTime > dst.dateTime )
            {
                tableItem.setData( src, dst, Newest );
            }
            else if( src.dateTime < dst.dateTime )
            {
                tableItem.setData( src, dst, Latest );
            }
        }
        else
        {
            if( src.dateTime >= dst.dateTime )
            {
                tableItem.setData( src, dst, Newest );
            }
            else if( src.dateTime < dst.dateTime )
            {
                tableItem.setData( src, dst, Latest );
            }
        }
    }

	return tableItem;
}

//******************************************************************************
// !!! Рекурсивная функция. !!!
// Осуществляет рекурсивный спуск. Выход из рекурсии - это отсутствие
// подкаталогов ниже проверяемого каталога.
void tCoreSync::getFileInfoList(const QDir &srcDir, const QString &mainDir_, tFilesInfo &fInfoMap)
{
    const QFileInfoList &list = srcDir.entryInfoList( QDir::Files |
                                                      QDir::Dirs  |
                                                      QDir::NoDotAndDotDot);
	tFileInfo oneFile;
	oneFile.mainDirPath = mainDir_.mid( 0, mainDir_.lastIndexOf("/") );

    for (auto elem = list.begin(); elem != list.end(); ++elem)
    {
        if(elem->isDir())
            getFileInfoList( elem->absoluteFilePath(), mainDir_, fInfoMap);
        else
		{
			QDir currentDir( QDir::toNativeSeparators( mainDir_ ) );

			oneFile.path << currentDir.dirName();

            QString strpath = elem->dir().absolutePath().mid( findMatch(*elem, mainDir_) );
			if( !strpath.isEmpty() )
                oneFile.path << strpath.split("/");

            if(mPartialMatch && (&fInfoMap == &filesInfoFromDst))
                oneFile.name = elem->fileName().chopped(mSkipCharNumber);
            else
                oneFile.name = elem->fileName();

			oneFile.size = elem->size();
			oneFile.dateTime = elem->lastModified();

            fInfoMap[ oneFile.relatePath() ][oneFile.name] = oneFile;
			oneFile.path.clear();
		}
    }
}

//******************************************************************************
void tCoreSync::slotSetMainDirs(QStringList &mainDirs, QString dstDir)
{
    mMainDirs = mainDirs;
	mDstDir	  = dstDir;
	QtConcurrent::run( this, &tCoreSync::collectSourceDir );
}

//******************************************************************************
// Возвращает индекс, если совпадение найдено, в противном случае возвращает -1
int tCoreSync::findMatch(const QFileInfo &elem, const QString &mDir)
{
    int index = -1;

	index = elem.dir().absolutePath().indexOf( mDir );

	if (index != -1)
		return index + mDir.length() + 1;
	else
        qDebug(logWarning()) << "Произошла ошибка при сравнении путей, проверьте верно ли указаны директории для поиска";

	return index;
}

//******************************************************************************
// Запись в XML имени файла и его атрибутов
void tCoreSync::writeFileToXml(QXmlStreamWriter &stream, const tFileInfo &elem)
{
    stream.writeStartElement( "file" );
	stream.writeAttribute( "name", elem.name.toUtf8() );
	stream.writeAttribute( "SizeOfFile", QString::number(elem.size).toUtf8() );
    stream.writeAttribute( "Modif", elem.dateTime.toString("yyyy.MM.dd-hh:mm:ss.zzz").toUtf8() );

    stream.writeEndElement();
}

//******************************************************************************
void tCoreSync::slotSaveMapToList(tDiffTable *table)
{
	if( table == nullptr )
	{
		qDebug(logInfo()) << "Внимание! Сравнение ещё не проводилось, сохранять нечего";
		return;
	}

	if( table->isEmpty() )
	{
		qDebug(logInfo()) << "Внимание! Сравнение ещё не проводилось, сохранять нечего";
		return;
	}

	// Для отображения прогресса
	int minimum = 0;
	int maximum = table->size() - 1;
	int progressValue = 0;

	QFile output(QDir::currentPath() + QDir::separator() + "ListOfSyncFilesFromMap.xml");
	if( output.open(QIODevice::WriteOnly) )
	{
		QXmlStreamWriter stream(&output);
		stream.setAutoFormatting(true);
		stream.writeStartDocument();
		stream.writeStartElement( "document" );
		stream.writeStartElement( "maindir" );

		QDir currentDir( QDir::toNativeSeparators( table->begin().value().begin()->source.mainDir() ) );
        stream.writeAttribute( "name", currentDir.dirName().toUtf8());//qDebug(logDebug()) << currentDir.dirName();

		// Устанавливаем итератор на первый элемент списка для последующей проверки
		auto lastElem = table->begin().value().begin();

		if( !lastElem->source.dir().isEmpty() )
		{
			stream.writeStartElement( "dir" );
			stream.writeAttribute( "name", lastElem->source.dir().toUtf8() );
		}

		// Цикл по ключам словаря(Относительный путь)
		for(tDiffTable::iterator currentDir=table->begin(); currentDir != table->end(); ++currentDir)
		{
			// Цикл по значениям словаря(Вектор с именами файлов)
			for(QList<tDiffItem>::iterator currentFile =  currentDir.value().begin();
										   currentFile != currentDir.value().end();
										   ++currentFile)
			{
				// Только если для файла выбрана синхронизация
				if(  currentFile->sync					&&
                     currentFile->isSync                &&
					!currentFile->source.name.isEmpty() &&
					(currentFile->direction != tDirectSync::Latest) )
				{
					// Проверяем сменилась ли текущая корневая директория
					bool isMainDirChanged = (lastElem->source.absoluteMainDirPath() != currentFile->source.absoluteMainDirPath());

					// Проверяем сменилась ли текущая директория
					if( lastElem->source.relatePath() != currentFile->source.relatePath() )
					{
						if(!lastElem->source.dir().isEmpty())
							stream.writeEndElement(); // Закрываем старый тег dir

						if( !currentFile->source.dir().isEmpty() && !isMainDirChanged )
						{
							stream.writeStartElement( "dir" );
							stream.writeAttribute( "name", currentFile->source.dir().toUtf8() );
						}
					}

					// Проверяем сменилась ли текущая корневая директория
					if( isMainDirChanged )
					{
//                        qDebug(logDebug()) << "dir is changed";
						stream.writeEndElement(); // maindir
						stream.writeStartElement( "maindir" );

						QDir currentDir( QDir::toNativeSeparators( currentFile->source.mainDir() ) );
						stream.writeAttribute( "name", currentDir.dirName().toUtf8());

						if( !currentFile->source.dir().isEmpty() )
						{
							stream.writeStartElement( "dir" );
							stream.writeAttribute( "name", currentFile->source.dir().toUtf8() );
						}
					}

					writeFileToXml(stream, currentFile->source); // file
					lastElem = currentFile;

				}
			}//For file

            emit signalShowProgress(minimum, maximum, ++progressValue );
		}//For dir

		stream.writeEndElement(); // maindir
		stream.writeEndElement(); // document
		stream.writeEndDocument();

		output.close();
	}
	else
	{
		qWarning(logWarning()) << "Во время сохранения файла произошла ошибка:" << output.errorString();
	}

}

//******************************************************************************
void tCoreSync::slotPauseSync(bool pause)
{
	mPause = pause;
}

//******************************************************************************
void tCoreSync::slotCancelSync()
{
    mCancel = true;
}

//******************************************************************************
void tCoreSync::slotSetExitCodes(const QString &errorCodes)
{
    mExitCodes.clear();

    QStringList errorCodeList = errorCodes.split(" ",QString::SkipEmptyParts);
    bool ok=true;
    int currentCode = 0;
    for(auto &code:errorCodeList)
    {
        currentCode = code.toInt(&ok,10);
        if(!ok)
            qCritical(logCritical()) << "Ошибка при преобразовании в число строки: \"" << code <<"\"";
        mExitCodes.insert(currentCode);
    }
}

//******************************************************************************
void tCoreSync::synchronization(tDiffTable *table)
{
    readProcProgSettings();

	QFuture<void> future;
	future = QtConcurrent::run(this, &tCoreSync::syncInThread, table);
//	future.waitForFinished();
}

//******************************************************************************
// Чтение ранее сохранённого списка в память
void tCoreSync::slotReadList()
{
	QFile output(QDir::currentPath() + QDir::separator() + "ListOfSyncFilesFromMap.xml");
    qDebug(logInfo()) << QDir::currentPath() + QDir::separator() + "ListOfSyncFilesFromMap.xml";
	if( output.open(QIODevice::ReadOnly) )
	{
		QXmlStreamReader xml(&output);
        filesInfoFromDst.clear();
		tFileInfo oneFile;
		while (!xml.atEnd())
		{
			QXmlStreamReader::TokenType token = xml.readNext();
			if (token == QXmlStreamReader::StartElement)
			{
				if (xml.name() == "maindir")
				{
					QXmlStreamAttributes attrib = xml.attributes();
					oneFile.path << attrib.value("name").toString();

					continue;
				}
				if (xml.name() == "dir")
				{
					QXmlStreamAttributes attrib = xml.attributes();
					oneFile.path << attrib.value("name").toString();

					continue;
				}
				if (xml.name() == "file")
				{
					QXmlStreamAttributes attrib = xml.attributes();
					oneFile.name = attrib.value("name").toString();
					oneFile.size = attrib.value("SizeOfFile").toULong();
					oneFile.dateTime = QDateTime::fromString(attrib.value("Modif")
                                            .toString(), "yyyy.MM.dd-hh:mm:ss.zzz");
					oneFile.mainDirPath = mDstDir;
                    filesInfoFromDst[oneFile.relatePath()][oneFile.name] = oneFile;

					continue;
				}

			}
			if (token == QXmlStreamReader::EndElement)
			{
				if (xml.name() == "maindir")
				{
					oneFile.path.clear();
					continue;
				}
				if (xml.name() == "dir")
				{
					oneFile.path.pop_back();
					continue;
				}
			}

		} // end while

		if (xml.hasError())
			qWarning(logWarning()) << "Произошла ошибка: " << xml.errorString() << xml.error() << xml.tokenString();

		output.close();
	}
	else
	{
		qWarning(logWarning()) << "Во время чтения файла произошла ошибка:" << output.errorString();
	}
}

//******************************************************************************
void tCoreSync::slotSynchroize()
{
	compareSrcAndDst();
}

//******************************************************************************
void tCoreSync::processOneFile(QList<tDiffItem>::iterator currentFile)
{
    QString program = mProgramProcessor;
    QStringList arguments;
//	QString baseArguments = mArgsProgram.split(" ",QString::SkipEmptyParts);

//    QStringList arguments = mArgsProgram.split(" ",QString::SkipEmptyParts);

    // Только если для файла выбрана синхронизация
    // И только если он был изменён (его дата новее)
    if(  currentFile->sync &&
        (currentFile->direction != tDirectSync::Equal) &&
        !currentFile->source.name.isEmpty() )
    {
        QFileInfo file(currentFile->source.absoluteFilePath());

//				arguments.clear();
//                arguments << baseArguments;
//                if((currentFile->source.size/1024/1024) > 1024)
//                    arguments << "-v1g"; //Делаем архив многотомным, только если размер файла больше гигабайта
//                arguments = mArgsProgram;
        arguments = mArgsProgram.split("#",QString::SkipEmptyParts);
        QString rpath;
        if(mDirectComparison)
            rpath = currentFile->destination.relatePathReduced();
        else
            rpath = currentFile->destination.relatePath();

        arguments.replaceInStrings("{DESTINATION}", mDstDir + "/" + rpath + "/" +
                                   file.completeBaseName()+ "." +file.suffix());
        arguments.replaceInStrings("{SOURCE}", currentFile->source.absoluteFilePath());
//qDebug() << program << arguments;
        QProcess myProcess;
//                QStringList argslist( arguments );
        myProcess.start(program, arguments);

        if( !myProcess.waitForFinished(-1) )
        {
            qCritical(logCritical()) << "Во время синхронизации файла "
                                     << currentFile->source.absoluteFilePath()
                                     << " произошла ошибка: <FONT COLOR=red>"
                                     << myProcess.errorString()
                                     << "</FONT>";

            // Выключим признак синхронизации для файла, если были ошибки
            // При этом он не будет помечен как синхронизированный в XML файле
            currentFile->sync = false;
        }
        else
        {
            if( myProcess.exitStatus() == QProcess::NormalExit )
            {
                if(mExitCodes.isEmpty() || mExitCodes.contains( myProcess.exitCode() ) )
                {
                    qWarning(logInfo()) << currentFile->source.absoluteFilePath() << "<FONT COLOR=green>Success</FONT>";
                    currentFile->isSync = true;
                }
                else
                {
                    qCritical(logCritical()) << "Во время синхронизации файла "
                                             << currentFile->source.absoluteFilePath()
                                             << " произошла ошибка: <FONT COLOR=red>"
                                             << myProcess.errorString()
                                             << " Код выхода: " << myProcess.exitCode()
                                             << " - не входит в список допустимых: " << mExitCodes
                                             << "</FONT>";
                   currentFile->sync = false;

                }

            }
            else
                currentFile->isSync = true;
        }
    }

    // Разрешим запуск нового потока
    mutex.lock();
    if (numUsedThreads > 0)
    {
        --numUsedThreads;
        poolNotFull.wakeAll();
    }
    mutex.unlock();
}

//******************************************************************************
// Выполняется в отдельном потоке, чтобы не замораживать интерфейс
void tCoreSync::syncInThread(tDiffTable *table)
{
	if( table == nullptr )
	{
		qDebug(logInfo()) << "Внимание! Сравнение ещё не проводилось, синхронизировать нечего";
		return;
	}

	if( table->isEmpty() )
	{
		qDebug(logInfo()) << "Внимание! Сравнение ещё не проводилось, синхронизировать нечего";
		return;
	}

	int minimum = 0;
    int maximum = static_cast<int>( countNew );
	int progressValue = 0;

    numUsedThreads = 0;

	// Цикл по ключам словаря(Относительный путь)
	for(tDiffTable::iterator currentDir=table->begin(); currentDir != table->end(); ++currentDir)
	{
		// Цикл по значениям словаря(Вектор с именами файлов)
		for(QList<tDiffItem>::iterator currentFile =  currentDir.value().begin();
									   currentFile != currentDir.value().end();
									   ++currentFile)
		{
			// Если требуется приостановить работу, то запускаем цикл ожидания
			while (mPause)
			{
                // Чтобы не грузить процессор пустым циклом, на каждой итерации
                // замораживаем поток на 500 мсек
				QThread::msleep(500); // 500 мсек
				// Если в режиме ожидания нажали отмену, то прекращаем работу
				if(mCancel)
				{
					mCancel = false;
                    slotSaveMapToList( table );
					return;
				}
			}


			if(mCancel)
			{
				mCancel = false;
                slotSaveMapToList( table );
				return;
			}

            mutex.lock();
            if(numUsedThreads == MaxConcurrent)
                poolNotFull.wait(&mutex);
            mutex.unlock();

            mutex.lock();
            ++numUsedThreads;

            if(currentFile->direction == Newest)
                emit signalShowProgress(minimum, maximum, progressValue++ );
            QtConcurrent::run(this, &tCoreSync::processOneFile, currentFile); // Не ждём завершения потока
            mutex.unlock();
		} // for currentFile
	} // for currentDir

	slotSaveMapToList( table );
	emit signalSyncFinished();
}
