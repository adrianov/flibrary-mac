#pragma once

#include <QString>

#include "fnd/memory.h"

#include "export/network.h"

class QNetworkRequest;

namespace HomeCompa::Network
{

class NETWORK_EXPORT Headers
{
public: // rule 5
	Headers(const Headers&)                = default;
	Headers(Headers&&) noexcept            = default;
	Headers& operator=(const Headers&)     = default;
	Headers& operator=(Headers&&) noexcept = default;

public:
	Headers();
	~Headers();

	void Append(QString key, QByteArray value);

	void SetToRequest(QNetworkRequest& request) const;

private:
	struct Impl;
	PropagateConstPtr<Impl, std::shared_ptr> m_impl;
};

}
