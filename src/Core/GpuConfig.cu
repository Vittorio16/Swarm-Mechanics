#include "Core/GpuConfig.h"
#include "Core/Config.h"

namespace GpuConfig { int persistentGrid = 256; }

void GpuConfig::init() {
    int device = 0;
    CUDA_CHECK(cudaGetDevice(&device));

    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, device));

    int cma = 0;
    CUDA_CHECK(cudaDeviceGetAttribute(&cma, cudaDevAttrConcurrentManagedAccess, device));
    
    // Enough blocks to fill every SM several times over so the scheduler can
    // hide latency, without paying to launch blocks that do nothing.
    persistentGrid = prop.multiProcessorCount * 8;

    std::cout << "GPU: " << prop.name
              << " | SMs: " << prop.multiProcessorCount
              << " | integrated: " << prop.integrated
              << " | concurrentManagedAccess: " << cma
              << " | persistentGrid: " << persistentGrid << std::endl;
}