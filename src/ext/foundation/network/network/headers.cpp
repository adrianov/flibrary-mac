#include "headers.h"

#include <unordered_map>

#include <QNetworkRequest>
#include <QString>

using namespace HomeCompa::Network;

struct Headers::Impl
{
	std::unordered_map<QString, QByteArray> rawHeaders;
};

Headers::Headers()
	: m_impl { std::shared_ptr<Impl> {} }
{
}

Headers::~Headers() = default;

void Headers::SetToRequest(QNetworkRequest& request) const
{
	if (!m_impl)
		return;

	for (const auto& [key, value] : m_impl->rawHeaders)
		request.setRawHeader(key.toUtf8(), value);
}

void Headers::Append(QString key, QByteArray value)
{
	if (!m_impl)
		m_impl.reset(std::make_shared<Impl>());

	m_impl->rawHeaders.try_emplace(std::move(key), std::move(value));
}
