#pragma once
#include <cstddef>
#include <cstdint>
#include <string>


// True when the launcher provides the kernel cache operations needed for import.
bool SwitchMemory_CanSynchronizeForGpu();


// Writes back dirty lines so a GPU read that follows sees them.
void SwitchMemory_CleanForGpu(const void* address, size_t size);

// Writes back and drops the lines ahead of a GPU write to the range.
void SwitchMemory_FlushForGpuWrite(const void* address, size_t size);

// Drops stale CPU lines after the GPU fence signals; never writes them back.
void SwitchMemory_InvalidateFromGpu(const void* address, size_t size);

std::string SwitchMemory_DescribeMapping(const void* address);

std::string SwitchMemory_DescribeStackRegion();
