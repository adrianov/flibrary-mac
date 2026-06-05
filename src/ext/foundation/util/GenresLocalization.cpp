#include "GenresLocalization.h"

#include <ranges>
#include <unordered_map>

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QTranslator>

#include "fnd/ScopedCall.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

struct Fixer
{
	Fixer()
	{
		const QDir dir = QCoreApplication::applicationDirPath() + "/locales";

		for (const auto& file : dir.entryList(QStringList() << QString("*.qm"), QDir::Files))
		{
			QTranslator translator;
			const auto  fileName = dir.absoluteFilePath(file);
			if (!translator.load(fileName))
				continue;

			const ScopedCall translatorGuard(
				[&] {
					QCoreApplication::installTranslator(&translator);
				},
				[&] {
					QCoreApplication::removeTranslator(&translator);
				}
			);

			for (const auto* genre : GENRES)
				m_map.try_emplace(QCoreApplication::translate(GENRE, genre).toLower(), genre);
		}
	}

	const QString& Fix(const QString& src) const
	{
		if (const auto it = m_map.find(src.toLower()); it != m_map.end())
			return it->second;
		return src;
	}

private:
	std::unordered_map<QString, QString> m_map;
};

using GenreFixer = const QString& (*)(const QString& src);

std::unique_ptr<Fixer> GENRE_FIXER;

const QString& FixGenreImpl(const QString& src)
{
	assert(GENRE_FIXER);
	return GENRE_FIXER->Fix(src);
}

const QString& FixGenreStub(const QString& src)
{
	return src;
}

GenreFixer FIX_GENRE = &FixGenreStub;

const QString& FixGenre(const QString& src)
{
	return FIX_GENRE(src);
}

} // namespace

GenreFixerInitializer::GenreFixerInitializer()
{
	GENRE_FIXER = std::make_unique<Fixer>();
	FIX_GENRE   = &FixGenreImpl;
}

GenreFixerInitializer::~GenreFixerInitializer()
{
	FIX_GENRE = &FixGenreStub;
	GENRE_FIXER.reset();
}

namespace HomeCompa::Util
{

void ParseGenresString(QStringList& dst, QString src)
{
	src.replace(':', QChar { 0xA789 });
	src.replace(',', '/');
	src.replace(';', '/');
	for (const auto& genre : src.split('/') | std::views::transform([](const auto& item) {
								 return item.trimmed();
							 }) | std::views::filter([](const auto& item) {
								 return item.length() > 2;
							 }))
		dst << FixGenre(genre);
}

}
