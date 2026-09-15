#pragma once

#include <algorithm>
#include <cstdint>

namespace MemMapper::SwitchGuestMemoryLayout
{
	struct Range { uintptr_t begin, end; };
	constexpr uintptr_t Mem2Begin = 0x10000000;
	constexpr uintptr_t Mem2End = 0x50000000;
	constexpr uintptr_t GuestSize = 0x100000000ull;

	constexpr uintptr_t MainStackGap = 0x00400000;
	constexpr uintptr_t CodeEnd = Mem2Begin - MainStackGap;

	constexpr Range MappedRanges[] = {
		{0, CodeEnd},
		{Mem2Begin, Mem2End},
		{0xA0000000, 0xBC000000},
		{0xE0000000, 0xEA000000},
		{0xF4000000, 0xFA000000},
		{0xFFC00000, GuestSize},
	};

	template<typename Query>
	uintptr_t FindBase(Range stack, Range aslr, Query query,
	                   size_t* blockedRange = nullptr, uintptr_t* blockedAt = nullptr)
	{
		if (stack.end <= stack.begin || aslr.end <= aslr.begin ||
			stack.end < Mem2End || stack.end - stack.begin < Mem2End - Mem2Begin ||
			aslr.end - aslr.begin < GuestSize)
			return 0;
		uintptr_t base = std::max(aslr.begin, stack.begin > Mem2Begin ? stack.begin - Mem2Begin : 0);
		const uintptr_t last = std::min(aslr.end - GuestSize, stack.end >= Mem2End ? stack.end - Mem2End : 0);
		while (base <= last)
		{
			uintptr_t nextBase = base;
			size_t rangeIndex = 0;
			for (const Range& range : MappedRanges)
			{
				for (uintptr_t address = base + range.begin; address < base + range.end;)
				{
					uintptr_t end = 0;
					bool free = false;
					if (!query(address, end, free) || end <= address)
						return 0;
					if (!free)
					{
						if (blockedRange) *blockedRange = rangeIndex;
						if (blockedAt) *blockedAt = address;
						nextBase = end - range.begin;
						break;
					}
					address = end;
				}
				if (nextBase != base)
					break;
				rangeIndex++;
			}
			if (nextBase == base)
				return base;
			base = nextBase;
		}
		return 0;
	}
}
