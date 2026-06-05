#pragma once

#include <iostream>
#include <memory>

#include "NonCopyMovable.h"

class QIODevice;

#include "export/fnd.h"

namespace HomeCompa
{

class QStdIStream final : public std::istream
{
	NON_COPY_MOVABLE(QStdIStream)

public:
	FND_EXPORT static std::unique_ptr<std::istream> create(QIODevice& dev);

private:
	explicit QStdIStream(std::unique_ptr<class QStdIStreamBuf> buf);
	~QStdIStream() override;

private:
	std::unique_ptr<QStdIStreamBuf> m_buf;
};

class QStdOStream final : public std::ostream
{
	NON_COPY_MOVABLE(QStdOStream)

public:
	FND_EXPORT static std::unique_ptr<std::ostream> create(QIODevice& dev);

private:
	explicit QStdOStream(std::unique_ptr<class QStdOStreamBuf> buf);
	~QStdOStream() override;

private:
	std::unique_ptr<QStdOStreamBuf> m_buf;
};

} // namespace HomeCompa
