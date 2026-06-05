#pragma once

#include <memory>

namespace HomeCompa::DB
{

class ICommand;
class IQuery;

class ITransaction // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~ITransaction() = default;

	virtual bool Commit()   = 0;
	virtual bool Rollback() = 0;

	[[nodiscard]] virtual std::unique_ptr<ICommand> CreateCommand(std::string_view command) = 0;
	[[nodiscard]] virtual std::unique_ptr<IQuery>   CreateQuery(std::string_view command)   = 0;

	[[nodiscard]] std::unique_ptr<ICommand> CreateCommand(const QStringView command)
	{
		const auto str = command.toUtf8();
		return CreateCommand(std::string_view { str.data(), static_cast<size_t>(str.size()) });
	}

	[[nodiscard]] std::unique_ptr<IQuery> CreateQuery(const QStringView command)
	{
		const auto str = command.toUtf8();
		return CreateQuery(std::string_view { str.data(), static_cast<size_t>(str.size()) });
	}
};

} // namespace HomeCompa::DB
