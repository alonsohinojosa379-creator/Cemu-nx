#pragma once

#include "util/MemMapper/SwitchGuestMemoryLayout.h"
#include <cstddef>

struct VirtmemReservation;

namespace MemMapper::SwitchGuestMemory
{
	constexpr size_t ReservationCount = sizeof(SwitchGuestMemoryLayout::MappedRanges) / sizeof(SwitchGuestMemoryLayout::Range);

	struct Reservation
	{
		void* base;
		size_t size;
		VirtmemReservation* handles[ReservationCount];
	};

	void ReserveEarly();
	Reservation TakeEarly();
	void DiscardEarly(const char* reason);
	void Release(Reservation& reservation);
	const char* DescribeEarlyReservation();
	void PrintDiagnostics(void (*writeLine)(void*, const char*), void* context);
}
