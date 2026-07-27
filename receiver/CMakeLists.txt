cmake_minimum_required(VERSION 3.20)

project(SYNKROADReceiver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(SYNKROADReceiver WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/UIManager.cpp
    src/ConfigManager.cpp
    src/NetworkManager.cpp
    src/InputManager.cpp
    src/ControllerState.cpp
    src/InputInjectionManager.cpp
    src/KeyboardMouseBackend.cpp
    src/ui/controls/ToggleSwitch.cpp
)

target_link_libraries(SYNKROADReceiver PRIVATE
    ws2_32
    bcrypt
    comctl32
    user32
)