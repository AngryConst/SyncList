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
            return temp.join("/");
	}
	else
		return QString();
}

//******************************************************************************
QString tFileInfo::relatePath()
{
	if(path.isEmpty())
		return QString();

    return path.join("/"); // Путь без слеша в конце
}
//******************************************************************************
QString tFileInfo::relatePathReduced()
{
    if(path.isEmpty())
        return QString();

    auto tmp = path;
    tmp.removeFirst();
    return tmp.join("/");
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
    return path.join("/").append("/").append(name);
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
