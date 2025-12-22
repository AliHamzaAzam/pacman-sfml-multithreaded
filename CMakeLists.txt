cmake_minimum_required(VERSION 3.16)
project(PacMan CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Copy resources to build directory
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

# macOS specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
endif()

# Source files
set(SOURCES
    src/main.cpp
)

# Define executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find and link SFML
find_package(SFML 3.0 COMPONENTS System Window Graphics Audio REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::System
    SFML::Window
    SFML::Graphics
    SFML::Audio
)

# macOS frameworks
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
