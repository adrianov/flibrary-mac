#pragma once

namespace HomeCompa::Util::BookHash
{

class IParser
{
public:
	virtual ~IParser() = default;

	virtual HashParseResult GetResult() = 0;
	virtual ImageHashItem   GetCover()  = 0;
	virtual ImageHashItems  GetImages() = 0;
};

}
