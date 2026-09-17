cmake_minimum_required(VERSION 3.22)

project(Thakur
    VERSION 1.5.0
    DESCRIPTION "Thakur Executor - Qt6 Native UI"
    HOMEPAGE_URL "https://github.com/phantom0001-coder/Thakur"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/ui"
)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)

qt_standard_project_setup()

set(THAKUR_SOURCES
    thakur_ui/src/main_qt.cpp
    thakur_ui/src/mainwindow.cpp
    thakur_ui/src/splashscreen.cpp
    thakur_ui/src/script_hub.cpp
    thakur_ui/src/ThemeManager.cpp
    thakur_ui/src/EditorWidget.cpp
    thakur_ui/src/ipc/ipc_client.cpp
    thakur_ui/src/hub/HubManager.cpp
    thakur_ui/src/premium/PremiumManager.cpp
    thakur_ui/src/updater/Updater.cpp
    thakur_ui/src/credits.cpp
    thakur_ui/src/plans.cpp
    thakur_ui/src/settings.cpp
    thakur_ui/src/script_editor.cpp
)

set(THAKUR_HEADERS
    thakur_ui/src/mainwindow.h
    thakur_ui/src/splashscreen.h
    thakur_ui/src/script_hub.h
    thakur_ui/src/ThemeManager.h
    thakur_ui/src/EditorWidget.h
    thakur_ui/src/ipc/ipc_client.h
    thakur_ui/src/hub/HubManager.h
    thakur_ui/src/premium/PremiumManager.h
    thakur_ui/src/updater/Updater.h
    thakur_ui/src/credits.h
    thakur_ui/src/plans.h
    thakur_ui/src/settings.h
    thakur_ui/src/script_editor.h
)

set(THAKUR_RESOURCES "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/resources.qrc")
    list(APPEND THAKUR_RESOURCES thakur_ui/resources.qrc)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/resources/resources.qrc")
    list(APPEND THAKUR_RESOURCES thakur_ui/resources/resources.qrc)
endif()

qt_add_executable(thakur
    ${THAKUR_SOURCES}
    ${THAKUR_HEADERS}
    ${THAKUR_RESOURCES}
)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/resources/thakur.rc")
    target_sources(thakur PRIVATE thakur_ui/resources/thakur.rc)
endif()

set_target_properties(thakur PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "thakur"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(thakur PRIVATE
    THAKUR_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    THAKUR_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    THAKUR_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    THAKUR_VERSION_STRING="${PROJECT_VERSION}"
    QT_USE_QSTRINGBUILDER
    QT_NO_DEPRECATED_WARNINGS
)

target_include_directories(thakur PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src/ipc"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src/hub"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src/premium"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src/updater"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/ui"
)

target_link_libraries(thakur PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    user32
    shell32
    dwmapi
    wininet
    crypt32
    iphlpapi
)

if(MSVC)
    target_compile_options(thakur PRIVATE
        /W4
        /wd4100
        /wd4127
        /wd4251
        /permissive-
        /Zc:__cplusplus
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(thakur PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
    )
endif()

if(WIN32)
    get_filename_component(_qt6_cmake_dir "${Qt6_DIR}" ABSOLUTE)
    get_filename_component(_qt6_lib_cmake "${_qt6_cmake_dir}" DIRECTORY)
    get_filename_component(_qt6_lib       "${_qt6_lib_cmake}" DIRECTORY)
    get_filename_component(_qt6_root      "${_qt6_lib}"       DIRECTORY)
    set(_qt6_bin "${_qt6_root}/bin")

    find_program(WINDEPLOYQT6_EXECUTABLE
        NAMES windeployqt6 windeployqt
        HINTS "${_qt6_bin}"
        NO_DEFAULT_PATH
    )

    if(WINDEPLOYQT6_EXECUTABLE)
        message(STATUS "Found windeployqt6: ${WINDEPLOYQT6_EXECUTABLE}")
        add_custom_command(TARGET thakur POST_BUILD
            COMMAND "${WINDEPLOYQT6_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --compiler-runtime
                "$<TARGET_FILE:thakur>"
            COMMENT "Running windeployqt6 on thakur.exe..."
            VERBATIM
        )
    else()
        message(WARNING "windeployqt6 not found — skipping deployment step.")
    endif()
endif()

install(TARGETS thakur
    RUNTIME DESTINATION bin
    BUNDLE  DESTINATION .
)

enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

message(STATUS "=== Thakur ${PROJECT_VERSION} ===")
message(STATUS "  Qt version   : ${Qt6_VERSION}")
message(STATUS "  Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output dir   : ${CMAKE_BINARY_DIR}/bin")