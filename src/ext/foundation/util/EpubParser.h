#pragma once

#include <QStringList>

#include "fnd/EnumBitmask.h"

#include "export/util.h"

class QIODevice;
class QDateTime;

namespace HomeCompa
{

class Zip;

}

namespace HomeCompa::Util::EpubParser
{

using ImageIndex = std::vector<std::pair<QString, int>>;

struct ContentItem
{
	QString    id;
	QByteArray body;
};

enum class Mode
{
	None   = 0,
	Images = 1 << 0,
	Texts  = 1 << 1,
	All    = Images | Texts
};

struct ParseResult
{
	QString                  title;
	QString                  language;
	QStringList              genres;
	std::vector<QStringList> authors;
	QString                  annotation;
	QByteArray               imageIndex;
	bool                     coverExists { false };
	std::vector<ContentItem> images;
	std::vector<ContentItem> texts;
	std::vector<ContentItem> others;
};

UTIL_EXPORT ParseResult Parse(QIODevice& stream, Mode mode = Mode::None);
UTIL_EXPORT ParseResult Parse(const Zip& zip, const QString& fileName, Mode mode = Mode::None);
UTIL_EXPORT ImageIndex  GetImageIndex(const QByteArray& bytes);

} // namespace HomeCompa::Util::EpubParser

ENABLE_BITMASK_OPERATORS(HomeCompa::Util::EpubParser::Mode);
