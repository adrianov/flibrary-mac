// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//
#include "Fb2EpubParserImpl.h"

#include "Fb2EpubImages.h"

namespace HomeCompa::Util
{

namespace
{

constexpr auto COVERPAGE_IMAGE = "FictionBook/description/title-info/coverpage/image";

} // namespace

bool Fb2Parser::OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes)
{
	if (name.compare("title-info", Qt::CaseInsensitive) == 0)
	{
		inTitleInfo = true;
		return true;
	}
	if (name.compare("body", Qt::CaseInsensitive) == 0)
	{
		const auto bodyName = attributes.GetAttribute("name");
		if (bodyName.compare("notes", Qt::CaseInsensitive) == 0)
		{
			inNotesBody = true;
			inBody      = true;
			return true;
		}
		if (!sawMainBody)
		{
			sawMainBody = true;
			inMainBody  = true;
			inBody      = true;
			ResetBodyChar();
		}
		return true;
	}

	if (inTitleInfo)
	{
		if (coverId.isEmpty() && (path == COVERPAGE_IMAGE || name.compare("image", Qt::CaseInsensitive) == 0))
		{
			if (const auto id = RefIdFromAttr(attributes); !id.isEmpty())
				coverId = id;
			return true;
		}
		if (name.compare("book-title", Qt::CaseInsensitive) == 0)
		{
			inBookTitle = true;
			return true;
		}
		if (name.compare("lang", Qt::CaseInsensitive) == 0)
		{
			inLanguage = true;
			return true;
		}
		if (name.compare("author", Qt::CaseInsensitive) == 0 && authorFirst.isEmpty() && authorLast.isEmpty())
		{
			inAuthor = true;
			return true;
		}
		if (inAuthor && name.compare("first-name", Qt::CaseInsensitive) == 0)
		{
			inFirstName = true;
			return true;
		}
		if (inAuthor && name.compare("last-name", Qt::CaseInsensitive) == 0)
		{
			inLastName = true;
			return true;
		}
	}

	if (name.compare("binary", Qt::CaseInsensitive) == 0)
	{
		currentBinaryId   = attributes.GetAttribute("id").trimmed();
		currentBinaryMime = attributes.GetAttribute("content-type");
		inBinary          = true;
		binaryBuffer.clear();
		return true;
	}

	if (!inBody)
		return true;

	return OnBodyStartElement(name, attributes);
}

} // namespace HomeCompa::Util
