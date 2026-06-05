#pragma once

#include <QByteArray>
#include <QString>

namespace HomeCompa::Util
{

struct Fb2ToEpubOptions;

inline bool ConvertFb2ToEpub(const QString&, const QString&, const Fb2ToEpubOptions*) { return false; }
inline bool ConvertFb2BytesToEpub(const QByteArray&, const QString&, const QString&, const Fb2ToEpubOptions*) { return false; }
inline bool RepackEpubForBooks(const QString&, const QString&) { return false; }
inline bool RepackEpubBytesForBooks(const QByteArray&, const QString&) { return false; }

}
