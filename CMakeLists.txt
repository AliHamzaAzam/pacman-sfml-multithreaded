cmake_minimum_required(VERSION 3.25)  # FetchContent_Declare(... SYSTEM) needs 3.25+
project(PacMan CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
endif()

# --- SFML via FetchContent, built from source and statically linked. ---
# No SFML pre-install needed on dev machines or CI runners, and the result is
# one self-contained binary (no dylibs/frameworks to ship).
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# POSIX threads (pthreads / named semaphores) — link explicitly for correctness
# rather than relying on libc folding pthread in.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Copy resources next to the binary so the game finds them at runtime.
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

set(SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Audio is unused — link only the modules the game actually needs.
target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::System
    SFML::Window
    SFML::Graphics
    Threads::Threads
)

# macOS frameworks (kept for safety with static SFML windowing).
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
endif()
