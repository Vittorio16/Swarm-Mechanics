cmake_minimum_required(VERSION 3.18)
project(SwarmMechanics LANGUAGES CXX CUDA)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES native)

option(BUILD_GUI "Build with SFML GUI" ON)

if(BUILD_GUI)
    include(FetchContent)
    message(STATUS "Scaricando SFML...")
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

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cu")

add_executable(SwarmSim ${SOURCES})

target_compile_definitions(SwarmSim PRIVATE CUDA_API_PER_THREAD_DEFAULT_STREAM)
target_compile_options(SwarmSim PRIVATE \(<\):--default-stream=per-thread>)
target_link_libraries(SwarmSim PRIVATE CUDA::curand)

# Only link SFML and set definition if GUI is enabled 
if(BUILD_GUI)
    target_compile_definitions(SwarmSim PRIVATE USE_SFML_GUI)
    target_link_libraries(SwarmSim PRIVATE sfml-graphics sfml-window sfml-system)
endif()