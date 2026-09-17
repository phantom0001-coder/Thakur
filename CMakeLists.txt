cmake_minimum_required(VERSION 3.22)

project(Thakur
    VERSION 1.5.0
    DESCRIPTION "Thakur - Native Win32 + Microsoft WebView2"
    HOMEPAGE_URL "https://github.com/phantom0001-coder/Thakur"
    LANGUAGES CXX RC
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "Thakur is a Windows-only application.")
endif()

set(WEBVIEW2_SDK_ROOT "" CACHE PATH "Root of the Microsoft.Web.WebView2 NuGet package")

if(NOT WEBVIEW2_SDK_ROOT)
    file(GLOB WEBVIEW2_PACKAGE_DIRS LIST_DIRECTORIES TRUE
        "${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.Web.WebView2.*")
    if(WEBVIEW2_PACKAGE_DIRS)
        list(SORT WEBVIEW2_PACKAGE_DIRS ORDER DESCENDING)
        list(GET WEBVIEW2_PACKAGE_DIRS 0 WEBVIEW2_SDK_ROOT)
    endif()
endif()

if(NOT WEBVIEW2_SDK_ROOT OR NOT EXISTS "${WEBVIEW2_SDK_ROOT}/build/native/include/WebView2.h")
    message(FATAL_ERROR
        "Microsoft WebView2 SDK not found. Install the Microsoft.Web.WebView2 NuGet package "
        "into ${CMAKE_CURRENT_SOURCE_DIR}/packages or set -DWEBVIEW2_SDK_ROOT=<package-root>."
    )
endif()

set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_SDK_ROOT}/build/native/include")
set(WEBVIEW2_LIB_DIR "${WEBVIEW2_SDK_ROOT}/build/native/x64")
set(WEBVIEW2_LOADER_LIB "${WEBVIEW2_LIB_DIR}/WebView2Loader.dll.lib")
set(WEBVIEW2_LOADER_DLL "${WEBVIEW2_LIB_DIR}/WebView2Loader.dll")

foreach(_path IN ITEMS "${WEBVIEW2_INCLUDE_DIR}" "${WEBVIEW2_LOADER_LIB}" "${WEBVIEW2_LOADER_DLL}")
    if(NOT EXISTS "${_path}")
        message(FATAL_ERROR "Required WebView2 SDK file not found: ${_path}")
    endif()
endforeach()

set(THAKUR_SOURCES
    thakur_ui/src/main.cpp
    thakur_ui/src/script_hub.cpp
    thakur_ui/src/ipc/ipc_client.cpp
    thakur_ui/src/injector/ThakurInjector.cpp
    thakur.rc
)

add_executable(thakur WIN32 ${THAKUR_SOURCES})

target_include_directories(thakur PRIVATE
    "${WEBVIEW2_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src/ipc"
    "${CMAKE_CURRENT_SOURCE_DIR}/thakur_ui/src/injector"
)

target_compile_definitions(thakur PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_compile_options(thakur PRIVATE /W4 /EHsc /permissive-)

target_link_libraries(thakur PRIVATE
    "${WEBVIEW2_LOADER_LIB}"
    user32
    gdi32
    shell32
    advapi32
    ole32
    oleaut32
    wininet
    dwmapi
    crypt32
    iphlpapi
)

set_target_properties(thakur PROPERTIES
    OUTPUT_NAME "thakur"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET thakur POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WEBVIEW2_LOADER_DLL}"
        "$<TARGET_FILE_DIR:thakur>/WebView2Loader.dll"
    COMMENT "Staging WebView2Loader.dll"
)

install(TARGETS thakur RUNTIME DESTINATION bin)

message(STATUS "=== Thakur ${PROJECT_VERSION} ===")
message(STATUS "  UI host      : Native Win32 + Microsoft WebView2")
message(STATUS "  WebView2 SDK  : ${WEBVIEW2_SDK_ROOT}")
message(STATUS "  Compiler      : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output dir    : ${CMAKE_BINARY_DIR}/bin")
