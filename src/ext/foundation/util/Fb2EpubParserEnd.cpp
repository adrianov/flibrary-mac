// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

namespace HomeCompa::Util
{

bool Fb2Parser::OnEndElement(const QString& name, const QString& /*path*/)
{
	if (name.compare("title-info", Qt::CaseInsensitive) == 0)
	{
		inTitleInfo = false;
		return true;
	}
	if (name.compare("body", Qt::CaseInsensitive) == 0)
	{
		if (inNotesBody)
			inNotesBody = false;
		else if (inMainBody)
			inMainBody = false;
		inBody = inMainBody || inNotesBody;
		return true;
	}
	if (name.compare("book-title", Qt::CaseInsensitive) == 0)
	{
		inBookTitle = false;
		return true;
	}
	if (name.compare("lang", Qt::CaseInsensitive) == 0)
	{
		inLanguage = false;
		return true;
	}
	if (name.compare("author", Qt::CaseInsensitive) == 0)
	{
		inAuthor = inFirstName = inLastName = false;
		return true;
	}
	if (name.compare("first-name", Qt::CaseInsensitive) == 0)
	{
		inFirstName = false;
		return true;
	}
	if (name.compare("last-name", Qt::CaseInsensitive) == 0)
	{
		inLastName = false;
		return true;
	}
	if (name.compare("binary", Qt::CaseInsensitive) == 0 && inBinary)
	{
		inBinary = false;
		const auto data = QByteArray::fromBase64(binaryBuffer.toUtf8());
		binaryBuffer.clear();
		if (!currentBinaryId.isEmpty() && !data.isEmpty())
		{
			binaries.insert(currentBinaryId, qMakePair(data, currentBinaryMime));
			if (currentBinaryId == coverId && coverData.isEmpty())
			{
				coverData = data;
				coverMime = currentBinaryMime;
			}
		}
		currentBinaryId.clear();
		currentBinaryMime.clear();
		return true;
	}

	if (!inBody)
		return true;

	if (inNotesBody)
	{
		if (name.compare("title", Qt::CaseInsensitive) == 0 && inNoteTitle)
		{
			inNoteTitle = false;
			return true;
		}
		if (name.compare("section", Qt::CaseInsensitive) == 0 && inNoteSection)
		{
			if (!noteBuffer.isEmpty())
				footnotes[currentNoteId] = noteBuffer.trimmed();
			RegisterFootnoteAliases(currentNoteId, noteTitleBuffer.trimmed());
			currentNoteId.clear();
			noteBuffer.clear();
			noteTitleBuffer.clear();
			inNoteSection = false;
			ResetBodyChar();
			return true;
		}
	}
	else if (name.compare("section", Qt::CaseInsensitive) == 0)
	{
		bodyBuffer.append("</section>\n");
		return true;
	}

	if (!inNotesBody)
	{
		if (name.compare("title", Qt::CaseInsensitive) == 0)
		{
			inTitle = false;
			bodyBuffer.append("</h1>\n");
			AddTocItem();
			return true;
		}
		if (name.compare("subtitle", Qt::CaseInsensitive) == 0)
		{
			inSubtitle = false;
			bodyBuffer.append("</h2>\n");
			AddTocItem();
			return true;
		}
	}

	if (name.compare("p", Qt::CaseInsensitive) == 0 && !(inTitle || inSubtitle))
	{
		inParagraph = false;
		if (!inNotesBody)
			bodyBuffer.append("</p>\n");
		return true;
	}
	if (name.compare("emphasis", Qt::CaseInsensitive) == 0)
	{
		inEmphasis = false;
		ActiveBuffer().append("</em>");
		MarkInlineCloseBoundary();
		return true;
	}
	if (name.compare("strong", Qt::CaseInsensitive) == 0)
	{
		inStrong = false;
		ActiveBuffer().append("</strong>");
		MarkInlineCloseBoundary();
		return true;
	}
	if (name.compare("a", Qt::CaseInsensitive) == 0 && inLink)
	{
		inLink = false;
		ActiveBuffer().append("</a>");
		MarkInlineCloseBoundary();
		return true;
	}
	return true;
}

bool ParseFb2(QIODevice& stream, ParsedFb2& result)
{
	Fb2Parser parser(stream);
	parser.Parse();
	result = parser.TakeResult();
	return !result.bodyHtml.isEmpty();
}

} // namespace HomeCompa::Util
