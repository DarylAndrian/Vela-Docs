; ════════════════════════════════════════════════════════════════════════════
; Vela Docs — NSIS Installer Script
; ════════════════════════════════════════════════════════════════════════════
; Run with:
;   makensis vela-docs.nsi
; Produces:
;   VelaDocs-Setup-<version>.exe
; ════════════════════════════════════════════════════════════════════════════

Unicode True
SetCompressor /SOLID lzma

; ──────────────────────────────────────────────────────────────────────────
; Metadata
; ──────────────────────────────────────────────────────────────────────────

!define APPNAME      "Vela Docs"
!define COMPANYNAME  "Vela Docs Contributors"
!define DESCRIPTION  "A modern word processor combining MS Word + Google Docs"
!define VERSIONMAJOR 0
!define VERSIONMINOR 5
!define VERSIONBUILD 4
!define VERSION "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define CAPABILITIES_KEY "Software\Vela Docs\Capabilities"
!define DOCX_PROGID "VelaDocs.Document"
!define APPLICATION_KEY "Software\Classes\Applications\vela-docs.exe"
!define HELPURL      "https://github.com/your-org/vela-docs"
!define UPDATEURL    "https://github.com/your-org/vela-docs/releases"
!define ABOUTURL     "https://github.com/your-org/vela-docs"

!define INSTALLSIZE  150000  ; ~150 MB estimate (Phase 0 is much smaller)

; ──────────────────────────────────────────────────────────────────────────
; Includes & macros
; ──────────────────────────────────────────────────────────────────────────

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

; ──────────────────────────────────────────────────────────────────────────
; Installer UI
; ──────────────────────────────────────────────────────────────────────────

Name "${APPNAME} ${VERSION}"
!ifdef VELA_PER_USER
!define INSTALL_REGISTRY HKCU
OutFile "VelaDocs-Setup-${VERSION}-User.exe"
InstallDir "$LOCALAPPDATA\Programs\Vela Docs"
RequestExecutionLevel user
!else
!define INSTALL_REGISTRY HKLM
OutFile "VelaDocs-Setup-${VERSION}.exe"
InstallDir "$PROGRAMFILES64\Vela Docs"
RequestExecutionLevel admin
!endif
InstallDirRegKey ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" "InstallLocation"
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "CompanyName" "${COMPANYNAME}"
VIAddVersionKey "FileDescription" "${APPNAME} Setup"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "Copyright Vela Docs Contributors"
VIAddVersionKey "ProductVersion" "${VERSION}"
ShowInstDetails show
ShowUninstDetails show

!define MUI_ABORTWARNING
!define MUI_ICON   "..\resources\icons\vela-docs.ico"
!define MUI_UNICON "..\resources\icons\vela-docs.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE  "license.rtf"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

; ──────────────────────────────────────────────────────────────────────────
; Installation section
; ──────────────────────────────────────────────────────────────────────────

