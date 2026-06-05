#include "fb2_epub_test_checks.h"

#include "util/EpubBooksPack.h"
#include "util/Fb2EpubConverter.h"
#include "util/Fb2EpubParser.h"
#include "util/Fb2EpubText.h"
#include "util/Fb2Format.h"
#include "util/ImageRestore.h"

#include "platform/StrUtil.h"
#include "settings/Settings.h"
#include "zip.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <exception>
#include <iostream>

namespace Fb2EpubTest
{

using HomeCompa::Util::ParsedFb2;
using HomeCompa::Zip;
bool EpubHasMember(const QString& epubPath, const QString& member)
{
	try
	{
		return Zip(epubPath).GetFileNameList().contains(member);
	}
	catch (...)
	{
		return false;
	}
}

QByteArray ReadEpubMember(const QString& epubPath, const QString& member)
{
	try
	{
		const auto stream = Zip(epubPath).Read(member);
		return stream ? stream->GetStream().readAll() : QByteArray {};
	}
	catch (...)
	{
		return {};
	}
}

bool WriteEpubBytes(const QString& epubPath, const QByteArray& epubBytes)
{
	QFile file(epubPath);
	return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(epubBytes) == epubBytes.size();
}

bool CheckParsed(const ParsedFb2& parsed)
{
	const auto& html = parsed.bodyHtml;
	if (!html.contains(QStringLiteral("компании<em> Veuve")))
	{
		std::cerr << "missing space before emphasis\n";
		return false;
	}
	if (!html.contains(QStringLiteral("вот le jour")))
	{
		std::cerr << "missing space between scripts\n";
		return false;
	}
	if (!html.contains(QStringLiteral("не толстеют. Pourquoi<sup><a epub:type=\"noteref\" href=\"#fn-n1\">1</a></sup>")))
	{
		std::cerr << "missing superscript footnote marker\n";
		return false;
	}
	if (html.contains(QStringLiteral("[<a epub:type=\"noteref\"")))
	{
		std::cerr << "footnote should use sup, not bracket markers\n";
		return false;
	}
	if (!html.contains(QStringLiteral("epub:type=\"noteref\"")))
	{
		std::cerr << "missing noteref link\n";
		return false;
	}
	if (!html.contains(QStringLiteral("href=\"#fn-n1\"")))
	{
		std::cerr << "missing footnote link for note 1\n";
		return false;
	}
	if (!html.contains(QStringLiteral("id=\"fn-n1\"")))
	{
		std::cerr << "missing footnote target for note 1\n";
		return false;
	}
	if (!html.contains(QStringLiteral("epub:type=\"footnote\"")))
	{
		std::cerr << "missing footnote aside\n";
		return false;
	}
	if (!html.contains(QStringLiteral("href=\"#fn-n2\"")))
	{
		std::cerr << "missing footnote link\n";
		return false;
	}
	if (!html.contains(QStringLiteral("id=\"fn-n2\"")))
	{
		std::cerr << "missing footnote target\n";
		return false;
	}
	if (html.contains(QStringLiteral("class=\"footnotes\"")))
	{
		std::cerr << "footnotes section should be omitted for pop-up notes\n";
		return false;
	}
	return true;
}

bool CheckArchiveFileName()
{
	const QStringList archiveFiles { QStringLiteral("249292.fb2") };
	if (HomeCompa::Util::ResolveArchiveBookFile(archiveFiles, QStringLiteral("249292")) != QStringLiteral("249292.fb2"))
	{
		std::cerr << "ResolveArchiveBookFile failed for INPX base name\n";
		return false;
	}
	if (!HomeCompa::Util::IsExportableEpubSource(QStringLiteral("249292")))
	{
		std::cerr << "IsExportableEpubSource failed for INPX base name\n";
		return false;
	}
	if (!HomeCompa::Util::IsExportableEpubSource(QStringLiteral("249292.fb2")))
	{
		std::cerr << "IsExportableEpubSource failed for full fb2 name\n";
		return false;
	}
	return true;
}

bool CheckArchiveEpubExport(const QString& archivePath, const QString& bookFile, const QString& epubPath)
{
	try
	{
		const HomeCompa::Zip zip(archivePath);
		const auto        archiveFile = HomeCompa::Util::ResolveArchiveBookFile(zip.GetFileNameList(), bookFile);
		if (archiveFile.isEmpty())
		{
			std::cerr << "ResolveArchiveBookFile returned empty for " << bookFile.toStdString() << '\n';
			return false;
		}
		const auto        settings    = HomeCompa::SettingsFactory::CreateStub();
		const auto        stream      = zip.Read(archiveFile);
		const auto        bytes       = HomeCompa::Util::PrepareToExport(stream->GetStream(), archivePath, archiveFile, *settings);
		if (bytes.isEmpty())
		{
			std::cerr << "PrepareToExport returned empty bytes\n";
			return false;
		}

		const HomeCompa::Util::Fb2ToEpubOptions options { .archiveFolder = archivePath, .bookFile = archiveFile, .settings = settings.get() };
		const auto path = HomeCompa::Platform::PathToString(HomeCompa::Platform::StringToPath(epubPath));
		QFile::remove(path);
		if (!HomeCompa::Util::ConvertFb2BytesToEpub(bytes, archiveFile, path, &options))
		{
			std::cerr << "ConvertFb2BytesToEpub failed for " << path.toStdString() << '\n';
			return false;
		}

		const QFileInfo output(path);
		if (!output.exists() || output.size() == 0)
		{
			std::cerr << "epub output missing\n";
			return false;
		}

		std::cout << "ok " << path.toStdString() << " size=" << output.size() << '\n';
		return true;
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << '\n';
		return false;
	}
}

bool CheckGluedWords()
{
	const auto glued = QStringLiteral("Предупреждение:не рекомендуется");
	if (HomeCompa::Util::SplitGluedWords(glued) != glued)
	{
		std::cerr << "glued: invented space after punctuation\n";
		return false;
	}
	if (HomeCompa::Util::SplitGluedWords(QStringLiteral("12:30")) != QStringLiteral("12:30"))
	{
		std::cerr << "glued: split numeric time\n";
		return false;
	}
	if (HomeCompa::Util::SplitGluedWords(QStringLiteral("bookdesigner@the-ebook.org"))
	    != QStringLiteral("bookdesigner@the-ebook.org"))
	{
		std::cerr << "glued: split email/domain period\n";
		return false;
	}
	const auto scriptGlue = HomeCompa::Util::SplitGluedWords(QStringLiteral("компанииVeuve"));
	if (!scriptGlue.contains(QStringLiteral("компании Veuve")))
	{
		std::cerr << "glued: missing script boundary: " << scriptGlue.toStdString() << '\n';
		return false;
	}
	return true;
}

bool CheckWarningSpacing(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	if (!parsed.bodyHtml.contains(QStringLiteral("Предупреждение: <em>не")))
	{
		std::cerr << "missing space before emphasis after colon: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckEmphasisParenSpacing(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	if (!parsed.bodyHtml.contains(QString::fromUtf8("работает</em> (clean")))
	{
		std::cerr << "missing space before parenthesis after emphasis: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckFootnoteAfterSentence(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	if (!parsed.bodyHtml.contains(QStringLiteral("time.<sup><a epub:type=\"noteref\" href=\"#fn-n8\">8</a></sup> On")))
	{
		std::cerr << "footnote glued to next sentence: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckFootnotePunctSpacing(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	const auto snippet = QStringLiteral("impressa&quot;</em><sup><a epub:type=\"noteref\" href=\"#fn-n_5\">5</a></sup><em>, помоги");
	if (!parsed.bodyHtml.contains(snippet))
	{
		std::cerr << "space before comma after footnote: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	const auto closing = QStringLiteral("inertiae</em><sup><a epub:type=\"noteref\" href=\"#fn-n_6\">6</a></sup><em>»</em>");
	if (!parsed.bodyHtml.contains(closing))
	{
		std::cerr << "space before closing quote after footnote: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckInlineWordBoundary(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	const auto& html = parsed.bodyHtml;
	if (html.contains(QStringLiteral("мечтаетстать")))
	{
		std::cerr << "glued words across inline tag: " << html.toStdString() << '\n';
		return false;
	}
	if (!html.contains(QStringLiteral("</em> стать")))
	{
		std::cerr << "missing space after </em>: " << html.toStdString() << '\n';
		return false;
	}
	if (!html.contains(QStringLiteral("компании<em> Veuve")))
	{
		std::cerr << "missing space after <em> open: " << html.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckFootnoteEmphasis(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	const auto aside = QStringLiteral("<aside epub:type=\"footnote\" id=\"fn-n4\"><p>Никогда, никогда (<em>фр.</em>).</p></aside>");
	if (!parsed.bodyHtml.contains(aside))
	{
		std::cerr << "footnote emphasis not rendered: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}
	if (parsed.bodyHtml.contains(QStringLiteral("&lt;em&gt;")))
	{
		std::cerr << "footnote has escaped emphasis tags\n";
		return false;
	}

	return true;
}

bool CheckDropCap(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open " << fb2Path.toStdString() << '\n';
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << fb2Path.toStdString() << " parse failed\n";
		return false;
	}

	if (!parsed.bodyHtml.contains(QString::fromUtf8("<em>П</em>одумать")))
	{
		std::cerr << "drop cap split from word: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	if (!parsed.bodyHtml.contains(QStringLiteral("компании<em> Veuve")))
	{
		std::cerr << "missing space before emphasis across scripts: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckInlineImage(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open inline image fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "inline image parse failed\n";
		return false;
	}

	if (parsed.images.size() != 1)
	{
		std::cerr << "expected one inline image, got " << parsed.images.size() << '\n';
		return false;
	}

	if (!parsed.bodyHtml.contains(QStringLiteral("<img src=\"images/pic.png\"")))
	{
		std::cerr << "inline image markup missing: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	const auto epubPath = QStringLiteral("/tmp/fb2_epub_inline_image_test.epub");
	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "inline image conversion failed\n";
		return false;
	}

	if (!EpubHasMember(epubPath, QStringLiteral("OEBPS/images/pic.png")))
	{
		std::cerr << "inline image file missing from epub\n";
		return false;
	}

	QFile::remove(epubPath);
	return true;
}

bool CheckSpecialTitle(const QString& fb2Path)
{
	const auto title = QString::fromUtf8(u8"ЖОПА («Жизнь одна — подумай, а!»)");

	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open special title fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "special title parse failed\n";
		return false;
	}

	if (parsed.title != title)
	{
		std::cerr << "unexpected title: " << parsed.title.toStdString() << '\n';
		return false;
	}

	const auto epubPath = QStringLiteral("/tmp/fb2_epub_special_title_test.epub");
	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "special title conversion failed\n";
		return false;
	}

	const auto opf = QString::fromUtf8(ReadEpubMember(epubPath, QStringLiteral("OEBPS/content.opf")));
	QFile::remove(epubPath);
	if (!opf.contains(title))
	{
		std::cerr << "title missing from opf\n";
		return false;
	}

	return true;
}

bool CheckChapterTitle(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open chapter_title fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "chapter_title parse failed\n";
		return false;
	}

	if (!parsed.bodyHtml.contains(QStringLiteral("Глава первая<br />")))
	{
		std::cerr << "missing title line break: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}
	if (parsed.bodyHtml.contains(QString::fromUtf8("Глава первая Коротышки")))
	{
		std::cerr << "title lines glued: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}
	if (parsed.tocItems.empty() || !parsed.tocItems.front().title.contains(QStringLiteral(" — ")))
	{
		std::cerr << "toc title missing separator\n";
		return false;
	}

	return true;
}

bool CheckCoverOverride(const QString& fb2Path)
{
	static const QByteArray png = QByteArray::fromHex(
		"89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c489"
		"0000000a49444154789c6300010000050001000d0a2db40000000049454e44ae426082"
	);

	const auto epubPath = QStringLiteral("/tmp/fb2_epub_cover_test.epub");
	const HomeCompa::Util::EpubCover       cover { png, QStringLiteral("image/png") };
	const HomeCompa::Util::Fb2ToEpubOptions options { .coverOverride = &cover };
	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath, &options))
	{
		std::cerr << "cover override conversion failed\n";
		return false;
	}

	if (!EpubHasMember(epubPath, QStringLiteral("OEBPS/cover.xhtml")) || !EpubHasMember(epubPath, QStringLiteral("OEBPS/cover.png")))
	{
		std::cerr << "cover assets missing from epub\n";
		return false;
	}

	QFile::remove(epubPath);
	return true;
}

bool CheckPoem(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open poem fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "poem parse failed\n";
		return false;
	}

	const auto& html = parsed.bodyHtml;
	if (!html.contains(QStringLiteral("z3998:poem")))
	{
		std::cerr << "missing poem section: " << html.toStdString() << '\n';
		return false;
	}
	if (!html.contains(QString::fromUtf8("Знайка шёл гулять на речку,")))
	{
		std::cerr << "missing first verse line: " << html.toStdString() << '\n';
		return false;
	}
	if (!html.contains(QString::fromUtf8("Перепрыгнул через овечку.")))
	{
		std::cerr << "missing second verse line: " << html.toStdString() << '\n';
		return false;
	}
	if (!html.contains(QStringLiteral("Second stanza line one,")))
	{
		std::cerr << "missing second stanza: " << html.toStdString() << '\n';
		return false;
	}

	return true;
}

bool CheckFb2Elements(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open fb2_elements fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "fb2_elements parse failed\n";
		return false;
	}

