#pragma once

#include "flihash.h"

#include "export/util.h"

class QCryptographicHash;

namespace HomeCompa::Util
{

using HashValues = std::vector<std::pair<size_t, QString>>;
using Hist       = std::unordered_map<QString, size_t>;

UTIL_EXPORT void ParseBookHash(BookHashItem& bookHashItem, QCryptographicHash& cryptographicHash);
UTIL_EXPORT Hist CollectHistogram(QByteArray body, QCryptographicHash& md5);

struct CalculateHashResult
{
	HashValues hashValues;
	QString    hash;
	size_t     count;
	size_t     size;
};

UTIL_EXPORT CalculateHashResult CalculateHash(Hist& hist);

void SetHash(ImageHashItem& item, QCryptographicHash& cryptographicHash);

}
