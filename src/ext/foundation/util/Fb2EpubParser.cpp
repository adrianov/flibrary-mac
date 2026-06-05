// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubImages.h"
#include "Fb2EpubText.h"

namespace HomeCompa::Util
{

Fb2Parser::Fb2Parser(QIODevice& stream)
	: SaxParser(stream)
{
}

ParsedFb2 Fb2Parser::TakeResult()
{
	FixFootnoteReferences();
	FixFootnoteLinkMarkers();
	AppendFootnotesHtml();

	std::vector<Fb2EmbeddedImage> images;
	ResolveBodyImagePlaceholders(bodyBuffer, images, bodyImageIds, binaries, coverId);

	auto author = authorFirst.trimmed();
	if (!authorLast.trimmed().isEmpty())
		author = author.isEmpty() ? authorLast.trimmed() : QString("%1 %2").arg(author, authorLast.trimmed());

	return ParsedFb2 {
		.title     = titleBuffer.trimmed(),
		.author    = std::move(author),
		.language  = languageBuffer.trimmed(),
		.bodyHtml  = std::move(bodyBuffer),
		.coverData = coverData,
		.coverMime = coverMime,
		.images    = std::move(images),
		.tocItems  = std::move(tocItems),
	};
}

void Fb2Parser::AddTocItem()
{
	if (headingBuffer.isEmpty())
		return;
	tocItems.push_back(Fb2TocItem { .id = QString("h%1").arg(headingCount), .title = headingBuffer.trimmed(), .level = headingLevel });
	headingBuffer.clear();
	headingLevel = 0;
}

bool Fb2Parser::OnCharacters(const QString& /*path*/, const QString& value)
{
	if (value.isEmpty())
		return true;

	if (inBinary)
	{
		binaryBuffer.append(value);
		return true;
	}
	if (inBookTitle)
	{
		AppendCollapsedText(titleBuffer, value);
		return true;
	}
	if (inLanguage)
	{
		AppendCollapsedText(languageBuffer, value);
		return true;
	}
	if (inAuthor && inFirstName)
	{
		AppendCollapsedText(authorFirst, value);
		return true;
	}
	if (inAuthor && inLastName)
	{
		AppendCollapsedText(authorLast, value);
		return true;
	}
	if (!inBody)
		return true;

	if (inNoteTitle && inNoteSection)
	{
		AppendCollapsedText(noteTitleBuffer, value);
		return true;
	}

	AppendBodyText(value);
	return true;
}

} // namespace HomeCompa::Util
