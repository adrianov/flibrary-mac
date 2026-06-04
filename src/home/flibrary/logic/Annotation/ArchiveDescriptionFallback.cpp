#include "ArchiveDescriptionFallback.h"

#include "util/EpubParser.h"

#include "data/DataItem.h"
#include "interface/localization.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

constexpr auto CONTEXT = "Annotation";
constexpr auto NO_FILE_METADATA = QT_TRANSLATE_NOOP("Annotation", "No structured metadata in the file");
TR_DEF

IDataItem* AppendSection(IDataItem& root, const QString& name)
{
	auto child = root.AppendChild(NavigationItem::Create());
	child->SetData(name);
	return child.get();
}

void AppendField(IDataItem& parent, const QString& name, const QString& value)
{
	if (value.isEmpty())
		return;

	auto child = parent.AppendChild(NavigationItem::Create());
	child->SetData(QString("%1: %2").arg(name, value));
}

void AppendTranslators(IDataItem& titleInfo, const IDataItem& translators)
{
	for (size_t i = 0, count = translators.GetChildCount(); i < count; ++i)
	{
		const auto& translator = *translators.GetChild(i);
		auto        node       = titleInfo.AppendChild(NavigationItem::Create());
		node->SetData("translator");
		AppendField(*node, "first-name", translator.GetData(AuthorItem::Column::FirstName));
		AppendField(*node, "last-name", translator.GetData(AuthorItem::Column::LastName));
		AppendField(*node, "middle-name", translator.GetData(AuthorItem::Column::MiddleName));
	}
}

} // namespace

void ArchiveDescriptionFallback::Apply(ArchiveParser::Data& data)
{
	if (data.description->GetChildCount() > 0)
		return;

	const auto& publish = data.publishInfo;
	if (!publish.publisher.isEmpty() || !publish.city.isEmpty() || !publish.year.isEmpty() || !publish.isbn.isEmpty())
	{
		auto* section = AppendSection(*data.description, "publish-info");
		AppendField(*section, "publisher", publish.publisher);
		AppendField(*section, "city", publish.city);
		AppendField(*section, "year", publish.year);
		AppendField(*section, "isbn", publish.isbn);
	}

	if (!data.language.isEmpty() || !data.sourceLanguage.isEmpty() || !data.keywords.empty() || data.translators->GetChildCount() > 0)
	{
		auto* titleInfo = AppendSection(*data.description, "title-info");
		AppendField(*titleInfo, "lang", data.language);
		AppendField(*titleInfo, "src-lang", data.sourceLanguage);
		if (!data.keywords.empty())
		{
			QString keywords;
			for (const auto& item : data.keywords)
				keywords.isEmpty() ? keywords = item : keywords.append(", ").append(item);
			AppendField(*titleInfo, "keywords", keywords);
		}
		AppendTranslators(*titleInfo, *data.translators);
	}

	if (!data.error.isEmpty())
		AppendField(*data.description, "parse-error", data.error);

	if (data.description->GetChildCount() == 0)
		AppendField(*data.description, "note", Tr(NO_FILE_METADATA));
}

void ArchiveDescriptionFallback::ApplyEpub(ArchiveParser::Data& data, const Util::EpubParser::ParseResult& parsed)
{
	if (data.description->GetChildCount() > 0)
		return;

	auto* titleInfo = AppendSection(*data.description, "title-info");
	AppendField(*titleInfo, "book-title", parsed.title);
	AppendField(*titleInfo, "lang", parsed.language);
	if (!parsed.annotation.isEmpty())
		AppendField(*titleInfo, "annotation", parsed.annotation);

	for (const auto& author : parsed.authors)
	{
		auto node = titleInfo->AppendChild(NavigationItem::Create());
		node->SetData("author");
		if (author.size() > 0)
			AppendField(*node, "last-name", author[0]);
		if (author.size() > 1)
			AppendField(*node, "first-name", author[1]);
		if (author.size() > 2)
			AppendField(*node, "middle-name", author[2]);
	}

	if (!parsed.genres.isEmpty())
		AppendField(*titleInfo, "genre", parsed.genres.join(", "));

	if (data.description->GetChildCount() == 1 && titleInfo->GetChildCount() == 0)
		AppendField(*data.description, "note", Tr(NO_FILE_METADATA));
}
