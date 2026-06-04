#pragma once

#include "ArchiveParser.h"

namespace HomeCompa::Util::EpubParser
{

struct ParseResult;

}

namespace HomeCompa::Flibrary::ArchiveDescriptionFallback
{

void Apply(ArchiveParser::Data& data);
void ApplyEpub(ArchiveParser::Data& data, const Util::EpubParser::ParseResult& parsed);

}
