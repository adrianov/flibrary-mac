#include "fb2_epub_test_checks.h"
#include "util/EpubBooksPack.h"
#include "util/Fb2EpubConverter.h"
#include "util/Fb2EpubParser.h"

#include "zip.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <cstdlib>
#include <iostream>

namespace
{

using HomeCompa::Util::ParsedFb2;

bool ConvertOnly(const QString& fb2Path, const QString& epubPath)
{
	QFile fb2File(fb2Path);
	if (!fb2File.open(QIODevice::ReadOnly))
	{
		std::cerr << "cannot open input\n";
		return false;
	}

	const auto bytes = fb2File.readAll();
	fb2File.seek(0);

	ParsedFb2 parsed;
	if (!HomeCompa::Util::ParseFb2(fb2File, parsed) || parsed.bodyHtml.isEmpty())
	{
		std::cerr << "parse failed\n";
		return false;
	}

	const auto bytesEpub = epubPath + ".bytes.epub";
	QFile::remove(bytesEpub);
	if (!HomeCompa::Util::ConvertFb2BytesToEpub(bytes, QFileInfo(fb2Path).fileName(), bytesEpub))
	{
		std::cerr << "ConvertFb2BytesToEpub failed\n";
		return false;
	}

	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "conversion failed\n";
		return false;
	}

	if (!HomeCompa::Util::IsBooksCompatibleEpub(epubPath) || !HomeCompa::Util::IsBooksCompatibleEpub(bytesEpub))
	{
		std::cerr << "epub is not books-compatible\n";
		return false;
	}

	std::cout << "ok " << epubPath.toStdString() << " images=" << parsed.images.size() << "\n";
	return QFileInfo::exists(epubPath) && QFileInfo(epubPath).size() > 0;
}

} // namespace

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	if (argc == 3 && std::getenv("FB2_EPUB_CONVERT_ONLY") != nullptr)
		return ConvertOnly(argv[1], argv[2]) ? 0 : 1;

	if (std::getenv("ARCHIVE_FILE_NAME_ONLY") != nullptr)
		return Fb2EpubTest::CheckArchiveFileName() ? 0 : 1;

	if (argc == 4 && std::getenv("ARCHIVE_EXPORT_ONLY") != nullptr)
		return Fb2EpubTest::CheckArchiveEpubExport(argv[1], argv[2], argv[3]) ? 0 : 1;

	if (argc == 3 && std::getenv("EPUB_REPACK_ONLY") != nullptr)
		return HomeCompa::Util::RepackEpubForBooks(argv[1], argv[2]) ? 0 : 1;

	if (!Fb2EpubTest::CheckGluedWords())
		return 1;

	if (!Fb2EpubTest::CheckArchiveFileName())
		return 1;

	if (!Fb2EpubTest::CheckWarningSpacing(QStringLiteral("../../tests/fixtures/warning_spacing.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckEmphasisParenSpacing(QStringLiteral("../../tests/fixtures/emphasis_paren_spacing.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckFootnoteAfterSentence(QStringLiteral("../../tests/fixtures/footnote_after_sentence.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckFootnotePunctSpacing(QStringLiteral("../../tests/fixtures/footnote_punct_spacing.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckFootnoteEmphasis(QStringLiteral("../../tests/fixtures/footnote_emphasis.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckInlineWordBoundary(QStringLiteral("../../tests/fixtures/inline_word_boundary.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckDropCap(QStringLiteral("../../tests/fixtures/drop_cap.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckInlineImage(QStringLiteral("../../tests/fixtures/inline_image.fb2")))
		return 1;

	if (!Fb2EpubTest::CheckSpecialTitle(QStringLiteral("../../tests/fixtures/special_title.fb2")))
		return 1;

#ifdef Q_OS_MACOS
	if (!Fb2EpubTest::CheckEpubRepack())
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

	if (!convertOnly && !Fb2EpubTest::CheckParsed(parsed))
		return 1;

	if (!HomeCompa::Util::ConvertFb2ToEpub(fb2Path, epubPath))
	{
		std::cerr << "conversion failed\n";
		return 1;
	}

	if (!convertOnly && !Fb2EpubTest::CheckCoverOverride(fb2Path))
		return 1;

	if (!QFileInfo::exists(epubPath) || QFileInfo(epubPath).size() == 0)
	{
		std::cerr << "output missing\n";
		return 1;
	}

	const auto contentXhtml = QString::fromUtf8(Fb2EpubTest::ReadEpubMember(epubPath, QStringLiteral("OEBPS/content.xhtml")));
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
