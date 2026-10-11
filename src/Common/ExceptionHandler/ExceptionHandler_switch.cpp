#include "ExceptionHandler.h"

#include <switch.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#define CEMU_STRINGIZE_IMPL(value) #value
#define CEMU_STRINGIZE(value) CEMU_STRINGIZE_IMPL(value)

extern "C" void _start() __attribute__((visibility("hidden")));
extern "C" void __libnx_exception_returnentry(void); // libnx: where the kernel resumes into the handler

namespace
{
	std::atomic_flag s_handlingException = ATOMIC_FLAG_INIT;

	void WriteAll(int fd, const char* data, size_t size)
	{
		while (size != 0)
		{
			const ssize_t written = ::write(fd, data, size);
			if (written > 0)
			{
				data += written;
				size -= static_cast<size_t>(written);
				continue;
			}
			if (written < 0 && errno == EINTR)
				continue;
			return;
		}
	}

	// Mirror of the kernel's ams::svc::aarch64::ExceptionInfo. It lives in the
	// process-local region, which is the first TLS slot of the process, at
	// offset 0x1C0 - 0x78. When svcReturnFromException(0) is called, the kernel
	// restores x0..x8, lr, sp and pc from this structure and resumes the thread.
	struct KernelExceptionInfo
	{
		u64 x[9];
		u64 lr;
		u64 sp;
		u64 pc;
		u32 pstate;
		u32 afsr0;
		u32 afsr1;
		u32 esr;
		u64 far;
	};
	static_assert(sizeof(KernelExceptionInfo) == 0x78);
	static_assert(sizeof(FpuRegister) == 16);

	constexpr uintptr_t kPageSize = 0x1000;
	constexpr uintptr_t kTlsSlotSize = 0x200;
	constexpr uintptr_t kExceptionInfoOffset = 0x148;
	constexpr u32 kMaxRetriesPerSite = 32;
	constexpr int kMaxWaitPolls = 20; // 20 x 1 ms
	constexpr int kMaxReports = 4;

	std::atomic<uintptr_t> s_mainTlsPage{0};
	std::atomic<u64> s_lastRetryPc{0};
	std::atomic<u64> s_lastRetryFar{0};
	std::atomic<u32> s_siteRetries{0};
	std::atomic<u32> s_totalRetries{0};

	bool IsPageUsable(u64 address, bool needWrite)
	{
		MemoryInfo mapping{};
		u32 pageInfo = 0;
		if (R_FAILED(svcQueryMemory(&mapping, &pageInfo, address)))
			return false;
		if (mapping.type == MemType_Unmapped)
			return false;
		const u32 need = needWrite ? (Perm_R | Perm_W) : Perm_R;
		return (mapping.perm & need) == need;
	}

	KernelExceptionInfo* FindExceptionInfo(const ThreadExceptionDump* context)
	{
		const uintptr_t bases[2] = {
			s_mainTlsPage.load(std::memory_order_relaxed),
			reinterpret_cast<uintptr_t>(armGetTls()) & ~(kPageSize - 1)};
		for (const uintptr_t base : bases)
		{
			if (base == 0)
				continue;
			for (int delta = -1; delta <= 1; ++delta)
			{
				const uintptr_t page = base + static_cast<intptr_t>(delta) * static_cast<intptr_t>(kPageSize);
				if (!IsPageUsable(page, true))
					continue;
				for (uintptr_t slot = 0; slot < kPageSize / kTlsSlotSize; ++slot)
				{
					auto* info = reinterpret_cast<KernelExceptionInfo*>(page + slot * kTlsSlotSize + kExceptionInfoOffset);
					// libnx rewrote x0 (-> dump) and pc (-> returnentry) before calling us,
					// so this pair identifies the live exception_info unambiguously.
					if (info->x[0] == reinterpret_cast<u64>(context) &&
						info->pc == reinterpret_cast<u64>(&__libnx_exception_returnentry))
						return info;
				}
			}
		}
		return nullptr;
	}

