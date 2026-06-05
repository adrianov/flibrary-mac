#pragma once

#include <QStringList>

#include "export/util.h"

namespace HomeCompa
{

class Zip;

}

class QDateTime;

namespace HomeCompa::Util::Fb2InpxParser
{

static constexpr char LIST_SEPARATOR   = ':';
static constexpr char NAMES_SEPARATOR  = ',';
static constexpr char FIELDS_SEPARATOR = '\x04';

struct ParseResult
{
	QString     line;
	QStringList annotation;
};

UTIL_EXPORT ParseResult Parse(const QString& folder, const Zip& zip, const QString& fileName, const QDateTime& zipDateTime, bool isDeleted);
UTIL_EXPORT QString     GetSeqNumber(QString seqNumber);

} // namespace HomeCompa::Util::Fb2InpxParser
