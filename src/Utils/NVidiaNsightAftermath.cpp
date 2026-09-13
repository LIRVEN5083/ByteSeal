#include "NVidiaNsightAftermath.h"


void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData){
    FILE* f = fopen("vk_device_lost_crash.nv-gpudmp", "wb");
    if (f) {
        fwrite(pGpuCrashDump, 1, gpuCrashDumpSize, f);
        fclose(f);
        std::cout << "[Aftermath] GPU Crash Dump saved to vk_device_lost_crash.nv-gpudmp" << std::endl;
    }
}