Section "Vela Docs (required)" SecVelaDocs
    SectionIn RO

    SetOutPath "$INSTDIR"

    ; windeployqt puts the runtime next to vela-docs.exe.
    File /r /x "*.pdb" /x "vela-tests-*.exe" "..\build\bin\Release\*.*"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Start Menu
    CreateDirectory "$SMPROGRAMS\Vela Docs"
    CreateShortcut  "$SMPROGRAMS\Vela Docs\Vela Docs.lnk"            "$INSTDIR\vela-docs.exe"
    CreateShortcut  "$SMPROGRAMS\Vela Docs\Uninstall.lnk"            "$INSTDIR\Uninstall.exe"
    CreateShortcut  "$DESKTOP\Vela Docs.lnk"                         "$INSTDIR\vela-docs.exe"

    ; Registry keys for Add/Remove Programs
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "DisplayName"     "${APPNAME}"
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "InstallLocation" "$INSTDIR"
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "DisplayIcon"     "$INSTDIR\vela-docs.exe,0"
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "Publisher"       "${COMPANYNAME}"
    WriteRegStr ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "DisplayVersion"  "${VERSION}"
    WriteRegDWORD ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "VersionMajor"    "${VERSIONMAJOR}"
    WriteRegDWORD ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "VersionMinor"    "${VERSIONMINOR}"
    WriteRegDWORD ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "NoModify"        1
    WriteRegDWORD ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "NoRepair"        1
    WriteRegDWORD ${INSTALL_REGISTRY} "${UNINSTALL_KEY}" \
                     "EstimatedSize"   "${INSTALLSIZE}"

    ; Offer Vela Docs in Open with and Default apps for this installation scope.
    SetRegView 64
    WriteRegStr ${INSTALL_REGISTRY} "Software\Classes\${DOCX_PROGID}" "" "Word Document"
    WriteRegStr ${INSTALL_REGISTRY} "Software\Classes\${DOCX_PROGID}\DefaultIcon" "" "$\"$INSTDIR\vela-docs.exe$\",0"
    WriteRegStr ${INSTALL_REGISTRY} "Software\Classes\${DOCX_PROGID}\shell\open\command" "" "$\"$INSTDIR\vela-docs.exe$\" $\"%1$\""
    WriteRegStr ${INSTALL_REGISTRY} "Software\Classes\.docx\OpenWithProgids" "${DOCX_PROGID}" ""
    WriteRegStr ${INSTALL_REGISTRY} "${APPLICATION_KEY}" "FriendlyAppName" "${APPNAME}"
    WriteRegStr ${INSTALL_REGISTRY} "${APPLICATION_KEY}\SupportedTypes" ".docx" ""
    WriteRegStr ${INSTALL_REGISTRY} "${APPLICATION_KEY}\shell\open\command" "" "$\"$INSTDIR\vela-docs.exe$\" $\"%1$\""
    WriteRegStr ${INSTALL_REGISTRY} "${CAPABILITIES_KEY}" "ApplicationName" "${APPNAME}"
    WriteRegStr ${INSTALL_REGISTRY} "${CAPABILITIES_KEY}" "ApplicationDescription" "${DESCRIPTION}"
    WriteRegStr ${INSTALL_REGISTRY} "${CAPABILITIES_KEY}" "ApplicationIcon" "$\"$INSTDIR\vela-docs.exe$\",0"
    WriteRegStr ${INSTALL_REGISTRY} "${CAPABILITIES_KEY}\FileAssociations" ".docx" "${DOCX_PROGID}"
    WriteRegStr ${INSTALL_REGISTRY} "Software\RegisteredApplications" "${APPNAME}" "${CAPABILITIES_KEY}"
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'

SectionEnd

; ──────────────────────────────────────────────────────────────────────────
; Uninstaller
; ──────────────────────────────────────────────────────────────────────────

Section "Uninstall"
    ; Remove install dir
    RMDir /r "$INSTDIR"

    ; Start menu + desktop shortcuts
    Delete "$SMPROGRAMS\Vela Docs\Vela Docs.lnk"
    Delete "$SMPROGRAMS\Vela Docs\Uninstall.lnk"
    Delete "$DESKTOP\Vela Docs.lnk"
    RMDir  "$SMPROGRAMS\Vela Docs"

    ; Registry
    DeleteRegKey ${INSTALL_REGISTRY} "${UNINSTALL_KEY}"
    SetRegView 64
    DeleteRegValue ${INSTALL_REGISTRY} "Software\RegisteredApplications" "${APPNAME}"
    DeleteRegKey ${INSTALL_REGISTRY} "${CAPABILITIES_KEY}"
    DeleteRegKey ${INSTALL_REGISTRY} "${APPLICATION_KEY}"
    DeleteRegValue ${INSTALL_REGISTRY} "Software\Classes\.docx\OpenWithProgids" "${DOCX_PROGID}"
    DeleteRegKey /ifempty ${INSTALL_REGISTRY} "Software\Classes\.docx\OpenWithProgids"
    DeleteRegKey ${INSTALL_REGISTRY} "Software\Classes\${DOCX_PROGID}"
!ifndef VELA_PER_USER
    ; Older machine-wide installers set this shared default value.
    ReadRegStr $0 HKLM "Software\Classes\.docx" ""
    ${If} $0 == "${DOCX_PROGID}"
        DeleteRegValue HKLM "Software\Classes\.docx" ""
    ${EndIf}
!endif
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd
