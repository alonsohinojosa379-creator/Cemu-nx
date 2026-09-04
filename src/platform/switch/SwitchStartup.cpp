#include "util/MemMapper/SwitchGuestMemory.h"

extern "C" void __real_argvSetup();
extern "C" void __wrap_argvSetup()
{
	__real_argvSetup();
	MemMapper::SwitchGuestMemory::ReserveEarly();
}
