#include "platform/switch/SwitchThread.h"

#include <switch.h>

#include <atomic>
#include <cstdio>
#include <malloc.h>
#include <pthread.h>
#include <time.h>

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

void SwitchThread_ReportCreateFailure(const char* what, int errorCode)
{
	FILE* file = std::fopen("sdmc:/switch/cemu/thread-fail.txt", "a");
	if (!file)
		return;
	u64 memoryTotal = 0, memoryUsed = 0;
	svcGetInfo(&memoryTotal, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
	svcGetInfo(&memoryUsed, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
	const struct mallinfo allocator = mallinfo();
	std::fprintf(file, "thread=%s errno=%d (11=EAGAIN, 12=ENOMEM)\n", what, errorCode);
	std::fprintf(file, "process_memory_total=%llu process_memory_used=%llu\n",
	             static_cast<unsigned long long>(memoryTotal), static_cast<unsigned long long>(memoryUsed));
	std::fprintf(file, "malloc_arena=%d malloc_in_use=%d malloc_free=%d\n", allocator.arena, allocator.uordblks,
	             allocator.fordblks);
	std::fclose(file);
}

namespace
{
	std::atomic<unsigned> s_shaderModules{0};
	std::atomic<unsigned> s_pipelines{0};
}

void SwitchThread_CountShaderModule() { s_shaderModules.fetch_add(1, std::memory_order_relaxed); }
void SwitchThread_CountPipeline() { s_pipelines.fetch_add(1, std::memory_order_relaxed); }

namespace
{
	void* MemoryMonitorMain(void*)
	{
		const time_t start = time(nullptr);
		u64 lowestFree = ~0ull;
		for (;;)
		{
			u64 memoryTotal = 0, memoryUsed = 0;
			svcGetInfo(&memoryTotal, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
			svcGetInfo(&memoryUsed, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
			const u64 memoryFree = memoryTotal > memoryUsed ? memoryTotal - memoryUsed : 0;
			if (memoryFree < lowestFree)
				lowestFree = memoryFree;
			const struct mallinfo allocator = mallinfo();
			if (FILE* file = std::fopen("sdmc:/switch/cemu/memlog.txt", "a"))
			{
				std::fprintf(file, "t=%llds free=%lluMB lowest_free=%lluMB used=%lluMB malloc_in_use=%dMB malloc_arena=%dMB shaders=%u pipelines=%u\n",
				             static_cast<long long>(time(nullptr) - start),
				             static_cast<unsigned long long>(memoryFree >> 20),
				             static_cast<unsigned long long>(lowestFree >> 20),
				             static_cast<unsigned long long>(memoryUsed >> 20), allocator.uordblks >> 20,
				             allocator.arena >> 20, s_shaderModules.load(), s_pipelines.load());
				std::fclose(file);
			}
			svcSleepThread(5ull * 1000ull * 1000ull * 1000ull);
		}
		return nullptr;
	}
} // namespace

void SwitchThread_StartMemoryMonitor()
{
	static bool started = false;
	if (started)
		return;
	started = true;
	if (FILE* file = std::fopen("sdmc:/switch/cemu/memlog.txt", "w"))
		std::fclose(file);
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0)
		return;
	pthread_attr_setstacksize(&attr, 32 * 1024);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_t thread;
	pthread_create(&thread, &attr, MemoryMonitorMain, nullptr);
	pthread_attr_destroy(&attr);
}
