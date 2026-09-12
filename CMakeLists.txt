 cmake_minimum_required(VERSION 3.18)


project(SwarmMechanics  LANGUAGES CXX CUDA)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES native)

include(FetchContent)
message(STATUS "Scaricando SFML...")


FetchContent_Declare(

    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1

)

FetchContent_MakeAvailable(sfml)

find_package(CUDAToolkit REQUIRED)

include_directories(include)


# Trova tutti i tuoi file sorgente
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cu")


add_executable(SwarmSim ${SOURCES})

# Allocates different streams for each thread
target_compile_definitions(SwarmSim PRIVATE CUDA_API_PER_THREAD_DEFAULT_STREAM)

# Reroutes <<<grid, block>>> kernel launches to appropriate streams
target_compile_options(SwarmSim PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--default-stream=per-thread>
)

target_link_libraries(SwarmSim PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system
    CUDA::curand
) 