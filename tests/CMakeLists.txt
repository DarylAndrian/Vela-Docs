# ════════════════════════════════════════════════════════════════════════════
# tests/CMakeLists.txt — v0.5.0
# ════════════════════════════════════════════════════════════════════════════

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
    GIT_SHALLOW    TRUE
)
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Catch2)

# ─── Unit tests ─────────────────────────────────────────────────────────────
# The units under test are compiled directly into the test binary. Without these
# the target links against nothing and every DocxExporter / Zip / PageStyle
# symbol is undefined.
add_executable(vela-tests-unit
    unit/test_main.cpp
    unit/test_zip.cpp
    unit/test_docx.cpp
    unit/test_v0_5_0.cpp

    ${CMAKE_SOURCE_DIR}/src/formats/DocxExporter.cpp
    ${CMAKE_SOURCE_DIR}/src/formats/DocxImporter.cpp
    ${CMAKE_SOURCE_DIR}/src/document/PageStyle.cpp
    ${CMAKE_SOURCE_DIR}/src/util/Zip.cpp
    ${CMAKE_SOURCE_DIR}/src/util/Logger.cpp
)
# test_main.cpp supplies its own main(), so link Catch2 without WithMain.
target_link_libraries(vela-tests-unit PRIVATE
    Catch2::Catch2
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate   # QZipReader / QZipWriter used by util/Zip.cpp
    Qt6::Xml
    Qt6::Widgets      # util/Logger.cpp -> app/Application.h -> <QApplication>
)

target_include_directories(vela-tests-unit PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(Catch)
catch_discover_tests(vela-tests-unit)

if(WIN32 AND VE_HAS_WEBENGINE AND VE_HAS_WEBCHANNEL)
    add_executable(vela-tests-desktop desktop/test_document_workflow.cpp ${CMAKE_SOURCE_DIR}/src/vela_assets.qrc)
    target_link_libraries(vela-tests-desktop PRIVATE vela-app)
    add_dependencies(vela-tests-desktop vela-docs)
    add_test(NAME "Desktop document workflow" COMMAND vela-tests-desktop)
    set_tests_properties("Desktop document workflow" PROPERTIES TIMEOUT 30)
endif()
