#pragma once

#include <QJsonObject>

#include "interface/constants/Enums.h"
#include "interface/logic/IDataItem.h"

namespace HomeCompa::Flibrary::AnnotationCacheItemCodec
{

[[nodiscard]] QJsonObject   ToJson(const IDataItem& item);
[[nodiscard]] IDataItem::Ptr FromJson(const QJsonObject& obj, ItemType fallbackType = ItemType::Navigation);
[[nodiscard]] IDataItem::Ptr FromJsonField(const QJsonObject& root, const char* key, ItemType fallbackType = ItemType::Navigation);

} // namespace HomeCompa::Flibrary::AnnotationCacheItemCodec
