// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//

#pragma once

#include <QByteArray>
#include <QChar>
#include <QMap>
#include <QString>
#include <initializer_list>
#include <vector>

#include <QPair>

#include "Fb2EpubParser.h"
#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"

namespace HomeCompa::Util
{

inline QString FirstAttr(const XmlAttributes& attributes, std::initializer_list<const char*> keys)
{
	for (const char* key : keys)
	{
		const auto value = attributes.GetAttribute(key);
		if (!value.isEmpty())
			return value;
	}
	return {};
}

class Fb2Parser final : public SaxParser
{
public:
	explicit Fb2Parser(QIODevice& stream);

	[[nodiscard]] ParsedFb2 TakeResult();

private:
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override;
	bool OnBodyStartElement(const QString& name, const XmlAttributes& attributes);
	bool OnEndElement(const QString& name, const QString& path) override;
	bool OnCharacters(const QString& path, const QString& value) override;
	void AddTocItem();

	[[nodiscard]] bool     InContent() const;
	[[nodiscard]] QString& ActiveBuffer();
	void                   MarkInlineOpenBoundary();
	void                   MarkInlineCloseBoundary();
	void                   AppendSpaceBeforeInlineIfNeeded();
	void                   ResetBodyChar();
	void                   AppendBodyText(const QString& value);
	void                   AppendFootnotesHtml();
	void                   FixFootnoteReferences();
	void                   FixFootnoteLinkMarkers();
	void                   RegisterFootnoteAliases(const QString& id, const QString& title);
	[[nodiscard]] QString  ResolveFootnoteKey(const QString& fragment) const;
	[[nodiscard]] QString  FootnoteLink(const XmlAttributes& attributes) const;

	QString                 titleBuffer;
	QString                 languageBuffer;
	QString                 authorFirst;
	QString                 authorLast;
	QString                 bodyBuffer;
	QString                 noteBuffer;
	QString                 noteTitleBuffer;
	QString                 currentNoteId;
	QString                 binaryBuffer;
	QString                 currentBinaryId;
	QString                 currentBinaryMime;
	QString                 headingBuffer;
	QString                 coverId;
	QByteArray              coverData;
	QString                 coverMime;
	QMap<QString, QPair<QByteArray, QString>> binaries;
	std::vector<QString>    bodyImageIds;
	QMap<QString, QString>  footnotes;
	QMap<QString, QString>  footnoteAliases;
	std::vector<Fb2TocItem> tocItems;

	bool    inTitleInfo { false };
	bool    inMainBody { false };
	bool    inNotesBody { false };
	bool    inNoteSection { false };
	bool    inNoteTitle { false };
	bool    sawMainBody { false };
	bool    inBody { false };
	bool    inBookTitle { false };
	bool    inLanguage { false };
	bool    inAuthor { false };
	bool    inFirstName { false };
	bool    inLastName { false };
	bool    inParagraph { false };
	bool    inVerse { false };
	bool    inTitle { false };
	bool    inSubtitle { false };
	bool    inEmphasis { false };
	bool    inStrong { false };
	bool    inLink { false };
	bool    inBinary { false };
	int     headingLevel { 0 };
	quint64 headingCount { 0 };
	QChar   lastBodyChar;
	QChar   lastWordChar;
	bool    needSpaceBeforeText { false };
	int     inlineWordChars { 0 };
};

} // namespace HomeCompa::Util
