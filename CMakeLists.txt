cmake_minimum_required(VERSION 3.15)
project(riscv_emu CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use stricter warnings in debug builds
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ------------------ SystemC Setup ------------------
set(SYSTEMC_HOME /usr/local/systemc-2.3.4)   # Change if installed elsewhere
set(SYSTEMC_INCLUDE_DIR ${SYSTEMC_HOME}/include)
set(SYSTEMC_LIBRARY_DIR ${SYSTEMC_HOME}/lib-linux64)

include_directories(${SYSTEMC_INCLUDE_DIR})
link_directories(${SYSTEMC_LIBRARY_DIR})
# ----------------------------------------------------

# Include directory
# include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect all source files recursively under src/
file(GLOB_RECURSE EMU_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# Define the main emulator executable
add_executable(emu ${EMU_SOURCES})

# Link SystemC
target_link_libraries(emu systemc pthread m)
