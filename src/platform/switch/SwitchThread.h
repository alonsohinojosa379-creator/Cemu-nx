#pragma once
#include <cstdint>


void SwitchThread_RestoreProcessAffinity();

int SwitchThread_AllowedCoreCount();

// Restrict the calling thread to the three application cores.
void SwitchThread_PinToAppCores(const char* what);

bool SwitchThread_PinToHelperCore(const char* what, int32_t priority);

void SwitchThread_AllowHelperCore(const char* what);
