#include "Core/GpuConfig.h"
#include "Core/Config.h"

namespace GpuConfig {
    int persistentGrid = 256;
    int smCount = 0;
    int clockKHz = 0;
    std::string deviceName;
    int concurrentManagedAccess = 0;
}

void GpuConfig::init(int gridMultiplier) {
    int device = 0;
    CUDA_CHECK(cudaGetDevice(&device));

    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, device));

    CUDA_CHECK(cudaDeviceGetAttribute(&concurrentManagedAccess, cudaDevAttrConcurrentManagedAccess, device));
    
    // Enough blocks to fill every SM several times over so the scheduler can
    // hide latency, without paying to launch blocks that do nothing.
    smCount  = prop.multiProcessorCount;
    CUDA_CHECK(cudaDeviceGetAttribute(&clockKHz, cudaDevAttrClockRate, device));
    persistentGrid = prop.multiProcessorCount * 8;
    deviceName = prop.name;

    if (gridMultiplier < 1) gridMultiplier = 1;
    persistentGrid = gridMultiplier * smCount;

    std::cout << "GPU: " << prop.name
              << " | SMs: " << prop.multiProcessorCount
              << " | integrated: " << prop.integrated
              << " | concurrentManagedAccess: " << concurrentManagedAccess
              << " | persistentGrid: " << persistentGrid << std::endl;
}