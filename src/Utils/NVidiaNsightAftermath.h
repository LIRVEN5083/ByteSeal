#pragma once
#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include <iostream>

extern "C" void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);