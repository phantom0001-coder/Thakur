cmake_minimum_required(VERSION 3.22)

project(ThakurTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Test)

set(TEST_SOURCES
    test_voltro.cpp
    ../thakur_ui/src/ThemeManager.cpp
    ../thakur_ui/src/hub/HubManager.cpp
    ../thakur_ui/src/premium/PremiumManager.cpp
    ../thakur_ui/src/script_hub.cpp
)

set(TEST_HEADERS
    ../thakur_ui/src/ThemeManager.h
    ../thakur_ui/src/hub/HubManager.h
    ../thakur_ui/src/premium/PremiumManager.h
    ../thakur_ui/src/script_hub.h
)

add_executable(thakur_tests
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)

target_include_directories(thakur_tests PRIVATE
    ../thakur_ui/src
    ../thakur_ui/src/hub
    ../thakur_ui/src/premium
    ../thakur_ui/src/updater
    ../thakur_ui/src/ipc
)

target_link_libraries(thakur_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    user32
    wininet
    iphlpapi
)

add_test(NAME ThakurUnitTests COMMAND thakur_tests)
