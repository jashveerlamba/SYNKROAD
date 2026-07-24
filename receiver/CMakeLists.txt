cmake_minimum_required(VERSION 3.20)

project(SYNKROADReceiver VERSION 1.0.0 LANGUAGES CXX)

# ------------------------------------------------------------
# C++
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Executable
# ------------------------------------------------------------

add_executable(SYNKROADReceiver WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/UIManager.cpp
)

# ------------------------------------------------------------
# Header Files
# ------------------------------------------------------------

target_include_directories(SYNKROADReceiver
    PRIVATE
        include
)

# ------------------------------------------------------------
# Windows Libraries
# ------------------------------------------------------------

target_link_libraries(SYNKROADReceiver
    PRIVATE
        ws2_32
        user32
        gdi32
)

# ------------------------------------------------------------
# MSVC Settings
# ------------------------------------------------------------

if(MSVC)
    target_compile_options(SYNKROADReceiver PRIVATE
        /W4
        /EHsc
        /permissive-
    )
endif()