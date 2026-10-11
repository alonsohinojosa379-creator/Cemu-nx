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
