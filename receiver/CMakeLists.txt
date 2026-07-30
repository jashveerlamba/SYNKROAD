cmake_minimum_required(VERSION 3.16)
project(SYNKROADReceiver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prevent Windows header macro collisions globally across all 22 steps
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(SYNKROADReceiver ${SOURCES})

# Link required Windows libraries
target_link_libraries(SYNKROADReceiver PRIVATE ws2_32 user32 gdi32 comctl32)