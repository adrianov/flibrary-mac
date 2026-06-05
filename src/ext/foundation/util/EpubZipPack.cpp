// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// In-memory EPUB ZIP packing (macOS), used in FLibrary.
//
#include "EpubZipPack.h"

#include <QBuffer>
#include <algorithm>
#include <ranges>

#include "zip.h"

namespace HomeCompa::Util
{

namespace
{

constexpr auto MIME_TYPE       = "application/epub+zip";
constexpr auto MIME_TYPE_FILE = "mimetype";

QString EpubRootPrefix(const QStringList& names)
{
	QStringList files;
	for (const auto& name : names)
	{
		if (name.endsWith('/') || name == QStringLiteral("FLibraryImageIndex.json"))
			continue;
		files << name;
	}

	if (files.isEmpty())
		return {};

	const auto slash = files.front().indexOf('/');
	if (slash <= 0)
		return {};

	const auto prefix = files.front().left(slash + 1);
	if (std::ranges::all_of(files, [&](const QString& name) {
			return name.startsWith(prefix);
		}))
		return prefix;

	return {};
}

} // namespace

bool IsBooksCompatibleEpubBytes(const QByteArray& epubBytes)
{
	try
	{
		QBuffer buffer;
		buffer.setData(epubBytes);
		buffer.open(QIODevice::ReadOnly);

		const Zip zip(buffer);
		const auto names = zip.GetFileNameList();
		if (names.isEmpty() || names.front() != MIME_TYPE_FILE)
			return false;

		const auto stream = zip.Read(MIME_TYPE_FILE);
		return stream && stream->GetStream().readAll() == MIME_TYPE;
	}
	catch (...)
	{
		return false;
	}
}

std::vector<EpubMember> ReadEpubMembers(const QByteArray& epubBytes)
{
	try
	{
		QBuffer buffer;
		buffer.setData(epubBytes);
		buffer.open(QIODevice::ReadOnly);

		const Zip zip(buffer);
		const auto names  = zip.GetFileNameList();
		const auto prefix = EpubRootPrefix(names);

		std::vector<EpubMember> members;
		members.reserve(names.size());
		for (const auto& name : names)
		{
			if (name.endsWith('/') || name == QStringLiteral("FLibraryImageIndex.json"))
				continue;

			const auto entryName = prefix.isEmpty() || !name.startsWith(prefix) ? name : name.mid(prefix.size());
			const auto stream    = zip.Read(name);
			if (!stream)
				return {};

			members.emplace_back(entryName, stream->GetStream().readAll());
		}
		return members;
	}
	catch (...)
	{
		return {};
	}
}

QByteArray RepackEpubMembersForBooks(const std::vector<EpubMember>& members)
{
	auto normalized = members;
	normalized.erase(
		std::remove_if(normalized.begin(), normalized.end(), [](const EpubMember& member) {
			return member.first == MIME_TYPE_FILE;
		}),
		normalized.end()
	);
	normalized.insert(normalized.begin(), { MIME_TYPE_FILE, MIME_TYPE });
	return HomeCompa::PackEpubMembers(normalized);
}

} // namespace HomeCompa::Util
