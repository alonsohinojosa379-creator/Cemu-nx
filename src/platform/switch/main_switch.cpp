#include <switch.h>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#include "WindowSystem.h"
#include "Cemu/Logging/CemuLogging.h"
#include "Common/ExceptionHandler/ExceptionHandler.h"
#include "audio/SwitchAudioAPI.h"
#include "config/LaunchSettings.h"
#include "SwitchPlatform.h"
#include "SwitchMemoryBudget.h"
#include "common/SwitchStorage.h"
#include "common/SwitchLaunchPolicy.h"
#include "platform/switch/SwitchThread.h"
#include "util/MemMapper/MemMapper.h"
#include "util/MemMapper/SwitchGuestMemory.h"

std::atomic_bool g_isGPUInitFinished = false;

void* g_switchGuestPoolBase = nullptr;
size_t g_switchGuestPoolSize = 0;
size_t g_switchNewlibHeapSize = 0;
size_t g_switchHeapTotalSize = 0;
size_t g_switchInheritedHeapSize = 0;
size_t g_switchRecoveredHeapSize = 0;
u64 g_switchProcessMemoryTotal = 0;
u64 g_switchProcessMemoryUsed = 0;
Result g_switchHeapExpansionResult = 0;
bool g_switchHeapExpansionAttempted = false;

// Mappings that a previous core left behind in the heap (the loader keeps the process when it
// chainloads, so thread stacks that were never unmapped stay inaccessible: perm 0, type heap).
// Found in __libnx_initheap, before malloc exists, so only static storage is used here.
struct SwitchHeapHole
{
	u64 address;
	u64 size;
	u32 type;
	u32 perm;
	u32 attr;
};
constexpr u32 kMaxHeapHoles = 64;
SwitchHeapHole g_switchHeapHoles[kMaxHeapHoles];
u32 g_switchHeapHoleCount = 0;
u64 g_switchHeapSpanBefore = 0;
u64 g_switchHeapSpanAfter = 0;


namespace
{
	constexpr size_t kHeapAlignment = 0x200000;
	constexpr size_t kApplicationReserve = 96 * 1024 * 1024;
	constexpr const char* kLastCoreErrorPath = "sdmc:/switch/cemu/last-core-error.log";

	void WriteLastCoreError(const char* stage, const char* detail)
	{
		FILE* file = std::fopen(kLastCoreErrorPath, "w");
		if (!file)
			return;
		std::fprintf(file, "stage=%s\n", stage ? stage : "unknown");
		std::fprintf(file, "detail=%s\n", detail ? detail : "unknown");
		std::fprintf(file, "inherited_heap=%zu\n", g_switchInheritedHeapSize);
		std::fprintf(file, "total_heap=%zu\n", g_switchHeapTotalSize);
		std::fprintf(file, "newlib_heap=%zu\n", g_switchNewlibHeapSize);
		std::fprintf(file, "guest_pool=%zu\n", g_switchGuestPoolSize);
		std::fprintf(file, "guest_backing=%s\n", MemMapper::DescribeGuestBacking());
		std::fprintf(file, "heap_expansion_attempted=%d\n", g_switchHeapExpansionAttempted ? 1 : 0);
		std::fprintf(file, "heap_expansion_result=0x%08x\n", static_cast<unsigned>(g_switchHeapExpansionResult));
		MemMapper::SwitchGuestMemory::PrintDiagnostics([](void* context, const char* line) {
			std::fprintf(static_cast<FILE*>(context), "%s\n", line);
		}, file);
		std::fclose(file);
	}

	constexpr const char* kLastAbortPath = "sdmc:/switch/cemu/last-abort.log";

