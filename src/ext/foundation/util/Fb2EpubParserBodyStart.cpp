// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubImages.h"
#include "Fb2EpubText.h"

namespace HomeCompa::Util
{

bool Fb2Parser::OnBodyStartElement(const QString& name, const XmlAttributes& attributes)
{
	if (inNotesBody)
	{
		if (name.compare("section", Qt::CaseInsensitive) == 0)
		{
			const auto id = attributes.GetAttribute("id");
			if (!id.isEmpty())
			{
				currentNoteId = id;
				noteBuffer.clear();
				noteTitleBuffer.clear();
				inNoteSection = true;
				ResetBodyChar();
			}
			return true;
		}
		if (!inNoteSection)
			return true;
		if (name.compare("title", Qt::CaseInsensitive) == 0)
		{
			inNoteTitle = true;
			return true;
		}
	}
	else if (name.compare("section", Qt::CaseInsensitive) == 0)
	{
		bodyBuffer.append("<section>\n");
		return true;
	}

	if (name.compare("image", Qt::CaseInsensitive) == 0 && inMainBody && !inNotesBody)
	{
		const auto id = RefIdFromAttr(attributes);
		if (!id.isEmpty() && id != coverId)
		{
			bodyImageIds.push_back(id);
			bodyBuffer.append(QString("{{FB2IMG:%1}}\n").arg(id));
		}
		return true;
	}

	if (!inNotesBody)
	{
		if (name.compare("title", Qt::CaseInsensitive) == 0)
		{
			inTitle      = true;
			headingLevel = 1;
			headingBuffer.clear();
			bodyBuffer.append(QString("<h1 id=\"h%1\">").arg(++headingCount));
			return true;
		}
		if (name.compare("subtitle", Qt::CaseInsensitive) == 0)
		{
			inSubtitle   = true;
			headingLevel = 2;
			headingBuffer.clear();
			bodyBuffer.append(QString("<h2 id=\"h%1\">").arg(++headingCount));
			return true;
		}
	}

	if (name.compare("p", Qt::CaseInsensitive) == 0 && (inTitle || inSubtitle))
	{
		AppendSpaceIfNeeded(bodyBuffer);
		AppendSeparatorIfNeeded(headingBuffer);
		return true;
	}
	if (name.compare("p", Qt::CaseInsensitive) == 0)
	{
		inParagraph = true;
		if (!inNotesBody)
			bodyBuffer.append("<p>");
		return true;
	}
	if (name.compare("poem", Qt::CaseInsensitive) == 0)
	{
		if (!inNotesBody)
			bodyBuffer.append("<section epub:type=\"poem\">\n");
		return true;
	}
	if (name.compare("stanza", Qt::CaseInsensitive) == 0)
	{
		if (!inNotesBody)
			bodyBuffer.append("<p>");
		return true;
	}
	if (name.compare("v", Qt::CaseInsensitive) == 0)
	{
		inVerse = true;
		return true;
	}
	if (name.compare("emphasis", Qt::CaseInsensitive) == 0)
	{
		inEmphasis       = true;
		inlineWordChars  = 0;
		MarkInlineOpenBoundary();
		AppendSpaceBeforeInlineIfNeeded();
		ActiveBuffer().append("<em>");
		return true;
	}
	if (name.compare("strong", Qt::CaseInsensitive) == 0)
	{
		inStrong        = true;
		inlineWordChars = 0;
		MarkInlineOpenBoundary();
		AppendSpaceBeforeInlineIfNeeded();
		ActiveBuffer().append("<strong>");
		return true;
	}
	if (name.compare("empty-line", Qt::CaseInsensitive) == 0 && !inNotesBody)
	{
		bodyBuffer.append("<br />\n");
		return true;
	}
	if (name.compare("a", Qt::CaseInsensitive) == 0)
	{
		const auto fnLink = FootnoteLink(attributes);
		if (!fnLink.isEmpty())
		{
			inLink          = true;
			inlineWordChars = 0;
			MarkInlineOpenBoundary();
			bodyBuffer.append(QString("<a epub:type=\"noteref\" href=\"%1\">").arg(EscapeHtmlText(fnLink)));
			return true;
		}
		const auto href = FirstAttr(attributes, { "xlink:href", "l:href", "href" });
		if (!href.isEmpty())
		{
			inLink          = true;
			inlineWordChars = 0;
			MarkInlineOpenBoundary();
			AppendSpaceBeforeInlineIfNeeded();
			ActiveBuffer().append(QString("<a href=\"%1\">").arg(EscapeHtmlText(href)));
		}
		return true;
	}
	return true;
}

} // namespace HomeCompa::Util
