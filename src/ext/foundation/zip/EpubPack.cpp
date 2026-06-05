// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// In-memory EPUB ZIP packing, used in FLibrary.
//
#include "zip.h"

#include <QBuffer>
#include <QVariant>
#include <utility>
#include <vector>

namespace HomeCompa
{

QByteArray PackEpubMembers(const std::vector<std::pair<QString, QByteArray>>& members)
{
	if (members.empty() || members.front().first != QStringLiteral("mimetype"))
		return {};

	const auto zipFiles = Zip::CreateZipFileController();
	for (const auto& [name, body] : members)
		zipFiles->AddFile(name, body);

	QByteArray packed;
	QBuffer buffer(&packed);
	buffer.open(QIODevice::WriteOnly);

	Zip output(buffer, Zip::Format::Zip);
	output.SetProperty(Zip::PropertyId::CompressionLevel, QVariant::fromValue(Zip::CompressionLevel::None));
	if (!output.Write(*zipFiles))
		return {};

	return packed;
}

} // namespace HomeCompa
