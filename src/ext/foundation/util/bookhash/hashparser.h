#pragma once

#include "flihash.h"

#include "export/util.h"

class QIODevice;

namespace HomeCompa::Util
{

struct HashParser
{
#define HASH_PARSER_CALLBACK_ITEMS_X_MACRO  \
	HASH_PARSER_CALLBACK_ITEM(id)           \
	HASH_PARSER_CALLBACK_ITEM(hash)         \
	HASH_PARSER_CALLBACK_ITEM(folder)       \
	HASH_PARSER_CALLBACK_ITEM(file)         \
	HASH_PARSER_CALLBACK_ITEM(title)        \
	HASH_PARSER_CALLBACK_ITEM(originFolder) \
	HASH_PARSER_CALLBACK_ITEM(originFile)

	struct HashImageItem
	{
		QString id;
		QString hash;
		QString pHash;
	};

	struct Section
	{
		Section* parent { nullptr };
		size_t   count { 0 };
		size_t   size { 0 };
		using Ptr = std::unique_ptr<Section>;
		std::unordered_map<QString, Ptr> children;
	};

	using HashImageItems = std::vector<HashImageItem>;

	class IObserver // NOLINT(cppcoreguidelines-special-member-functions)
	{
	public:
		virtual ~IObserver()                                  = default;
		virtual void OnParseStarted(const QString& sourceLib) = 0;
		virtual bool OnBookParsed(
#define HASH_PARSER_CALLBACK_ITEM(NAME) QString NAME,
			HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM
				HashImageItem cover,
			HashImageItems    images,
			Section::Ptr      section,
			TextHistogram     textHistogram,
			QStringList       annotation
		) = 0;
	};

	UTIL_EXPORT static void Parse(QIODevice& input, IObserver& observer);
};

} // namespace HomeCompa::Util
