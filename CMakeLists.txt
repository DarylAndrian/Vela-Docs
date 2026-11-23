# ════════════════════════════════════════════════════════════════════════════
# Vela Docs — Top-level CMake build script (Qt 6.10+)
# ════════════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.24)
project(VelaDocs
    VERSION 0.5.1
    DESCRIPTION "A modern word processor combining MS Word + Google Docs features"
    LANGUAGES CXX
)

# ──────────────────────────────────────────────────────────────────────────
# Global settings
# ──────────────────────────────────────────────────────────────────────────

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ──────────────────────────────────────────────────────────────────────────
# Qt 6.10 — required
#
# The tree originally demanded Qt 6.11, but Qt ships NO QtWebEngine for
# Windows in 6.11.0-6.12.1 (checked download.qt.io 2026-11-23: those repos
# have no qtwebengine package or module at all). Since the entire UI is a
# QWebEngineView, 6.11 is unsatisfiable on Windows; 6.10.3 is the newest
# Windows build that still provides QtWebEngine, so that is the floor here.
# ──────────────────────────────────────────────────────────────────────────

find_package(Qt6 6.10 REQUIRED COMPONENTS
    Core
    Gui
    GuiPrivate   # QZipReader / QZipWriter (util/Zip.cpp) need the private headers
    Widgets
    Network
    Sql
    PrintSupport
    Svg
    Xml
)

# Optional Qt modules — pulled in only if found. Everything here except the
# WebEngine/WebChannel trio is genuinely optional, so it goes in
# OPTIONAL_COMPONENTS: listing a missing component in COMPONENTS flips
# Qt6_FOUND to false even with QUIET.
find_package(Qt6 6.10 QUIET COMPONENTS
    WebEngineCore
    WebEngineWidgets
    WebChannel
    OPTIONAL_COMPONENTS
    Pdf
    PdfWidgets
    Multimedia
    LinguistTools
    Tools
)

# Define convenience variables
set(VE_QT_MODULES_REQUIRED Core Gui Widgets Network Sql PrintSupport Svg Xml)
set(VE_QT_MODULES_OPTIONAL WebEngineCore WebEngineWidgets WebChannel Pdf PdfWidgets Multimedia)

# Track which optional modules are available
set(VE_HAS_WEBENGINE FALSE)
if(Qt6WebEngineCore_FOUND AND Qt6WebEngineWidgets_FOUND)
    set(VE_HAS_WEBENGINE TRUE)
    message(STATUS "Qt WebEngine: ENABLED — editor will use Chromium canvas")
else()
    message(WARNING "Qt WebEngine NOT FOUND — editor will fall back to a placeholder")
endif()

set(VE_HAS_PDF FALSE)
if(Qt6Pdf_FOUND AND Qt6PdfWidgets_FOUND)
    set(VE_HAS_PDF TRUE)
    message(STATUS "Qt Pdf: ENABLED")
endif()

set(VE_HAS_WEBCHANNEL FALSE)
if(Qt6WebChannel_FOUND)
    set(VE_HAS_WEBCHANNEL TRUE)
endif()

set(VE_HAS_MULTIMEDIA FALSE)
if(Qt6Multimedia_FOUND)
    set(VE_HAS_MULTIMEDIA TRUE)
endif()

# ──────────────────────────────────────────────────────────────────────────
# Third-party (CMake FetchContent)
# ──────────────────────────────────────────────────────────────────────────

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# cmark-gfm — GitHub Flavored Markdown parser.
# Nothing links it yet: formats/MarkdownImporter.cpp is still a stub and never
# calls into cmark, so the default is OFF to avoid a pointless 3rd-party build.
# (The exported targets are libcmark-gfm / libcmark-gfm_static, not
# cmark-gfm::cmark-gfm — the old link lines here could never have resolved.)
option(VE_USE_CMARK_GFM "Use cmark-gfm for Markdown import/export" OFF)
if(VE_USE_CMARK_GFM)
    FetchContent_Declare(
        cmark-gfm
        GIT_REPOSITORY https://github.com/github/cmark-gfm.git
        GIT_TAG        0.29.0.gfm.13
        GIT_SHALLOW    TRUE
    )
    set(CMARK_GFM_SHARED OFF CACHE BOOL "" FORCE)
    set(CMARK_GFM_STATIC ON CACHE BOOL "" FORCE)
    set(CMARK_GFM_TESTS OFF CACHE BOOL "" FORCE)
    set(CMARK_GFM_TOOLS OFF CACHE BOOL "" FORCE)
    set(CMARK_GFM_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cmark-gfm)
    message(STATUS "cmark-gfm: ENABLED")
endif()

# Catch2 — unit testing
option(VE_BUILD_TESTS "Build unit and integration tests" ON)
set(CATCH2_VERSION 3.5.0)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)

# ──────────────────────────────────────────────────────────────────────────
# Helper scripts
# ──────────────────────────────────────────────────────────────────────────

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DeployQt)  # windeployqt helper (no-op on non-Windows)

# ──────────────────────────────────────────────────────────────────────────
# Compiler-feature summary
# ──────────────────────────────────────────────────────────────────────────

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "  Vela Docs — Build Configuration")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "  Project version:    ${PROJECT_VERSION}")
message(STATUS "  Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:       ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:           ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Qt version:         ${Qt6_VERSION}")
message(STATUS "  WebEngine:          ${VE_HAS_WEBENGINE}")
message(STATUS "  PDF support:        ${VE_HAS_PDF}")
message(STATUS "  WebChannel:         ${VE_HAS_WEBCHANNEL}")
message(STATUS "  Multimedia:         ${VE_HAS_MULTIMEDIA}")
message(STATUS "  Output dir:         ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "")

# ──────────────────────────────────────────────────────────────────────────
# Subdirectories
# ──────────────────────────────────────────────────────────────────────────

add_subdirectory(src)

if(VE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ──────────────────────────────────────────────────────────────────────────
# Packaging (CPack)
# ──────────────────────────────────────────────────────────────────────────

set(CPACK_PACKAGE_NAME "VelaDocs")
set(CPACK_PACKAGE_VENDOR "Vela Docs Contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern word processor")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_GENERATOR "NSIS;IFW")
set(CPACK_IFW_VERBOSE ON)
set(CPACK_NSIS_DISPLAY_NAME "Vela Docs ${PROJECT_VERSION}")
set(CPACK_NSIS_PACKAGE_NAME "VelaDocs-${PROJECT_VERSION}")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/vela-docs.ico")
set(CPACK_NSIS_MUI_UNIICON "${CPACK_NSIS_MUI_ICON}")
include(CPack)