	const auto& html = parsed.bodyHtml;
	const auto require = [&](const QString& needle, const char* label) {
		if (html.contains(needle))
			return true;
		std::cerr << "missing " << label << ": " << html.toStdString() << '\n';
		return false;
	};

	if (!require(QStringLiteral("epub:type=\"epigraph\""), "epigraph"))
		return false;
	if (!require(QStringLiteral("Oscar Wilde"), "text-author"))
		return false;
	if (!require(QStringLiteral("<blockquote>"), "cite blockquote"))
		return false;
	if (!require(QStringLiteral("Sign on the wall."), "cite text"))
		return false;
	if (!require(QStringLiteral("<code>printf()</code>"), "code"))
		return false;
	if (!require(QStringLiteral("<s>deleted</s>"), "strikethrough"))
		return false;
	if (!require(QStringLiteral("H<sub>2</sub>O"), "sub"))
		return false;
	if (!require(QStringLiteral("x<sup>2</sup>"), "sup"))
		return false;
	if (!require(QStringLiteral("<u>under</u>"), "underline"))
		return false;
	if (!require(QStringLiteral("<u>lined</u>"), "not_supported_in_fb2_underline"))
		return false;
	if (!require(QStringLiteral("class=\"fb2-note\">custom</span>"), "style"))
		return false;
	if (!require(QStringLiteral("<table>"), "table"))
		return false;
	if (!require(QStringLiteral("<th>Name</th>"), "table header"))
		return false;
	if (!require(QStringLiteral("<td>Alpha</td>"), "table cell"))
		return false;
	if (!require(QStringLiteral("aside class=\"annotation\""), "annotation"))
		return false;
	if (!require(QStringLiteral("Editor note here."), "annotation text"))
		return false;
	if (!require(QStringLiteral("p class=\"subtitle\">Poem title</p>"), "poem subtitle"))
		return false;
	if (!require(QStringLiteral("Anonymous"), "poem text-author"))
		return false;

