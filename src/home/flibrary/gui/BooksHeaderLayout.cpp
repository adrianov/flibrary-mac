#include "BooksHeaderLayout.h"

#include <numeric>
#include <unordered_map>
#include <vector>

#include <QAbstractItemModel>
#include <QApplication>
#include <QHeaderView>
#include <QStyle>

#include "interface/constants/ModelRole.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

int ColumnWeight(const QString& name)
{
	static const std::unordered_map<QString, int> weights {
		{ "Author", 14 },     { "Title", 22 },    { "Series", 12 },   { "Genre", 12 },
		{ "AuthorFull", 14 }, { "FileName", 12 }, { "Folder", 10 },   { "UpdateDate", 8 },
		{ "Size", 6 },        { "SeqNumber", 5 }, { "Year", 5 },      { "LibRate", 5 },
		{ "UserRate", 5 },    { "Lang", 4 },      { "LibID", 5 },     { "SeriesId", 4 },
	};

	if (const auto it = weights.find(name); it != weights.end())
		return it->second;

	return 8;
}

int HeaderTextWidth(const QHeaderView& header, const QAbstractItemModel& model, const int logicalIndex)
{
	const auto title = model.headerData(logicalIndex, Qt::Horizontal, Role::HeaderTitle).toString();
	const auto margin = QApplication::style()->pixelMetric(QStyle::PM_HeaderMargin);
	return header.fontMetrics().horizontalAdvance(title) + 2 * margin + 8;
}

struct SectionLayout
{
	int logicalIndex;
	int minWidth;
	int weight;
};

std::vector<SectionLayout> VisibleSections(const QHeaderView& header)
{
	const auto* model = header.model();
	if (!model)
		return {};

	std::vector<SectionLayout> sections;
	sections.reserve(header.count());

	for (int i = 0, sz = header.count(); i < sz; ++i)
	{
		if (header.isSectionHidden(i))
			continue;

		const auto name = model->headerData(i, Qt::Horizontal, Role::HeaderName).toString();
		sections.push_back(
			{ i, std::max(header.minimumSectionSize(), HeaderTextWidth(header, *model, i)), ColumnWeight(name) }
		);
	}

	return sections;
}

void SetSectionWidths(QHeaderView& header, const std::vector<SectionLayout>& sections, std::vector<int> widths)
{
	for (size_t i = 0; i < sections.size(); ++i)
		header.resizeSection(sections[i].logicalIndex, std::max(widths[i], sections[i].minWidth));
}

void DistributeWidth(const std::vector<SectionLayout>& sections, int availableWidth, std::vector<int>& widths)
{
	widths.assign(sections.size(), 0);

	const auto minTotal = std::accumulate(sections.begin(), sections.end(), 0, [](const int sum, const SectionLayout& section) {
		return sum + section.minWidth;
	});

	if (minTotal >= availableWidth)
	{
		for (size_t i = 0; i < sections.size(); ++i)
			widths[i] = sections[i].minWidth;
		return;
	}

	const auto extra       = availableWidth - minTotal;
	const auto totalWeight = std::accumulate(sections.begin(), sections.end(), 0, [](const int sum, const SectionLayout& section) {
		return sum + section.weight;
	});

	auto assigned = 0;
	for (size_t i = 0; i < sections.size(); ++i)
	{
		const auto share = i + 1 == sections.size() ? extra - assigned : extra * sections[i].weight / totalWeight;
		widths[i]        = sections[i].minWidth + share;
		assigned += share;
	}
}

} // namespace

namespace HomeCompa::Flibrary::BooksHeaderLayout
{

void ApplyDefaultWidths(QHeaderView& header, const int availableWidth)
{
	const auto sections = VisibleSections(header);
	if (sections.empty() || availableWidth <= 0)
		return;

	std::vector<int> widths;
	DistributeWidth(sections, availableWidth, widths);
	SetSectionWidths(header, sections, widths);
}

void ScaleToWidth(QHeaderView& header, const int availableWidth)
{
	const auto sections = VisibleSections(header);
	if (sections.empty() || availableWidth <= 0)
		return;

	const auto current = header.length();
	if (current <= 0 || current >= availableWidth)
		return;

	std::vector<int> widths;
	widths.reserve(sections.size());

	for (const auto& section : sections)
	{
		const auto size = header.sectionSize(section.logicalIndex);
		widths.push_back(std::max(section.minWidth, size * availableWidth / current));
	}

	const auto assigned = std::accumulate(widths.begin(), widths.end(), 0);
	if (assigned < availableWidth && !widths.empty())
		widths.back() += availableWidth - assigned;

	SetSectionWidths(header, sections, widths);
}

}
