#pragma once

#include <QString>
#include <QStringList>

namespace HomeCompa
{
class ISettings;
}

namespace HomeCompa::Util
{

inline bool IsFb2Suffix(const QString&) { return false; }
inline bool IsEpubSuffix(const QString&) { return false; }
inline bool IsFb2Path(const QString&) { return false; }
inline bool IsEpubPath(const QString&) { return false; }
inline QStringList CollectFb2Args(const QStringList&) { return {}; }

struct Fb2ToEpubOptions
{
	QString archiveFolder;
	QString bookFile;
	ISettings* settings = nullptr;
};

}
