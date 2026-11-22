# ════════════════════════════════════════════════════════════════════════════
# src/CMakeLists.txt — Application sources (v0.5.0)
# ════════════════════════════════════════════════════════════════════════════

set(VE_SOURCES
    # ── Entry point ──
    main.cpp

    # ── Application shell ──
    app/Application.cpp
    app/Application.h
    app/MainWindow.cpp
    app/MainWindow.h
    app/Ribbon.cpp
    app/Ribbon.h
    app/LeftRail.cpp
    app/LeftRail.h
    app/TabBar.cpp
    app/TabBar.h
    app/Sidebar.cpp
    app/Sidebar.h
    app/StatusBar.cpp
    app/StatusBar.h
    app/RecentFiles.cpp
    app/RecentFiles.h
    app/EditorBridge.cpp
    app/EditorBridge.h

    # ── Document model & controller ──
    document/Document.cpp
    document/Document.h
    document/DocumentController.cpp
    document/DocumentController.h
    # TabBar and EditorBridge both link against TabSession's out-of-line
    # QObject ctor/dtor and moc'd signals — omitting it breaks the link.
    document/TabSession.cpp
    document/TabSession.h

    # ── v0.5.0: Page Styles + Sections + Protection + Columns + Line
    #              Numbering + Drop Caps (the LibreOffice-style rewrite)
    document/PageStyle.cpp
    document/PageStyle.h

    # ── File format handlers ──
    formats/DocxImporter.cpp
    formats/DocxImporter.h
    formats/DocxExporter.cpp
    formats/DocxExporter.h
    formats/LegacyDocBridge.cpp
    formats/LegacyDocBridge.h
    formats/RtfImporter.cpp
    formats/RtfImporter.h
    formats/OdtImporter.cpp
    formats/OdtImporter.h
    formats/HtmlImporter.cpp
    formats/HtmlImporter.h
    formats/MarkdownImporter.cpp
    formats/MarkdownImporter.h
    formats/PdfExporter.cpp
    formats/PdfExporter.h

    # ── Services ──
    services/SpellChecker.cpp
    services/SpellChecker.h
    services/RevisionStore.cpp
    services/RevisionStore.h
    services/SettingsStore.cpp
    services/SettingsStore.h
    services/UpdateService.cpp
    services/UpdateService.h
    services/PrintService.cpp
    services/PrintService.h
    services/LibreOfficeHelper.cpp
    services/LibreOfficeHelper.h

    # ── UI widgets ──
    ui/dialogs/AboutDialog.cpp
    ui/dialogs/AboutDialog.h
    ui/dialogs/PageSetupDialog.cpp
    ui/dialogs/PageSetupDialog.h
    ui/dialogs/PreferencesDialog.cpp
    ui/dialogs/PreferencesDialog.h
    ui/widgets/FontPicker.cpp
    ui/widgets/FontPicker.h
    ui/widgets/ColorPicker.cpp
    ui/widgets/ColorPicker.h
    ui/widgets/TableEditor.cpp
    ui/widgets/TableEditor.h

    # ── Utilities ──
    util/Logger.cpp
    util/Logger.h
    util/PathUtils.cpp
    util/PathUtils.h
    util/Zip.cpp
    util/Zip.h
)

