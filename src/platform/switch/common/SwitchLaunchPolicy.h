#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace SwitchLaunch
{
enum class Error : unsigned
{
	None,
	AddressSpace36,
	AddressSpaceUnsupported,
	AddressSpaceUnknown,
	HostMemoryUnavailable,
	VulkanRequired,
	LaunchFailed,
};

inline constexpr const char* ErrorPath = "sdmc:/switch/cemu/launch-error.txt";

class Failure : public std::runtime_error
{
public:
	Failure(Error error, const std::string& detail) : std::runtime_error(detail), error(error) {}
	const Error error;
};

inline unsigned AddressSpaceBits(uint64_t base, uint64_t size)
{
	if (!size || base > UINT64_MAX - size)
		return 0;
	const uint64_t end = base + size;
	if (end == (uint64_t{1} << 39)) return 39;
	if (end == (uint64_t{1} << 36)) return 36;
	if (end == (uint64_t{1} << 32)) return 32;
	return 0;
}

inline Error AddressSpaceError(unsigned bits)
{
	if (bits == 39) return Error::None;
	if (bits == 36) return Error::AddressSpace36;
	return bits ? Error::AddressSpaceUnsupported : Error::AddressSpaceUnknown;
}

inline bool AllowShortcutAutolaunch(Error pending, Error addressSpace, bool applet)
{
	return pending == Error::None && addressSpace == Error::None && !applet;
}

inline bool WriteError(Error error, const char* path = ErrorPath)
{
	FILE* file = std::fopen(path, "w");
	if (!file) return false;
	const bool written = std::fprintf(file, "CEMU_LAUNCH_ERROR_V1 %u\n", static_cast<unsigned>(error)) > 0;
	const bool closed = std::fclose(file) == 0;
	return written && closed;
}

inline Error TakeError(const char* path = ErrorPath)
{
	FILE* file = std::fopen(path, "r");
	if (!file) return Error::None;
	unsigned value = 0;
	char extra = 0;
	const int fields = std::fscanf(file, "CEMU_LAUNCH_ERROR_V1 %u %c", &value, &extra);
	std::fclose(file);
	std::remove(path);
	return fields == 1 && value > 0 && value <= static_cast<unsigned>(Error::LaunchFailed) ?
		static_cast<Error>(value) : Error::None;
}
}
