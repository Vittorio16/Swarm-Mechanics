 cmake_minimum_required(VERSION 3.14)


project(SwarmMechanics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)
message(STATUS "Scaricando SFML...")


FetchContent_Declare(

sfml
GIT_REPOSITORY https://github.com/SFML/SFML.git
GIT_TAG 2.6.1

)

FetchContent_MakeAvailable(sfml)

include_directories(include)


# Trova tutti i tuoi file sorgente
file(GLOB_RECURSE SOURCES "src/*.cpp")


add_executable(SwarmSim ${SOURCES})
target_link_libraries(SwarmSim PRIVATE sfml-graphics sfml-window sfml-system) 