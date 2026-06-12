cmake_minimum_required(VERSION 3.14)
project(nbody_final_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Downloaded as a tarball (not a git clone) so Git is not required to build
include(FetchContent)
FetchContent_Declare(
  raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(nbody_c src/nbody_c.c)
add_executable(nbody_sim src/nbody_sim.cpp)
add_executable(nbody_sim_fixed src/nbody_sim_fixed.cpp)
add_executable(solar_system_sim src/SolarSystemSim.cpp)

foreach(prog nbody_c nbody_sim nbody_sim_fixed solar_system_sim)
  target_link_libraries(${prog} PUBLIC raylib)
  # Visual Studio puts executables in build/Debug by default; keep them
  # directly in build/ so the run command is the same on every platform
  set_target_properties(${prog} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}")
endforeach()

# sqrt() in nbody_c.c needs libm on Linux
if(UNIX AND NOT APPLE)
  target_link_libraries(nbody_c PUBLIC m)
endif()