	// Runs when a C++ exception goes uncaught (e.g. in a worker thread) and
	// std::terminate() would otherwise end the process with a silent svcBreak.
	// Writes the exception text straight to the SD card before aborting.
	[[noreturn]] void SwitchTerminateHandler()
	{
		char detail[512] = "terminate() without an active C++ exception (abort/assert)";
		if (const std::exception_ptr active = std::current_exception())
		{
			try
			{
				std::rethrow_exception(active);
			}
			catch (const std::exception& exception)
			{
				std::snprintf(detail, sizeof(detail), "std::exception: %s", exception.what());
			}
			catch (...)
			{
				std::snprintf(detail, sizeof(detail), "uncaught exception of unknown (non-std) type");
			}
		}
		if (FILE* file = std::fopen(kLastAbortPath, "w"))
		{
			std::fprintf(file, "detail=%s\n", detail);
			std::fprintf(file, "guest_backing=%s\n", MemMapper::DescribeGuestBacking());
			std::fclose(file);
		}
		cemuLog_log(LogType::Force, "Switch: terminate called: {}", detail);
		cemuLog_waitForFlush();
		std::abort();
	}

	bool RecoverGuestPoolFromApplicationHeap(char* overrideBase, size_t overrideSize)
	{
		u64 heapRegionAddress = 0;
		if (R_FAILED(svcGetInfo(&heapRegionAddress, InfoType_HeapRegionAddress, CUR_PROCESS_HANDLE, 0)) ||
			R_FAILED(svcGetInfo(&g_switchProcessMemoryTotal, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)) ||
			R_FAILED(svcGetInfo(&g_switchProcessMemoryUsed, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0)))
			return false;

		const uintptr_t inheritedStart = reinterpret_cast<uintptr_t>(overrideBase);
		if (overrideSize > UINTPTR_MAX - inheritedStart)
			return false;
		const uintptr_t inheritedEnd = inheritedStart + overrideSize;
		if (heapRegionAddress > inheritedStart || inheritedEnd < heapRegionAddress)
			return false;

		const u64 currentHeapSize64 = inheritedEnd - heapRegionAddress;
		if (currentHeapSize64 > SIZE_MAX || g_switchProcessMemoryTotal <= g_switchProcessMemoryUsed + kApplicationReserve)
			return false;

		size_t expandable = static_cast<size_t>(g_switchProcessMemoryTotal -
			g_switchProcessMemoryUsed - kApplicationReserve) & ~(kHeapAlignment - 1);
		if (expandable < SwitchMemoryBudget_GetMandatoryGuestBackingSize() ||
			overrideSize > SIZE_MAX - expandable)
			return false;

		const size_t desiredPool = SwitchMemoryBudget_ComputeGuestPoolSize(overrideSize + expandable);
		if (desiredPool == 0)
			return false;
		size_t expansionSize = (desiredPool + kHeapAlignment - 1) & ~(kHeapAlignment - 1);
		if (expansionSize > expandable)
			expansionSize = expandable;
		if (expansionSize < SwitchMemoryBudget_GetMandatoryGuestBackingSize())
			return false;

		const size_t currentHeapSize = static_cast<size_t>(currentHeapSize64);
		if (currentHeapSize > SIZE_MAX - expansionSize)
			return false;

		void* expandedHeapBase = nullptr;
		g_switchHeapExpansionAttempted = true;
		g_switchHeapExpansionResult = svcSetHeapSize(&expandedHeapBase, currentHeapSize + expansionSize);
		if (R_FAILED(g_switchHeapExpansionResult) ||
			reinterpret_cast<uintptr_t>(expandedHeapBase) != heapRegionAddress)
			return false;

		g_switchGuestPoolBase = reinterpret_cast<void*>(inheritedEnd);
		g_switchGuestPoolSize = expansionSize;
		g_switchRecoveredHeapSize = expansionSize;
		return true;
	}
}

