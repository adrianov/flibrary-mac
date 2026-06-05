#include "translit.h"

#include <QString>

#include "icu/icu.h"
#include "platform/DyLib.h"

namespace HomeCompa::Util
{

namespace
{

bool TransliterateImpl(const ICU::TransliterateType transliterate, const char* id, QString& str)
{
	assert(transliterate);
	auto src = str.toStdU32String();
	src.push_back(0);
	std::u32string dst;
	if (!transliterate(id, &src, &dst))
		return false;

	str = QString::fromStdU32String(dst);
	return true;
}

}

struct Transliterator::Impl
{
	Platform::DyLib              lib { ICU::LIB_NAME };
	const ICU::TransliterateType transliterate { lib.GetTypedProc<ICU::TransliterateType>(ICU::TRANSLITERATE_NAME) };

	QString Transliterate(QString fileName) const
	{
		if (!transliterate)
			return fileName.replace(' ', '_');

		if (auto result = fileName;
		    TransliterateImpl(transliterate, "ru-ru_Latn/BGN", result) && TransliterateImpl(transliterate, "Any-Latin", result) && TransliterateImpl(transliterate, "Latin-ASCII", result))
			fileName = std::move(result);

		return fileName.replace(' ', '_');
	}
};

Transliterator::Transliterator() = default;

Transliterator::~Transliterator() = default;

bool Transliterator::IsReady() const noexcept
{
	return !!m_impl->transliterate;
}

QString Transliterator::Transliterate(QString fileName) const
{
	return m_impl->Transliterate(std::move(fileName));
}

} // namespace HomeCompa::Util
