#pragma once
#include <cstdint>


void SwitchThread_RestoreProcessAffinity();

int SwitchThread_AllowedCoreCount();

// Restrict the calling thread to the three application cores.
void SwitchThread_PinToAppCores(const char* what);

bool SwitchThread_PinToHelperCore(const char* what, int32_t priority);

void SwitchThread_AllowHelperCore(const char* what);

// Writes sdmc:/switch/cemu/thread-fail.txt (errno, memory totals, allocator state) after a
// thread could not be created. Uses only C stdio and syscalls, so it is safe to call when
// the C++ unwinder is not.
void SwitchThread_ReportCreateFailure(const char* what, int errorCode);

// Starts a tiny background thread that appends the process memory totals and the allocator
// state to sdmc:/switch/cemu/memlog.txt every few seconds (the file is rewritten at start).
// It never throws and silently does nothing when the thread cannot be created.
void SwitchThread_StartMemoryMonitor();

// Counters printed in memlog.txt to see whether memory growth follows shader/pipeline compiles.
void SwitchThread_CountShaderModule();
void SwitchThread_CountPipeline();