namespace
{
	// Keeps newlib's heap inside the largest run of readable and writable pages, so malloc never
	// hands out memory that a previous core still has mapped elsewhere (thread stacks).
	void SkipLeftoverHeapMappings(char*& heapStart, char*& heapEnd, size_t& heapSize)
	{
		const u64 begin = reinterpret_cast<u64>(heapStart);
		const u64 end = reinterpret_cast<u64>(heapEnd);
		g_switchHeapSpanBefore = end - begin;
		g_switchHeapHoleCount = 0;

		u64 bestStart = begin, bestSize = 0, runStart = begin, address = begin;
		bool anyHole = false;
		while (address < end)
		{
			MemoryInfo info{};
			u32 pageInfo = 0;
			if (R_FAILED(svcQueryMemory(&info, &pageInfo, address)))
				return; // cannot scan, keep the heap as it is
			const u64 blockEnd = std::min<u64>(info.addr + info.size, end);
			const bool readableWritable = (info.perm & Perm_Rw) == Perm_Rw;
			if (!readableWritable)
			{
				anyHole = true;
				if (g_switchHeapHoleCount < kMaxHeapHoles)
					g_switchHeapHoles[g_switchHeapHoleCount++] = {address, blockEnd - address, info.type, info.perm, info.attr};
				if (address - runStart > bestSize)
				{
					bestStart = runStart;
					bestSize = address - runStart;
				}
				runStart = blockEnd;
			}
			address = blockEnd;
		}
		if (!anyHole)
		{
			g_switchHeapSpanAfter = g_switchHeapSpanBefore;
			return;
		}
		if (end - runStart > bestSize)
		{
			bestStart = runStart;
			bestSize = end - runStart;
		}
		// newlib wants 16-byte alignment; keep 2 MiB alignment at the start for the allocator's sake
		const u64 alignedStart = (bestStart + 0x1FFFFF) & ~0x1FFFFFull;
		if (alignedStart >= bestStart + bestSize)
			return;
		bestSize -= alignedStart - bestStart;
		heapStart = reinterpret_cast<char*>(alignedStart);
		heapEnd = reinterpret_cast<char*>(alignedStart + bestSize);
		heapSize = static_cast<size_t>(bestSize);
		g_switchHeapSpanAfter = bestSize;
	}

	void WriteHeapScanReport()
	{
		FILE* file = std::fopen("sdmc:/switch/cemu/heap-scan.txt", "a");
		if (!file)
			return;
		std::fprintf(file, "heap span before=%llu after=%llu holes=%u\n",
		             static_cast<unsigned long long>(g_switchHeapSpanBefore),
		             static_cast<unsigned long long>(g_switchHeapSpanAfter), g_switchHeapHoleCount);
		for (u32 index = 0; index < g_switchHeapHoleCount; ++index)
		{
			const SwitchHeapHole& hole = g_switchHeapHoles[index];
			std::fprintf(file, "  hole 0x%llx+0x%llx type=0x%x perm=0x%x attr=0x%x\n",
			             static_cast<unsigned long long>(hole.address), static_cast<unsigned long long>(hole.size),
			             hole.type, hole.perm, hole.attr);
		}
		std::fclose(file);
	}
}

