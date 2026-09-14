#pragma once

#include "SwitchLaunchPolicy.h"
#include <switch.h>

inline unsigned SwitchAddressSpaceBits()
{
	u64 base = 0, size = 0;
	if (R_FAILED(svcGetInfo(&base, InfoType_AslrRegionAddress, CUR_PROCESS_HANDLE, 0)) ||
		R_FAILED(svcGetInfo(&size, InfoType_AslrRegionSize, CUR_PROCESS_HANDLE, 0)))
		return 0;
	return SwitchLaunch::AddressSpaceBits(base, size);
}