	return true;
}

bool CheckMetadata(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open fb2_metadata fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "fb2_metadata parse failed\n";
		return false;
	}

	if (parsed.metadata.authors.size() != 2)
	{
		std::cerr << "expected two authors, got " << parsed.metadata.authors.size() << '\n';
		return false;
	}
	if (parsed.metadata.translators.size() != 1)
	{
		std::cerr << "expected one translator\n";
		return false;
	}
	if (!parsed.metadata.annotation.contains(QStringLiteral("great")))
	{
		std::cerr << "annotation text missing\n";
		return false;
	}
	if (parsed.metadata.keywords.size() < 2)
	{
		std::cerr << "keywords missing\n";
		return false;
	}
	if (parsed.metadata.genres.size() < 2)
	{
		std::cerr << "genres missing\n";
		return false;
	}
	if (parsed.metadata.sequence.name != QStringLiteral("Test Series"))
	{
		std::cerr << "sequence name missing\n";
		return false;
	}
	if (parsed.metadata.publishInfo.isbn.isEmpty())
	{
		std::cerr << "isbn missing\n";
		return false;
	}

	const auto epubPath = QStringLiteral("/tmp/fb2_epub_metadata_test.epub");
	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "metadata epub conversion failed\n";
		return false;
	}

	const auto opf = QString::fromUtf8(ReadEpubMember(epubPath, QStringLiteral("OEBPS/content.opf")));
	const auto require = [&](const QString& needle, const char* label) {
		if (opf.contains(needle))
			return true;
		std::cerr << "missing " << label << " in opf: " << opf.toStdString() << '\n';
		return false;
	};

	if (!require(QStringLiteral("<dc:title>Meta Test Book</dc:title>"), "title"))
		return false;
	if (!require(QStringLiteral("Ivan Petrov"), "first author"))
		return false;
	if (!require(QStringLiteral("Anna Petrova"), "second author"))
		return false;
	if (!require(QStringLiteral("John Smith"), "translator"))
		return false;
	if (!require(QStringLiteral("property=\"role\" scheme=\"marc:relators\">aut"), "author role"))
		return false;
	if (!require(QStringLiteral("property=\"role\" scheme=\"marc:relators\">trl"), "translator role"))
		return false;
	if (!require(QStringLiteral("<dc:description>A great book.</dc:description>"), "description"))
		return false;
	if (!require(QStringLiteral("<dc:subject>space</dc:subject>"), "keyword"))
		return false;
	if (!require(QStringLiteral("<dc:subject>sf</dc:subject>"), "genre"))
		return false;
	if (!require(QStringLiteral("<dc:publisher>Test Publisher</dc:publisher>"), "publisher"))
		return false;
	if (!require(QStringLiteral("<dc:date>2024</dc:date>"), "date"))
		return false;
	if (!require(QStringLiteral("urn:isbn:9785389123456"), "isbn identifier"))
		return false;
	if (!require(QStringLiteral("property=\"belongs-to-collection\""), "series"))
		return false;
	if (!require(QStringLiteral("Test Series"), "series name"))
		return false;
	if (!require(QStringLiteral("property=\"group-position\">3"), "series number"))
		return false;
	if (!require(QStringLiteral("<dc:language>ru</dc:language>"), "language"))
		return false;
	if (!require(QStringLiteral("<dc:language>en</dc:language>"), "source language"))
		return false;

	QFile::remove(epubPath);
	return true;
}