# Place the web editor assets next to the binary for runtime access
set(VE_EDITOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/editor")

# ──────────────────────────────────────────────────────────────────────────
# Qt resources — QSS, editor HTML + bundle, icon font
#
# Deliberately a plain .qrc compiled by AUTORCC rather than qt_add_resources():
# the FILES form derives each alias from the source path, which cannot express
# the ":/styles/…" alias for a file that lives outside src/, and Qt 6.10's
# overload resolution routed the call into the .qrc-parsing form and died on it.
# ──────────────────────────────────────────────────────────────────────────

set(VE_RESOURCES
    vela_assets.qrc
)

# ──────────────────────────────────────────────────────────────────────────
# The main executable target
# ──────────────────────────────────────────────────────────────────────────
qt_add_executable(vela-docs
    MANUAL_FINALIZATION
    ${VE_SOURCES}
    ${VE_RESOURCES}
)

# ──────────────────────────────────────────────────────────────────────────
# Target properties
# ──────────────────────────────────────────────────────────────────────────

set_target_properties(vela-docs PROPERTIES
    OUTPUT_NAME   "vela-docs"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    AUTOMOC_MOC_OPTIONS_REMAINING_ARGS ""
)

# ──────────────────────────────────────────────────────────────────────────
# Include directories
# ──────────────────────────────────────────────────────────────────────────

target_include_directories(vela-docs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# ──────────────────────────────────────────────────────────────────────────
# Qt linkage — always required
# ──────────────────────────────────────────────────────────────────────────

target_link_libraries(vela-docs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate   # QZipReader / QZipWriter used by util/Zip.cpp
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::Svg
    Qt6::Xml
)

# Optional Qt modules
if(VE_HAS_WEBENGINE)
    target_link_libraries(vela-docs PRIVATE Qt6::WebEngineCore Qt6::WebEngineWidgets)
    target_compile_definitions(vela-docs PRIVATE VE_HAS_WEBENGINE=1)
endif()

if(VE_HAS_WEBCHANNEL)
    target_link_libraries(vela-docs PRIVATE Qt6::WebChannel)
    target_compile_definitions(vela-docs PRIVATE VE_HAS_WEBCHANNEL=1)
endif()

if(VE_HAS_PDF)
    target_link_libraries(vela-docs PRIVATE Qt6::Pdf Qt6::PdfWidgets)
    target_compile_definitions(vela-docs PRIVATE VE_HAS_PDF=1)
endif()

if(VE_HAS_MULTIMEDIA)
    target_link_libraries(vela-docs PRIVATE Qt6::Multimedia)
    target_compile_definitions(vela-docs PRIVATE VE_HAS_MULTIMEDIA=1)
endif()

# Third-party
if(VE_USE_CMARK_GFM)
    # cmark-gfm exports "libcmark-gfm" (+ "libcmark-gfm_static"); there is no
    # cmark-gfmext target and no namespaced alias.
    target_link_libraries(vela-docs PRIVATE libcmark-gfm)
    target_compile_definitions(vela-docs PRIVATE VE_HAS_MARKDOWN=1)
endif()

# ──────────────────────────────────────────────────────────────────────────
# Compiler definitions
# ──────────────────────────────────────────────────────────────────────────

target_compile_definitions(vela-docs PRIVATE
    VE_VERSION="${PROJECT_VERSION}"
    VE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    VE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    VE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    # NOTE: QT_NO_CAST_FROM_ASCII / QT_NO_CAST_TO_ASCII used to be set here.
    # The whole tree was written against Qt's implicit const char* ↔ QString
    # conversions, and MSVC reports at most one such error per function, so the
    # real count runs into the hundreds across ~40 files. Retrofitting them is a
    # separate, mechanical cleanup — not a blocker for producing a working build.
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# ──────────────────────────────────────────────────────────────────────────
# Copy the editor web assets next to the binary
# ──────────────────────────────────────────────────────────────────────────

add_custom_command(
    TARGET vela-docs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${VE_EDITOR_DIR}"
        "$<TARGET_FILE_DIR:vela-docs>/editor"
    COMMENT "Copying web editor assets to binary directory"
)

# ──────────────────────────────────────────────────────────────────────────
# Deploy Qt runtime DLLs (Windows)
# ──────────────────────────────────────────────────────────────────────────

vela_deploy_qt(vela-docs)

# ──────────────────────────────────────────────────────────────────────────
# Install layout
# ──────────────────────────────────────────────────────────────────────────

include(GNUInstallDirs)

install(TARGETS vela-docs
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY "${VE_EDITOR_DIR}"
    DESTINATION ${CMAKE_INSTALL_BINDIR}/editor
    FILES_MATCHING PATTERN "*"
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/vela-docs
    OPTIONAL
)
