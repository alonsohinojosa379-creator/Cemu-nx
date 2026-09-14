#pragma once

#include <cstdint>


// Arms the next frame. Safe from any thread.
void SwitchGpuCapture_Request();
bool SwitchGpuCapture_IsCapturing();

void SwitchGpuCapture_BeginFrame();
void SwitchGpuCapture_NoteDraw(uint32_t count, uint32_t instanceCount, uint32_t baseVertex, bool isIndexed);
void SwitchGpuCapture_EndFrame();