bool CheckConversionNext(const QString& fb2Path)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open fb2_conversion_next fixture\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
	{
		std::cerr << "fb2_conversion_next parse failed\n";
		return false;
	}

	if (parsed.metadata.sourceTitle != QStringLiteral("Original Title"))
	{
		std::cerr << "missing source title\n";
		return false;
	}
	if (parsed.metadata.sourceAuthors.size() != 1)
	{
		std::cerr << "missing source author\n";
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("epub:type=\"abstract\"")))
	{
		std::cerr << "missing annotation section: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("First paragraph.")))
	{
		std::cerr << "missing first annotation paragraph\n";
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("Second paragraph.")))
	{
		std::cerr << "missing second annotation paragraph\n";
		return false;
	}
	if (parsed.bodyHtml.contains(QStringLiteral("paragraph.Second")))
	{
		std::cerr << "glued annotation paragraphs: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("<section epub:type=\"abstract\" class=\"book-annotation\">")))
	{
		std::cerr << "missing annotation section\n";
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("fb2-body-appendix")))
	{
		std::cerr << "missing appendix body: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("Appendix text.")))
	{
		std::cerr << "missing appendix text\n";
		return false;
	}
	if (!parsed.bodyHtml.contains(QStringLiteral("alt=\"Diagram alt\"")))
	{
		std::cerr << "missing image alt: " << parsed.bodyHtml.toStdString() << '\n';
		return false;
	}

	const auto epubPath = QStringLiteral("/tmp/fb2_epub_conversion_next_test.epub");
	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "conversion_next epub failed\n";
		return false;
	}

	const auto opf = QString::fromUtf8(ReadEpubMember(epubPath, QStringLiteral("OEBPS/content.opf")));
	if (!opf.contains(QStringLiteral("property=\"dcterms:alternative\">Original Title")))
	{
		std::cerr << "missing source title in opf: " << opf.toStdString() << '\n';
		return false;
	}
	if (!opf.contains(QStringLiteral("name=\"original-author\" content=\"Ivan Ivanov\"")))
	{
		std::cerr << "missing original author in opf\n";
		return false;
	}

	QFile::remove(epubPath);
	return true;
}

