#include "util/EpubBooksPack.h"
#include "util/Fb2EpubConverter.h"
#include "util/Fb2EpubParser.h"
#include "util/Fb2EpubText.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <cstdlib>
#include <iostream>

namespace
{

using HomeCompa::Util::ParsedFb2;

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

	QProcess unzip;
	unzip.start("/usr/bin/unzip", { "-l", epubPath });
	if (!unzip.waitForFinished(-1) || unzip.exitCode() != 0)
	{
		std::cerr << "cannot list inline image epub\n";
		return false;
	}

	const auto listing = QString::fromUtf8(unzip.readAllStandardOutput());
	QFile::remove(epubPath);
	if (!listing.contains(QStringLiteral("OEBPS/images/pic.png")))
	{
		std::cerr << "inline image file missing from epub\n";
		return false;
	}

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

	QProcess unzip;
	unzip.start("/usr/bin/unzip", { "-p", epubPath, "OEBPS/content.opf" });
	if (!unzip.waitForFinished(-1) || unzip.exitCode() != 0)
	{
		std::cerr << "cannot read content.opf\n";
		return false;
	}

	const auto opf = QString::fromUtf8(unzip.readAllStandardOutput());
	QFile::remove(epubPath);
	if (!opf.contains(title))
	{
		std::cerr << "title missing from opf\n";
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

	QProcess unzip;
	unzip.start("/usr/bin/unzip", { "-l", epubPath });
	if (!unzip.waitForFinished(-1) || unzip.exitCode() != 0)
	{
		std::cerr << "cannot list epub archive\n";
		return false;
	}

	const auto listing = QString::fromUtf8(unzip.readAllStandardOutput());
	QFile::remove(epubPath);
	if (!listing.contains(QStringLiteral("OEBPS/cover.xhtml")) || !listing.contains(QStringLiteral("OEBPS/cover.png")))
	{
		std::cerr << "cover assets missing from epub\n";
		return false;
	}

	return true;
}

bool ConvertOnly(const QString& fb2Path, const QString& epubPath)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open input\n";
		return false;
	}

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed) || parsed.bodyHtml.isEmpty())
	{
		std::cerr << "parse failed\n";
		return false;
	}

	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "conversion failed\n";
		return false;
	}

	std::cout << "ok " << epubPath.toStdString() << " images=" << parsed.images.size() << "\n";
	return QFileInfo::exists(epubPath) && QFileInfo(epubPath).size() > 0;
}

bool CheckEpubRepack()
{
	QTemporaryDir dir;
	if (!dir.isValid())
		return false;

	const auto workDir = dir.path();
	{
		QFile image(QDir(workDir).filePath(QStringLiteral("i.jpg")));
		if (!image.open(QIODevice::WriteOnly) || image.write("x") != 1)
			return false;
	}

	const auto badEpub = QDir(workDir).filePath(QStringLiteral("bad.epub"));
	const auto goodEpub = QDir(workDir).filePath(QStringLiteral("good.epub"));

	auto runZip = [&](const QStringList& args) {
		QProcess zip;
		zip.setProgram(QStringLiteral("/usr/bin/zip"));
		zip.setArguments(args);
		zip.setWorkingDirectory(workDir);
		zip.start();
		return zip.waitForFinished(-1) && zip.exitCode() == 0;
	};

	if (!runZip({ QStringLiteral("-X9"), badEpub, QStringLiteral("i.jpg") }))
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

	QProcess unzip;
	unzip.start(QStringLiteral("/usr/bin/unzip"), { QStringLiteral("-l"), goodEpub });
	if (!unzip.waitForFinished(-1) || unzip.exitCode() != 0 || !QString::fromUtf8(unzip.readAllStandardOutput()).contains(QStringLiteral("i.jpg")))
	{
		std::cerr << "repacked epub lost images\n";
		return false;
	}

	return true;
}

} // namespace

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	if (argc == 3 && std::getenv("FB2_EPUB_CONVERT_ONLY") != nullptr)
		return ConvertOnly(argv[1], argv[2]) ? 0 : 1;

	if (argc == 3 && std::getenv("EPUB_REPACK_ONLY") != nullptr)
		return HomeCompa::Util::RepackEpubForBooks(argv[1], argv[2]) ? 0 : 1;

	if (!CheckGluedWords())
		return 1;

	if (!CheckWarningSpacing(QStringLiteral("../../tests/fixtures/warning_spacing.fb2")))
		return 1;

	if (!CheckEmphasisParenSpacing(QStringLiteral("../../tests/fixtures/emphasis_paren_spacing.fb2")))
		return 1;

	if (!CheckFootnoteAfterSentence(QStringLiteral("../../tests/fixtures/footnote_after_sentence.fb2")))
		return 1;

	if (!CheckFootnotePunctSpacing(QStringLiteral("../../tests/fixtures/footnote_punct_spacing.fb2")))
		return 1;

	if (!CheckFootnoteEmphasis(QStringLiteral("../../tests/fixtures/footnote_emphasis.fb2")))
		return 1;

	if (!CheckInlineWordBoundary(QStringLiteral("../../tests/fixtures/inline_word_boundary.fb2")))
		return 1;

	if (!CheckDropCap(QStringLiteral("../../tests/fixtures/drop_cap.fb2")))
		return 1;

	if (!CheckInlineImage(QStringLiteral("../../tests/fixtures/inline_image.fb2")))
		return 1;

	if (!CheckSpecialTitle(QStringLiteral("../../tests/fixtures/special_title.fb2")))
		return 1;

#ifdef Q_OS_MACOS
	if (!CheckEpubRepack())
		return 1;
#endif

	if (argc != 3)
	{
		std::cerr << "usage: fb2_epub_test <input.fb2> <output.epub>\n";
		return 2;
	}

	const QString fb2Path  = argv[1];
	const QString epubPath = argv[2];

	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open input\n";
		return 1;
	}

	const auto convertOnly = std::getenv("FB2_EPUB_CONVERT_ONLY") != nullptr;

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed))
		return 1;

	if (!convertOnly && !CheckParsed(parsed))
		return 1;

	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "conversion failed\n";
		return 1;
	}

	if (!convertOnly && !CheckCoverOverride(fb2Path))
		return 1;

	if (!QFileInfo::exists(epubPath) || QFileInfo(epubPath).size() == 0)
	{
		std::cerr << "output missing\n";
		return 1;
	}

	QProcess unzip;
	unzip.start("/usr/bin/unzip", { "-p", epubPath, "OEBPS/content.xhtml" });
	if (!unzip.waitForFinished(-1) || unzip.exitCode() != 0)
	{
		std::cerr << "cannot read content.xhtml from epub\n";
		return 1;
	}
	const auto contentXhtml = QString::fromUtf8(unzip.readAllStandardOutput());
	if (!convertOnly
	    && (!contentXhtml.contains(QStringLiteral("a[epub|type~=\"noteref\"]"))
	        || !contentXhtml.contains(QStringLiteral("<sup><a epub:type=\"noteref\""))))
	{
		std::cerr << "missing publisher footnote styles or markup in epub\n";
		return 1;
	}

	std::cout << "ok " << epubPath.toStdString() << "\n";
	return 0;
}