extern "C"
{
	u32 __nx_applet_type = AppletType_Application;

	extern char* fake_heap_start;
	extern char* fake_heap_end;

	void __libnx_initheap(void)
	{
		char* heapBase = nullptr;
		size_t heapTotal = 0;
		if (envHasHeapOverride())
		{
			heapBase = (char*)envGetHeapOverrideAddr();
			heapTotal = envGetHeapOverrideSize();
			if (!heapBase || heapTotal == 0)
				diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadGetInfo_Heap));
			g_switchInheritedHeapSize = heapTotal;
		}
		else
		{
			u64 avail = 0, used = 0;
			Result result = svcGetInfo(&avail, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
			if (R_FAILED(result))
				diagAbortWithResult(result);
			result = svcGetInfo(&used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
			if (R_FAILED(result))
				diagAbortWithResult(result);
			if (avail <= used + 0x200000)
				diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_OutOfMemory));
			size_t sz = (avail - used - 0x200000) & ~0x1FFFFFull;
			void* a = nullptr;
			result = svcSetHeapSize(&a, sz);
			if (R_FAILED(result))
				diagAbortWithResult(result);
			if (!a)
				diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));
			heapBase = (char*)a;
			heapTotal = sz;
		}

		const size_t targetGuestPool = SwitchMemoryBudget_ComputeGuestPoolSize(heapTotal);
		const bool recoveredGuestPool = targetGuestPool == 0 && envHasHeapOverride() &&
			RecoverGuestPoolFromApplicationHeap(heapBase, heapTotal);
		g_switchHeapTotalSize = heapTotal + g_switchRecoveredHeapSize;
		if (heapBase && targetGuestPool != 0 && targetGuestPool < heapTotal)
		{
			const size_t heapSize = (heapTotal - targetGuestPool) & ~0x1FFFFFull;
			g_switchGuestPoolBase = heapBase + heapSize;
			fake_heap_start = heapBase;
			fake_heap_end = heapBase + heapSize;
			g_switchGuestPoolSize = heapTotal - heapSize;
			g_switchNewlibHeapSize = heapSize;
		}
		else if (recoveredGuestPool)
		{
			fake_heap_start = heapBase;
			fake_heap_end = heapBase + heapTotal;
			g_switchNewlibHeapSize = heapTotal;
		}
		else
		{
			fake_heap_start = heapBase;
			fake_heap_end = heapBase + heapTotal;
			g_switchNewlibHeapSize = heapTotal;
		}
		SkipLeftoverHeapMappings(fake_heap_start, fake_heap_end, g_switchNewlibHeapSize);
	}
}

void requireConsole() {}

static bool s_romfsMounted = false;
static bool s_hidInitialized = false;
static bool s_socketInitialized = false;
static bool s_storageInitialized = false;

static std::string ReadLaunchGamePath()
{
	FILE* file = std::fopen("sdmc:/switch/cemu/switch.ini", "r");
	if (!file)
		return {};
	char line[2048]{};
	std::string result;
	while (std::fgets(line, sizeof(line), file))
	{
		size_t length = std::strlen(line);
		while (length && (line[length - 1] == '\r' || line[length - 1] == '\n'))
			line[--length] = '\0';
		if (std::strncmp(line, "game=", 5) == 0)
		{
			result.assign(line + 5);
			break;
		}
	}
	std::fclose(file);
	return result;
}

static bool SwitchPlatformInit()
{
	// Before anything else spawns a thread: new threads inherit this mask.
	SwitchThread_RestoreProcessAffinity();
	s_romfsMounted = R_SUCCEEDED(romfsInit());
	s_hidInitialized = R_SUCCEEDED(hidInitialize());
	if (s_hidInitialized)
	{
		padConfigureInput(8, HidNpadStyleSet_NpadStandard);
		// A single Joy-Con held sideways: stick -> left stick, SL/SR -> L/R (second player).
		hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);
	}
	s_socketInitialized = R_SUCCEEDED(socketInitializeDefault());
	SwitchStorage::InitializeFromConfig("sdmc:/switch/cemu/launcher.ini", true, nullptr,
	                                    ReadLaunchGamePath());
	s_storageInitialized = true;
	return s_romfsMounted && s_hidInitialized;
}

static void SwitchPlatformExit()
{
	MemMapper::SwitchGuestMemory::DiscardEarly("core exited before adoption");
	SwitchAudioAPI::Destroy();
	if (s_storageInitialized)
	{
		SwitchStorage::Shutdown();
		s_storageInitialized = false;
	}
	if (s_socketInitialized)
		socketExit();
	if (s_hidInitialized)
		hidExit();
	if (s_romfsMounted)
		romfsExit();
}

// Matches EXIT_DETECTION_STR in switch_launcher/fwd/main.c.
static constexpr const char* kForwarderExitSentinel = "__CEMU_FORWARDER_EXIT__";
static constexpr const char* kLauncherPath = "sdmc:/switch/cemu/cemu.nro";
static constexpr const char* kCorePath = "sdmc:/switch/cemu/.emu/cemu_core.nro";

