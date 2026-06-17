cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(bestprogressghost VERSION 1.0.0)

# ─────────────────────────────────────
# Architecture (safe for Windows)
# ─────────────────────────────────────
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# ─────────────────────────────────────
# Source files
# ─────────────────────────────────────
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# ─────────────────────────────────────
# Geode SDK check
# ─────────────────────────────────────
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK not set! Please set it to your Geode SDK path")
endif()

message(STATUS "Found Geode SDK: $ENV{GEODE_SDK}")

# ─────────────────────────────────────
# Add Geode SDK
# ─────────────────────────────────────
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ─────────────────────────────────────
# Geode mod setup
# ─────────────────────────────────────
setup_geode_mod(${PROJECT_NAME})