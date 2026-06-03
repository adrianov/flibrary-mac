#include "util/Fb2EpubConverter.h"
#include "util/Fb2EpubParser.h"
#include "util/Fb2EpubText.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
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
	const auto warning = HomeCompa::Util::SplitGluedWords(QStringLiteral("Предупреждение:не рекомендуется к прочтению несовершеннолетним"));
	if (!warning.contains(QStringLiteral("Предупреждение: не")))
	{
		std::cerr << "glued: " << warning.toStdString() << '\n';
		return false;
	}
	if (HomeCompa::Util::SplitGluedWords(QStringLiteral("12:30")) != QStringLiteral("12:30"))
	{
		std::cerr << "glued: split numeric time\n";
		return false;
	}
	if (!HomeCompa::Util::NeedsSpaceAfterPunctuation(QChar(u'е'), QChar(u':'), QChar(u'н')))
	{
		std::cerr << "missing space after colon before cyrillic word\n";
		return false;
	}
	if (HomeCompa::Util::NeedsSpaceAfterPunctuation(QChar(u'2'), QChar(u':'), QChar(u'3')))
	{
		std::cerr << "should not split numeric colon\n";
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

bool CheckCoverOverride(const QString& fb2Path)
{
	static const QByteArray png = QByteArray::fromHex(
		"89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c489"
		"0000000a49444154789c6300010000050001000d0a2db40000000049454e44ae426082"
	);

	const auto epubPath = QStringLiteral("/tmp/fb2_epub_cover_test.epub");
	const HomeCompa::Util::EpubCover cover { png, QStringLiteral("image/png") };
	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath, &cover))
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

} // namespace

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	if (!CheckGluedWords())
		return 1;

	if (!CheckWarningSpacing(QStringLiteral("../tests/fixtures/warning_spacing.fb2")))
		return 1;

	if (!CheckEmphasisParenSpacing(QStringLiteral("../tests/fixtures/emphasis_paren_spacing.fb2")))
		return 1;

	if (!CheckFootnoteAfterSentence(QStringLiteral("../tests/fixtures/footnote_after_sentence.fb2")))
		return 1;

	if (!CheckDropCap(QStringLiteral("../tests/fixtures/drop_cap.fb2")))
		return 1;

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

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed) || !CheckParsed(parsed))
		return 1;

	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "conversion failed\n";
		return 1;
	}

	if (!CheckCoverOverride(fb2Path))
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
	if (!contentXhtml.contains(QStringLiteral("a[epub|type~=\"noteref\"]"))
	    || !contentXhtml.contains(QStringLiteral("<sup><a epub:type=\"noteref\"")))
	{
		std::cerr << "missing publisher footnote styles or markup in epub\n";
		return 1;
	}

	std::cout << "ok " << epubPath.toStdString() << "\n";
	return 0;
}