	void NoteTransientFault(const ThreadExceptionDump* context, u32 waited)
	{
		const u32 total = s_totalRetries.fetch_add(1, std::memory_order_relaxed) + 1;
		if (total > kMaxReports)
			return;
		int fd = ::open("sdmc:/switch/cemu/transient_fault.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
		if (fd < 0)
			return;
		char line[192];
		const int length = std::snprintf(line, sizeof(line),
			"retry #%u pc_off=0x%llx lr_off=0x%llx far=0x%llx esr=0x%08x waited_ms=%u\n", total,
			static_cast<unsigned long long>(context->pc.x - reinterpret_cast<u64>(&_start)),
			static_cast<unsigned long long>(context->lr.x - reinterpret_cast<u64>(&_start)),
			static_cast<unsigned long long>(context->far.x), context->esr, waited);
		if (length > 0)
			WriteAll(fd, line, std::min(static_cast<size_t>(length), sizeof(line) - 1));
		::close(fd);
	}

	// A data abort on a page that is mapped (or becomes mapped) again right away
	// is a transient fault: e.g. the page was momentarily locked/remapped while
	// the GPU driver or another thread was changing it. Wait until the page is
	// accessible and re-execute the faulting instruction. Returns only if the
	// fault could not be resumed.
	void TryResumeTransientDataAbort(ThreadExceptionDump* context)
	{
		if (!threadExceptionIsAArch64(context))
			return;
		const u32 esr = context->esr;
		if ((esr >> 26) != 0x24) // data abort from a lower exception level
			return;
		if ((esr & 0x3F) > 0x0F) // translation, access-flag, permission or address-size faults only
			return;
		const bool needWrite = (esr & 0x40) != 0; // WnR

		// Bound the number of retries for the same instruction/address.
		if (s_lastRetryPc.load(std::memory_order_relaxed) == context->pc.x &&
			s_lastRetryFar.load(std::memory_order_relaxed) == context->far.x)
		{
			if (s_siteRetries.fetch_add(1, std::memory_order_relaxed) >= kMaxRetriesPerSite)
				return;
		}
		else
		{
			s_lastRetryPc.store(context->pc.x, std::memory_order_relaxed);
			s_lastRetryFar.store(context->far.x, std::memory_order_relaxed);
			s_siteRetries.store(0, std::memory_order_relaxed);
		}

		u32 waited = 0;
		while (!IsPageUsable(context->far.x, needWrite))
		{
			if (waited >= kMaxWaitPolls)
				return;
			svcSleepThread(1000000); // 1 ms
			++waited;
		}

		KernelExceptionInfo* info = FindExceptionInfo(context);
		if (info == nullptr)
			return;

		NoteTransientFault(context, waited);

		// Put back what libnx replaced (x0, sp, pc) plus the rest of the frame.
		for (int index = 0; index < 9; ++index)
			info->x[index] = context->cpu_gprs[index].x;
		info->lr = context->lr.x;
		info->sp = context->sp.x;
		info->pc = context->pc.x;

		s_handlingException.clear(std::memory_order_relaxed);

		// This handler ran C++ code that may have clobbered NEON registers the
		// interrupted code was using; reload them from the dump, then resume.
		const FpuRegister* q = context->fpu_gprs;
		asm volatile(
			"ldp q0, q1, [%0, #0]\n"
			"ldp q2, q3, [%0, #32]\n"
			"ldp q4, q5, [%0, #64]\n"
			"ldp q6, q7, [%0, #96]\n"
			"ldp q8, q9, [%0, #128]\n"
			"ldp q10, q11, [%0, #160]\n"
			"ldp q12, q13, [%0, #192]\n"
			"ldp q14, q15, [%0, #224]\n"
			"ldp q16, q17, [%0, #256]\n"
			"ldp q18, q19, [%0, #288]\n"
			"ldp q20, q21, [%0, #320]\n"
			"ldp q22, q23, [%0, #352]\n"
			"ldp q24, q25, [%0, #384]\n"
			"ldp q26, q27, [%0, #416]\n"
			"ldp q28, q29, [%0, #448]\n"
			"ldp q30, q31, [%0, #480]\n"
			:
			: "r"(q)
			: "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13",
			  "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",
			  "v28", "v29", "v30", "v31");
		svcReturnFromException(0);
	}
}

extern "C"
{
	void _start() __attribute__((visibility("hidden")));
	alignas(16) u8 __nx_exception_stack[0x10000];
	u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

	void __libnx_exception_handler(ThreadExceptionDump* context)
	{
		if (s_handlingException.test_and_set(std::memory_order_relaxed))
			return;

		// Does not return when the fault was transient and the thread was resumed.
		TryResumeTransientDataAbort(context);

		int fd = ::open("sdmc:/switch/cemu/crash.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0)
			fd = ::open("crash.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0)
			return;

		char report[4096]{};
		size_t used = 0;
		auto append = [&](const char* format, auto... args) {
			if (used >= sizeof(report))
				return;
			const int length = std::snprintf(report + used, sizeof(report) - used, format, args...);
			if (length <= 0)
				return;
			used += std::min(static_cast<size_t>(length), sizeof(report) - used - 1);
		};

		append("Cemu Switch native exception\n");
#if defined(EMULATOR_HASH)
		append("build: %s\n", CEMU_STRINGIZE(EMULATOR_HASH));
#endif
		append("module_base: 0x%016llx\n", reinterpret_cast<unsigned long long>(&_start));
		append("error_desc: 0x%08x\n", context->error_desc);
		for (int index = 0; index < 29; ++index)
			append("x%-2d: 0x%016llx\n", index,
				static_cast<unsigned long long>(context->cpu_gprs[index].x));
		append("fp: 0x%016llx\n", static_cast<unsigned long long>(context->fp.x));
		append("lr: 0x%016llx\n", static_cast<unsigned long long>(context->lr.x));
		append("sp: 0x%016llx\n", static_cast<unsigned long long>(context->sp.x));
		append("pc: 0x%016llx\n", static_cast<unsigned long long>(context->pc.x));
		append("pstate: 0x%08x\n", context->pstate);
		append("afsr0: 0x%08x\n", context->afsr0);
		append("afsr1: 0x%08x\n", context->afsr1);
		append("esr: 0x%08x\n", context->esr);
		append("far: 0x%016llx\n", static_cast<unsigned long long>(context->far.x));
		append("transient_retries: %u\n", s_totalRetries.load(std::memory_order_relaxed));
		MemoryInfo mapping{};
		u32 pageInfo = 0;
		const Result query = svcQueryMemory(&mapping, &pageInfo, context->far.x);
		if (R_SUCCEEDED(query))
			append("far_mapping: 0x%llx+0x%llx type=0x%x perm=0x%x attr=0x%x device_refs=%u ipc_refs=%u\n",
				static_cast<unsigned long long>(mapping.addr), static_cast<unsigned long long>(mapping.size),
				mapping.type, mapping.perm, mapping.attr, mapping.device_refcount, mapping.ipc_refcount);
		else
			append("far_mapping_query: 0x%08x\n", query);
		WriteAll(fd, report, used);
		::close(fd);
	}
}

void ExceptionHandler_Init()
{
	// The process-local region (holding the kernel exception_info) is in the first
	// TLS page of the process; remember the main thread's page to find it later.
	s_mainTlsPage.store(reinterpret_cast<uintptr_t>(armGetTls()) & ~(kPageSize - 1), std::memory_order_relaxed);
}

#undef CEMU_STRINGIZE
#undef CEMU_STRINGIZE_IMPL
