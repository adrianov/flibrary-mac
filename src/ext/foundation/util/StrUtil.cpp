#include "StrUtil.h"

#include <QStringList>

#include "fnd/IsOneOf.h"

#include "QtTypes.h"

namespace HomeCompa::Util
{

QString& SimplifyTitle(QString& value)
{
	RemoveIf(value, [](const QChar ch) {
		return ch != ' ' && !IsOneOf(ch.category(), QChar::Number_DecimalDigit, QChar::Letter_Lowercase);
	});

	QStringList digits;
	auto        split = value.split(' ');
	for (auto& word : split)
	{
		QString digitsWord;
		RemoveIf(word, [&](const QChar ch) {
			const auto c = ch.category();
			if (c == QChar::Number_DecimalDigit)
			{
				digitsWord.append(ch);
				return true;
			}

			return c != QChar::Letter_Lowercase;
		});
		if (!digitsWord.isEmpty())
			digits << std::move(digitsWord);
	}
	std::ranges::move(std::move(digits), std::back_inserter(split));

	return value;
}

QString& PrepareTitle(QString& value)
{
	static constexpr std::pair<char16_t, char16_t> replacementChar[] {
		{ 0x0451, 0x0435 },
		{ 0x0439, 0x0438 },
		{ 0x044A, 0x044C },
	};
	const std::pair<QString, QString> replacementString[] {
		{ QString("%1%2").arg(QChar { 0x044B }, QChar { 0x043E }), QString("%1%2").arg(QChar { 0x044C }, QChar { 0x044E }) },
	};

	value = value.toLower();

	std::ranges::transform(value, std::begin(value), [&](const QChar& ch) {
		const auto it = std::ranges::find(replacementChar, ch.unicode(), &std::pair<char16_t, char16_t>::first);
		if (it != std::cend(replacementChar))
			return QChar { it->second };

		const auto category = ch.category();
		if (IsOneOf(category, QChar::Separator_Space, QChar::Separator_Line, QChar::Separator_Paragraph, QChar::Other_Control)
		    || (category >= QChar::Punctuation_Connector && category <= QChar::Punctuation_Other))
			return QChar { 0x20 };

		return ch;
	});

	for (const auto& [from, to] : replacementString)
		value.replace(from, to);

	return value;
}

} // namespace HomeCompa::Util
