#include "util/Fb2EpubConverter.h"
#include "util/Fb2EpubParser.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <iostream>

namespace
{

using HomeCompa::Util::ParsedFb2;

bool CheckParsed(const ParsedFb2& parsed)
{
	const auto& html = parsed.bodyHtml;
	if (!html.contains(QStringLiteral("компании <em>Veuve")))
	{
		std::cerr << "missing space before emphasis\n";
		return false;
	}
	if (!html.contains(QStringLiteral("вот le jour")))
	{
		std::cerr << "missing space between scripts\n";
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
	if (!html.contains(QStringLiteral("Примечания")))
	{
		std::cerr << "missing footnotes heading\n";
		return false;
	}
	return true;
}

} // namespace

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
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

	if (!QFileInfo::exists(epubPath) || QFileInfo(epubPath).size() == 0)
	{
		std::cerr << "output missing\n";
		return 1;
	}

	std::cout << "ok " << epubPath.toStdString() << "\n";
	return 0;
}
