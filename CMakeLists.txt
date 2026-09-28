cmake_minimum_required(VERSION 3.16)
project(myapp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to an optimized build (1 billion spins is very slow in Debug)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# ── OpenXLSX (replaces Aspose.Cells) ─────────────────────────────
include(FetchContent)
set(OPENXLSX_CREATE_DOCS    OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_SAMPLES  OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    OpenXLSX
    GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
    GIT_TAG        master      # pin to a commit hash once the build works
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(OpenXLSX)

find_package(Threads REQUIRED)

# ── App ──────────────────────────────────────────────────────────
add_executable(myapp src/main.cpp)

target_include_directories(myapp PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Constants::excelFilePath = PROJECT_DIR + "Burning Classic.xlsx"
target_compile_definitions(myapp PRIVATE PROJECT_DIR="${CMAKE_SOURCE_DIR}/")

target_compile_options(myapp PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(myapp PRIVATE OpenXLSX::OpenXLSX Threads::Threads)