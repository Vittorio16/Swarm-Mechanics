cmake_minimum_required(VERSION 3.14)

project(SwarmMechanics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
message(STATUS "Scaricando SFML...")

FetchContent_Declare(
  sfml
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG        2.6.1
)

# Questo rende disponibili i target sfml-graphics, sfml-window, ecc.
FetchContent_MakeAvailable(sfml)

include_directories(include)

# Trova tutti i tuoi file sorgente (.cpp) in src/ e sottocartelle
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Crea l'eseguibile
add_executable(SwarmSim ${SOURCES})

# Linka le librerie SFML scaricate all'eseguibile
target_link_libraries(SwarmSim PRIVATE sfml-graphics sfml-window sfml-system)