static std::string ForwarderLauncherPath()
{
	u64 programId = 0;
	if (R_FAILED(svcGetInfo(&programId, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0)) || programId == 0)
		return {};

	char configPath[128]{};
	std::snprintf(configPath, sizeof(configPath), "sdmc:/switch/cemu/forwarders/%016llx.cfg",
	              (unsigned long long)programId);
	FILE* config = std::fopen(configPath, "rb");
	if (!config)
		return {};

	// <nro path>\0<arguments>\0, so the first string is the launcher.
	char target[FS_MAX_PATH]{};
	const size_t length = std::fread(target, 1, sizeof(target) - 1, config);
	std::fclose(config);
	target[length] = '\0';
	return target;
}

static void ReturnToLauncher()
{
	const std::string forwarderPath = ForwarderLauncherPath();
	if (!forwarderPath.empty())
	{
		cemuLog_log(LogType::Force, "Switch: restarting the forwarder to return to {}", forwarderPath);
		cemuLog_waitForFlush();

		// Does not return on success.
		const Result restartResult = appletRestartProgram(nullptr, 0);
		const Result launchResult = appletRequestLaunchApplication(0, nullptr);
		if (R_SUCCEEDED(launchResult) && envHasNextLoad())
		{
			envSetNextLoad(forwarderPath.c_str(), kForwarderExitSentinel);
			return;
		}
		cemuLog_log(LogType::Force,
		            "Switch: no clean restart (0x{:08x}/0x{:08x}), chainloading in place",
		            restartResult, launchResult);
	}

	if (envHasNextLoad())
		envSetNextLoad(kLauncherPath, kLauncherPath);
}

int main(int argc, char* argv[])
{
	std::set_terminate(SwitchTerminateHandler);
	const bool platformInitialized = SwitchPlatformInit();
	WriteHeapScanReport();
	if (!platformInitialized)
	{
		WriteLastCoreError("platform", "platform initialization failed");
		SwitchPlatformExit();
		std::_Exit(EXIT_FAILURE);
	}

	// A disengaged optional means that parsing succeeded and normal emulation
	// should continue.  A value is an explicit early-exit status (for example,
	// --help, --version, or a command-line parse error).
	if (const std::optional<int> commandLineResult = LaunchSettings::HandleCommandline(argc, argv);
		commandLineResult.has_value())
	{
		if (*commandLineResult == EXIT_SUCCESS)
			std::remove(kLastCoreErrorPath);
		else
			WriteLastCoreError("platform", "invalid command line");
		SwitchPlatformExit();
		std::_Exit(*commandLineResult);
	}
	std::remove(kLastCoreErrorPath);

	std::remove(SwitchLaunch::ErrorPath);
	int status = EXIT_SUCCESS;
	try
	{
		ExceptionHandler_Init();
		WindowSystem::Create();
	}
	catch (const SwitchLaunch::Failure& exception)
	{
		WriteLastCoreError("host-memory", exception.what());
		SwitchLaunch::WriteError(exception.error);
		status = EXIT_FAILURE;
	}
	catch (const std::exception& exception)
	{
		WriteLastCoreError("window", exception.what());
		SwitchLaunch::WriteError(SwitchLaunch::Error::LaunchFailed);
		status = EXIT_FAILURE;
	}
	catch (...)
	{
		WriteLastCoreError("window", "unknown exception");
		SwitchLaunch::WriteError(SwitchLaunch::Error::LaunchFailed);
		status = EXIT_FAILURE;
	}

	if (SwitchPlatform_ShouldRestartCore())
	{
		cemuLog_log(LogType::Force, "Switch: chainloading the core again for the relaunched title");
		cemuLog_waitForFlush();
		if (envHasNextLoad())
			envSetNextLoad(kCorePath, kCorePath);
		else
			ReturnToLauncher();
	}
	else if (SwitchPlatform_ShouldReturnToLauncher())
		ReturnToLauncher();
	SwitchPlatformExit();
	std::_Exit(status);
}
