cmake_minimum_required(VERSION 3.22)

project(Voltro3
    VERSION 1.5.0
    DESCRIPTION "Voltro Executor - Qt6 Native UI with WebView2"
    HOMEPAGE_URL "https://github.com/voltro"
    LANGUAGES CXX
)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt Automation ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# AUTOUIC search paths (where .ui files live)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/ui"
)

# --- Find Qt6 (minimum 6.4) ---
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)

qt_standard_project_setup()

# --- Source Files ---
set(VOLTRO_SOURCES
    voltro_ui/src/main.cpp
    voltro_ui/src/mainwindow.cpp
    voltro_ui/src/splashscreen.cpp
    voltro_ui/src/script_hub.cpp
    voltro_ui/src/ThemeManager.cpp
    voltro_ui/src/EditorWidget.cpp
    voltro_ui/src/ipc/ipc_client.cpp
    voltro_ui/src/hub/HubManager.cpp
    voltro_ui/src/premium/PremiumManager.cpp
    voltro_ui/src/updater/Updater.cpp
    voltro_ui/src/injector.cpp
    voltro_ui/src/injector/VoltroInjector.cpp
    voltro_ui/src/injector/Inj1Injector.cpp
    voltro_ui/src/credits.cpp
    voltro_ui/src/plans.cpp
    voltro_ui/src/settings.cpp
    voltro_ui/src/script_editor.cpp
    voltro_ui/src/memory_scanner.h
    voltro_ui/src/process_manager.h
    voltro_ui/src/security_guard.h
    voltro_ui/src/unc_suite.h
)

set(VOLTRO_HEADERS
    voltro_ui/src/mainwindow.h
    voltro_ui/src/splashscreen.h
    voltro_ui/src/script_hub.h
    voltro_ui/src/ThemeManager.h
    voltro_ui/src/EditorWidget.h
    voltro_ui/src/ipc/ipc_client.h
    voltro_ui/src/hub/HubManager.h
    voltro_ui/src/premium/PremiumManager.h
    voltro_ui/src/updater/Updater.h
    voltro_ui/src/injector.h
    voltro_ui/src/injector/VoltroInjector.h
    voltro_ui/src/injector/Inj1Injector.h
    voltro_ui/src/credits.h
    voltro_ui/src/plans.h
    voltro_ui/src/settings.h
    voltro_ui/src/script_editor.h
)

# --- Resources ---
set(VOLTRO_RESOURCES "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/resources.qrc")
    list(APPEND VOLTRO_RESOURCES voltro_ui/resources.qrc)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/resources/resources.qrc")
    list(APPEND VOLTRO_RESOURCES voltro_ui/resources/resources.qrc)
endif()

# --- Executable ---
qt_add_executable(voltro3 ${VOLTRO_SOURCES} ${VOLTRO_HEADERS} ${VOLTRO_RESOURCES})

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/resources/voltro.rc")
    target_sources(voltro3 PRIVATE voltro_ui/resources/voltro.rc)
endif()

# --- Target Properties ---
set_target_properties(voltro3 PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "voltro3"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(voltro3 PRIVATE
    VOLTRO_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    VOLTRO_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    VOLTRO_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    VOLTRO_VERSION_STRING="${PROJECT_VERSION}"
    QT_USE_QSTRINGBUILDER
    QT_NO_DEPRECATED_WARNINGS
    USE_WEBVIEW2=1
)

# --- Include Directories ---
target_include_directories(voltro3 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src/ipc"
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src/hub"
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src/premium"
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src/updater"
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src/injector"
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/ui"
)

# --- Link Libraries ---
target_link_libraries(voltro3 PRIVATE
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

# --- WebView2 Support ---
find_path(WEBVIEW2_INCLUDE_DIR NAMES webview2.h
    PATHS
    "C:/Program Files (x86)/Microsoft/WebView2"
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.Web.WebView2.1.0.4191.47/build/native/x64"
)

find_library(WEBVIEW2_LIBRARY NAMES WebView2Loader.lib
    PATHS
    "C:/Program Files (x86)/Microsoft/WebView2"
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.Web.WebView2.1.0.4191.47/build/native/x64"
)

if(WEBVIEW2_LIBRARY AND EXISTS "${WEBVIEW2_LIBRARY}")
    target_link_libraries(voltro3 PRIVATE ${WEBVIEW2_LIBRARY})
    target_include_directories(voltro3 PRIVATE ${WEBVIEW2_INCLUDE_DIR})
    message(STATUS "WebView2 library found and linked")
else()
    message(WARNING "WebView2 library not found in standard paths")
endif()

# --- Injected Runtime DLL ---
add_library(voltro_runtime SHARED
    voltro_ui/src/injector/dll/voltro_runtime.cpp
    voltro_ui/src/injector/dll/voltro_runtime.h
    voltro_ui/src/injector/dll/solara_engine.cpp
    voltro_ui/src/injector/dll/solara_engine.h
)

set_target_properties(voltro_runtime PROPERTIES
    OUTPUT_NAME "voltro_runtime"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(voltro_runtime PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/voltro_ui/src/injector/dll"
)

target_link_libraries(voltro_runtime PRIVATE
    user32
    psapi
)

# --- Post-Build: Copy WebView2Loader.dll ---
find_file(WEBVIEW2_DLLSrc NAMES WebView2Loader.dll
    PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.Web.WebView2.1.0.4191.47/runtimes/win-x64/native"
    "C:/Program Files (x86)/Microsoft/WebView2"
)

if(WEBVIEW2_DLLSrc AND EXISTS "${WEBVIEW2_DLLSrc}")
    add_custom_command(TARGET voltro3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WEBVIEW2_DLLSrc}"
            "$<TARGET_FILE_DIR:voltro3>/WebView2Loader.dll"
        COMMENT "Copying WebView2Loader.dll to output directory..."
    )
    message(STATUS "WebView2 DLL will be copied from: ${WEBVIEW2_DLLSrc}")
else()
    message(WARNING "WebView2Loader.dll not found - browser features may not work")
endif()

# --- Post-Build: windeployqt ---
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
        add_custom_command(TARGET voltro3 POST_BUILD
            COMMAND "${WINDEPLOYQT6_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --compiler-runtime
                "$<TARGET_FILE:voltro3>"
            COMMENT "Running windeployqt6 on voltro3.exe..."
            VERBATIM
        )
    else()
        message(WARNING "windeployqt6 not found - skipping deployment step.")
    endif()
endif()

# --- Install Rules ---
install(TARGETS voltro3
    RUNTIME DESTINATION bin
    BUNDLE  DESTINATION .
)

# --- Tests ---
enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

# --- Summary ---
message(STATUS "=== Voltro3 ${PROJECT_VERSION} ===")
message(STATUS "  Qt version   : ${Qt6_VERSION}")
message(STATUS "  Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output dir   : ${CMAKE_BINARY_DIR}/bin")
