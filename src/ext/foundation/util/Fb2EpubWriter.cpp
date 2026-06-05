// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion, used in FLibrary.
//
#include "Fb2EpubWriter.h"

#include <QFile>
#include <QUuid>

#include "Fb2EpubNav.h"
#include "Fb2EpubText.h"
#include "EpubZipPack.h"
#include "zip.h"

namespace HomeCompa::Util
{

namespace
{

void AddTextMember(std::vector<EpubMember>& members, const QString& path, const QString& text)
{
	members.emplace_back(path, text.toUtf8());
}

void AddBinaryMember(std::vector<EpubMember>& members, const QString& path, const QByteArray& data)
{
	members.emplace_back(path, data);
}

} // namespace

std::vector<EpubMember> BuildEpubMembers(const ParsedFb2& parsed, const QString& title)
{
	std::vector<EpubMember> members;
	members.reserve(8 + parsed.images.size());

	AddTextMember(members, QStringLiteral("mimetype"), QStringLiteral("application/epub+zip"));

	QString coverFileName;
	QString coverMediaType;
	if (!parsed.coverData.isEmpty())
	{
		coverFileName  = QString("cover.%1").arg(CoverExtension(parsed.coverMime));
		coverMediaType = parsed.coverMime.isEmpty() ? QStringLiteral("image/jpeg") : parsed.coverMime;
		AddBinaryMember(members, QStringLiteral("OEBPS/") + coverFileName, parsed.coverData);
	}

	for (const auto& image : parsed.images)
		AddBinaryMember(members, QStringLiteral("OEBPS/") + image.fileName, image.data);

	const auto containerXml = QStringLiteral(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
		"  <rootfiles>\n"
		"    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
		"  </rootfiles>\n"
		"</container>\n");
	AddTextMember(members, QStringLiteral("META-INF/container.xml"), containerXml);

	const auto safeTitle  = EscapeXmlText(title);
	const auto safeAuthor = EscapeXmlText(parsed.author);
	const auto safeLang   = EscapeXmlText(parsed.language);
	const auto language   = parsed.language.isEmpty() ? QStringLiteral("en") : parsed.language;
	const auto identifier = QString("urn:uuid:%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

	QString opf;
	opf += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	opf += "<package xmlns=\"http://www.idpf.org/2007/opf\" version=\"3.0\" unique-identifier=\"book-id\">\n";
	opf += "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n";
	opf += QString("    <dc:identifier id=\"book-id\">%1</dc:identifier>\n").arg(identifier);
	opf += QString("    <dc:title>%1</dc:title>\n").arg(safeTitle);
	opf += QString("    <dc:language>%1</dc:language>\n").arg(safeLang.isEmpty() ? QStringLiteral("en") : safeLang);
	if (!safeAuthor.isEmpty())
		opf += QString("    <dc:creator>%1</dc:creator>\n").arg(safeAuthor);
	opf += QString("    <meta property=\"dcterms:modified\">%1</meta>\n").arg(EpubTimestampUtc());
	if (!coverFileName.isEmpty())
		opf += "    <meta name=\"cover\" content=\"cover-image\"/>\n";
	opf += "  </metadata>\n  <manifest>\n";
	if (!coverFileName.isEmpty())
	{
		opf += QString("    <item id=\"cover-image\" href=\"%1\" media-type=\"%2\" properties=\"cover-image\"/>\n").arg(coverFileName, coverMediaType);
		opf += "    <item id=\"cover\" href=\"cover.xhtml\" media-type=\"application/xhtml+xml\"/>\n";
	}
	opf += "    <item id=\"content\" href=\"content.xhtml\" media-type=\"application/xhtml+xml\"/>\n";
	opf += "    <item id=\"nav\" href=\"nav.xhtml\" media-type=\"application/xhtml+xml\" properties=\"nav\"/>\n";
	qsizetype imageIndex = 0;
	for (const auto& image : parsed.images)
	{
		const auto mediaType = image.mime.isEmpty() ? QStringLiteral("image/jpeg") : image.mime;
		opf += QString("    <item id=\"img-%1\" href=\"%2\" media-type=\"%3\"/>\n")
		           .arg(++imageIndex)
		           .arg(image.fileName, mediaType);
	}
	opf += "  </manifest>\n  <spine>\n";
	if (!coverFileName.isEmpty())
		opf += "    <itemref idref=\"cover\"/>\n";
	opf += "    <itemref idref=\"content\"/>\n  </spine>\n</package>\n";
	AddTextMember(members, QStringLiteral("OEBPS/content.opf"), opf);

	const auto htmlTitle = EscapeHtmlText(title);
	const auto bodyHtml  = SanitizeXmlText(parsed.bodyHtml);
	const auto contentHtml = QString(
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<!DOCTYPE html>\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\" xml:lang=\"%1\" lang=\"%1\">\n"
		"<head><meta charset=\"utf-8\" /><title>%2</title>%3</head>\n"
		"<body>\n%4\n</body>\n</html>\n")
	                             .arg(language,
	                                  htmlTitle.isEmpty() ? QStringLiteral("Book") : htmlTitle,
	                                  EpubContentStyles(),
	                                  bodyHtml);
	AddTextMember(members, QStringLiteral("OEBPS/content.xhtml"), contentHtml);

	if (!coverFileName.isEmpty())
	{
		const auto coverHtml = QString(
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<!DOCTYPE html>\n"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"%1\" lang=\"%1\">\n"
			"<head><meta charset=\"utf-8\" /><title>%2</title>"
			"<style>body{margin:0;padding:0;text-align:center;}img{max-width:100%%;height:auto;}</style></head>\n"
			"<body><img src=\"%3\" alt=\"Cover\" /></body></html>\n")
		                         .arg(language, htmlTitle.isEmpty() ? QStringLiteral("Cover") : htmlTitle, coverFileName);
		AddTextMember(members, QStringLiteral("OEBPS/cover.xhtml"), coverHtml);
	}

	const auto navHtml = QString(
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<!DOCTYPE html>\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\">\n"
		"<head><meta charset=\"utf-8\" /><title>Table of Contents</title></head>\n"
		"<body><nav epub:type=\"toc\" id=\"toc\"><ol>\n%1    </ol></nav></body></html>\n")
	                         .arg(NavItemsHtml(parsed, !coverFileName.isEmpty(), htmlTitle.isEmpty() ? QStringLiteral("Book") : htmlTitle));
	AddTextMember(members, QStringLiteral("OEBPS/nav.xhtml"), navHtml);

	return members;
}

QByteArray BuildEpubBytes(const ParsedFb2& parsed, const QString& title)
{
	const auto members = BuildEpubMembers(parsed, title);
	return members.empty() ? QByteArray {} : HomeCompa::PackEpubMembers(members);
}

bool WriteEpubBytesToFile(const QByteArray& epubBytes, const QString& epubPath)
{
	if (epubBytes.isEmpty() || epubPath.isEmpty())
		return false;

	const auto tmpPath = epubPath + ".tmp-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
	{
		QFile tmp(tmpPath);
		if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate) || tmp.write(epubBytes) != epubBytes.size())
		{
			QFile::remove(tmpPath);
			return false;
		}
	}

	QFile::remove(epubPath);
	if (!QFile::rename(tmpPath, epubPath))
	{
		QFile::remove(tmpPath);
		return false;
	}

	return true;
}

} // namespace HomeCompa::Util
