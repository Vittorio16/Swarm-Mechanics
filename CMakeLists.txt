cmake_minimum_required(VERSION 3.10)
project(SwarmEvolution)

set(CMAKE_CXX_STANDARD 17)

# Trova SFML (Graphics, Window, System)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Includi la cartella header
include_directories(include)

# Trova tutti i file .cpp dentro src e sottocartelle
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Crea l'eseguibile
add_executable(SwarmSim ${SOURCES})

# Linka SFML all'eseguibile
target_link_libraries(SwarmSim sfml-graphics sfml-window sfml-system)