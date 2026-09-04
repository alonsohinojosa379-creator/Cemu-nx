#include "platform/switch/SwitchThread.h"

#include <switch.h>

#include "Cemu/Logging/CemuLogging.h"

namespace
{
	constexpr int32_t kIdealCoreDontCare = -1;
	constexpr uint32_t kAppCoreMask = 0b0111u;
	constexpr uint32_t kAllCoreMask = 0b1111u;
	constexpr uint32_t kHelperCoreMask = 0b1000u;
	constexpr int32_t kHelperCore = 3;

	constexpr int32_t kBackgroundPriority = 63;
} // namespace

void SwitchThread_RestoreProcessAffinity()
{
	uint64_t mask = 0;
	if (R_FAILED(svcGetInfo(&mask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)))
	{
		cemuLog_log(LogType::Force, "Switch: could not read the process core mask");
		return;
	}
	const Result rc = svcSetThreadCoreMask(CUR_THREAD_HANDLE, kIdealCoreDontCare, static_cast<u32>(mask));
	if (R_FAILED(rc))
		cemuLog_log(LogType::Force, "Switch: could not widen the main thread to core mask 0b{:04b} (rc=0x{:08x})",
				    static_cast<u32>(mask), rc);
	else
		cemuLog_log(LogType::Force, "Switch: {} of 4 CPU cores granted, main thread mask 0b{:04b}",
				    SwitchThread_AllowedCoreCount(), static_cast<u32>(mask));
}

int SwitchThread_AllowedCoreCount()
{
	uint64_t mask = 0;
	if (R_FAILED(svcGetInfo(&mask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0)))
		return 3;
	int cores = 0;
	for (int core = 0; core < 4; core++)
		if (mask & (UINT64_C(1) << core))
			cores++;
	return cores ? cores : 3;
}

void SwitchThread_PinToAppCores(const char* what)
{
	Result rc = svcSetThreadCoreMask(CUR_THREAD_HANDLE, kIdealCoreDontCare, kAppCoreMask);
	if (R_SUCCEEDED(rc))
		return;
	rc = svcSetThreadCoreMask(CUR_THREAD_HANDLE, 0, kAppCoreMask);
	if (R_FAILED(rc))
		cemuLog_log(LogType::Force, "Switch: could not restrict {} to cores 0/1/2 (rc=0x{:08x})", what, rc);
}

bool SwitchThread_PinToHelperCore(const char* what, int32_t priority)
{
	const Result pinResult = svcSetThreadCoreMask(CUR_THREAD_HANDLE, kHelperCore, kHelperCoreMask);
	if (R_SUCCEEDED(pinResult))
	{
		if (R_SUCCEEDED(svcSetThreadPriority(CUR_THREAD_HANDLE, static_cast<u32>(priority))))
			cemuLog_log(LogType::Force, "Switch: {} runs on core 3 at priority {}", what, priority);
		else
			cemuLog_log(LogType::Force, "Switch: {} runs on core 3 at the inherited priority", what);
		return true;
	}

	SwitchThread_PinToAppCores(what);
	if (R_FAILED(svcSetThreadPriority(CUR_THREAD_HANDLE, static_cast<u32>(priority))))
		cemuLog_log(LogType::Force, "Switch: could not raise {} to priority {}", what, priority);
	else
		cemuLog_log(LogType::Force, "Switch: {} stays on cores 0/1/2 at priority {} ({} of 4 cores granted)",
					    what, priority, SwitchThread_AllowedCoreCount());
	return false;
}

void SwitchThread_AllowHelperCore(const char* what)
{
	if (R_FAILED(svcSetThreadCoreMask(CUR_THREAD_HANDLE, kIdealCoreDontCare, kAllCoreMask)))
		SwitchThread_PinToAppCores(what);

	if (R_FAILED(svcSetThreadPriority(CUR_THREAD_HANDLE, static_cast<u32>(kBackgroundPriority))))
		cemuLog_log(LogType::Force, "Switch: could not lower {} to background priority", what);
}
