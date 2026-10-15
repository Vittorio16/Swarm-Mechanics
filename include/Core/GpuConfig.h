#pragma once
#include <string>

namespace GpuConfig {
    extern int persistentGrid;
    extern int smCount;
    extern int clockKHz;
    extern std::string deviceName;
    extern int concurrentManagedAccess;

    void init(int gridMultiplier = 8);
}