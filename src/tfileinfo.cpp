#include "tfileinfo.h"

//******************************************************************************
tFileInfo::tFileInfo()
{
}

//******************************************************************************
QString tFileInfo::dir()
{
	if(!path.isEmpty())
	{
		QStringList temp = path;
		temp.removeFirst();

		if(temp.isEmpty())
			return QString();
		else
		{
			QString localString;
			auto iter=temp.begin();

			localString = localString + (*iter);
			++iter;
			for( ; iter!=temp.end(); ++iter)
			{
				if(!iter->isEmpty())
					localString = localString + "/" + (*iter);
			}
			return localString; // Путь без слеша в конце
		}
	}
	else
		return QString();
}

//******************************************************************************
QString tFileInfo::relatePath()
{
	if(path.isEmpty())
		return QString();

	QString localString;
	auto iter=path.begin();
	localString = localString + (*iter);
	++iter;
	for( ; iter!=path.end(); ++iter)
	{
		if(!iter->isEmpty())
			localString = localString + "/" + (*iter);
	}
	return localString; // Путь без слеша в конце
}

//******************************************************************************
QString tFileInfo::absolutePath()
{
	if(mainDirPath.isEmpty())
		return relatePath();
	else
		return mainDirPath + "/" + relatePath();
}

//******************************************************************************
QString tFileInfo::relateFilePath()
{
	QString localString;
	for( auto iter=path.begin(); iter!=path.end(); ++iter)
	{
		if(!iter->isEmpty())
			localString = localString + (*iter) + "/";
	}
	localString.append(name);
	return localString;
}

//******************************************************************************
QString tFileInfo::absoluteFilePath()
{
	return mainDirPath + "/" + relateFilePath();
}

//******************************************************************************
bool tFileInfo::operator==(const tFileInfo &other)
{
	if(other.name == name)
		return other.path == path;
	else
		return false;
}