bool CheckEpubRepack()
{
	QByteArray badEpubBytes;
	{
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		const auto zipFiles = Zip::CreateZipFileController();
		zipFiles->AddFile(QStringLiteral("i.jpg"), QByteArray("x"));
		Zip zip(buffer, Zip::Format::Zip);
		if (!zip.Write(*zipFiles))
			return false;
		badEpubBytes = buffer.buffer();
	}

	QTemporaryDir dir;
	if (!dir.isValid())
		return false;

	const auto badEpub  = QDir(dir.path()).filePath(QStringLiteral("bad.epub"));
	const auto goodEpub = QDir(dir.path()).filePath(QStringLiteral("good.epub"));
	if (!WriteEpubBytes(badEpub, badEpubBytes))
		return false;

	if (HomeCompa::Util::IsBooksCompatibleEpub(badEpub))
	{
		std::cerr << "bad fixture should not be books-compatible\n";
		return false;
	}

	if (!HomeCompa::Util::RepackEpubForBooks(badEpub, goodEpub) || !HomeCompa::Util::IsBooksCompatibleEpub(goodEpub))
	{
		std::cerr << "epub repack failed\n";
		return false;
	}

	if (!EpubHasMember(goodEpub, QStringLiteral("i.jpg")))
	{
		std::cerr << "repacked epub lost images\n";
		return false;
	}

	return true;
}
}
