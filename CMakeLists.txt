cmake_minimum_required(VERSION 3.16)
project(PixelArtEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find SDL2
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 REQUIRED sdl2)
else()
    find_package(SDL2 REQUIRED)
endif()

add_executable(TinyCanvas
    src/main.cpp
    src/canvas.cpp
    src/editor.cpp
)

target_include_directories(TinyCanvas PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_directories(TinyCanvas PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(TinyCanvas ${SDL2_LIBRARIES})

# macOS specific
if(APPLE)
    target_link_libraries(TinyCanvas "-framework Cocoa")
endif()
