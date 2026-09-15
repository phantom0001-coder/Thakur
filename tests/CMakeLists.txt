cmake_minimum_required(VERSION 3.22)

project(VoltroTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Test)

set(TEST_SOURCES
    test_voltro.cpp
    ../voltro_ui/src/ThemeManager.cpp
    ../voltro_ui/src/hub/HubManager.cpp
    ../voltro_ui/src/premium/PremiumManager.cpp
    ../voltro_ui/src/script_hub.cpp
)

set(TEST_HEADERS
    ../voltro_ui/src/ThemeManager.h
    ../voltro_ui/src/hub/HubManager.h
    ../voltro_ui/src/premium/PremiumManager.h
    ../voltro_ui/src/script_hub.h
)

add_executable(voltro_tests
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)

target_include_directories(voltro_tests PRIVATE
    ../voltro_ui/src
    ../voltro_ui/src/hub
    ../voltro_ui/src/premium
    ../voltro_ui/src/updater
    ../voltro_ui/src/ipc
)

target_link_libraries(voltro_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    user32
    wininet
    iphlpapi
)

add_test(NAME VoltroUnitTests COMMAND voltro_tests)
