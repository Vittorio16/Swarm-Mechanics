cmake_minimum_required(VERSION 3.18)
project(SwarmMechanics LANGUAGES CXX CUDA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_ARCHITECTURES native)
# set(CMAKE_CUDA_ARCHITECTURES 87)

option(BUILD_GUI "Build with SFML GUI" OFF)

if(BUILD_GUI)
    include(FetchContent)
    message(STATUS "Fetching SFML...")
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
    )
    FetchContent_MakeAvailable(sfml)
endif()

find_package(CUDAToolkit REQUIRED)
include_directories(include)

# CONFIGURE_DEPENDS makes CMake re-glob when you add a source file.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.cu")

add_executable(SwarmSim ${SOURCES})

# Routes the CUDA runtime API to per-thread default streams
target_compile_definitions(SwarmSim PRIVATE CUDA_API_PER_THREAD_DEFAULT_STREAM)

# Routes <<<grid, block>>> launches to the same place.
target_compile_options(SwarmSim PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--default-stream=per-thread>)

if(BUILD_GUI)
    target_compile_definitions(SwarmSim PRIVATE USE_SFML_GUI)
    target_link_libraries(SwarmSim PRIVATE sfml-graphics sfml-window sfml-system)
endif()