#pragma once

#include <filesystem>
#include <string>

#include "fnd/NonCopyMovable.h"

#include "export/platform.h"

namespace HomeCompa::Platform
{

class PLATFORM_EXPORT DyLib
{
	NON_COPY_MOVABLE(DyLib)

public:
	DyLib();
	explicit DyLib(std::filesystem::path moduleName);
	~DyLib();

	bool  IsOpen() const;
	void  Detach();
	bool  Open(std::filesystem::path moduleName);
	void  Close();
	void* GetProc(const std::string& procName);

	template <typename ProcType>
	ProcType GetTypedProc(const std::string& procName)
	{
		void* const procPtr = this->GetProc(procName);
		return reinterpret_cast<ProcType>(procPtr);
	}

	std::string GetErrorDescription()
	{
		return m_errorDescription;
	}

private:
	void*                 m_handle { nullptr };
	std::filesystem::path m_moduleName;
	std::string           m_errorDescription;
};

} // namespace HomeCompa::Platform
