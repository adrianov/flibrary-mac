#include "AnnotationCacheCodecItem.h"

#include <algorithm>

#include <QJsonArray>

#include "data/DataItem.h"

namespace HomeCompa::Flibrary::AnnotationCacheItemCodec
{

namespace
{

class CachedItem final : public DataItem
{
public:
	static IDataItem::Ptr Create(const int columns, const ItemType type)
	{
		return std::make_shared<CachedItem>(columns, type);
	}

	CachedItem(const int columns, const ItemType type)
		: DataItem(static_cast<size_t>(std::max(1, columns)))
		, m_type(type)
	{
	}

private:
	[[nodiscard]] IDataItem::Ptr Clone() const override
	{
		return std::make_shared<CachedItem>(*this);
	}

	[[nodiscard]] ItemType GetType() const noexcept override
	{
		return m_type;
	}

private:
	ItemType m_type;
};

} // namespace

QJsonObject ToJson(const IDataItem& item)
{
	const auto type = item.GetType();
	auto       columns = item.GetColumnCount();
	if (type == ItemType::Books)
		columns = BookItem::Column::Last;

	QJsonArray data;
	for (int c = 0; c < columns; ++c)
		data.append(item.GetRawData(c));

	QJsonArray children;
	for (size_t i = 0, sz = item.GetChildCount(); i < sz; ++i)
		children.append(ToJson(*item.GetChild(i)));

	return {
		{ "id", item.GetId() },
		{ "type", static_cast<int>(type) },
		{ "data", data },
		{ "children", children },
	};
}

IDataItem::Ptr FromJson(const QJsonObject& obj, const ItemType fallbackType)
{
	const auto typeValue = static_cast<ItemType>(obj.value("type").toInt(static_cast<int>(fallbackType)));
	const auto data      = obj.value("data").toArray();
	auto       item      = CachedItem::Create(data.size(), typeValue);

	item->SetId(obj.value("id").toString());

	const auto columnsToCopy = std::min<int>(data.size(), item->GetColumnCount());
	for (int c = 0; c < columnsToCopy; ++c)
		item->SetData(data.at(c).toString(), c);

	for (const auto child : obj.value("children").toArray())
	{
		if (!child.isObject())
			continue;
		item->AppendChild(FromJson(child.toObject(), ItemType::Navigation));
	}

	return item;
}

IDataItem::Ptr FromJsonField(const QJsonObject& root, const char* key, const ItemType fallbackType)
{
	if (!root.contains(key) || !root.value(key).isObject())
		return NavigationItem::Create();
	return FromJson(root.value(key).toObject(), fallbackType);
}

} // namespace HomeCompa::Flibrary::AnnotationCacheItemCodec
