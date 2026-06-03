#pragma once

class QHeaderView;

namespace HomeCompa::Flibrary::BooksHeaderLayout
{

void ApplyDefaultWidths(QHeaderView& header, int availableWidth);
void ScaleToWidth(QHeaderView& header, int availableWidth);

}
