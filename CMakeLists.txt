 cmake_minimum_required(VERSION 3.18)


project(SwarmMechanics  LANGUAGES CXX CUDA)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

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
target_link_libraries(SwarmSim PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system
    CUDA::curand
) 