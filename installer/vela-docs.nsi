; ════════════════════════════════════════════════════════════════════════════
; Vela Docs — NSIS Installer Script — v0.5.0
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
!define VERSIONBUILD 0
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

Name "${APPNAME} ${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
OutFile "VelaDocs-Setup-${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.exe"
InstallDir "$PROGRAMFILES64\Vela Docs"
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString"
RequestExecutionLevel admin
ShowInstDetails show
ShowUninstDetails show

!define MUI_ABORTWARNING
!define MUI_ICON   "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

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

    ; The Qt-deployed folder (output of windeployqt) lives at:
    ;   build/bin/Release/vela-docs/
    ; Copy everything under it into the install dir.
    File /r "..\build\bin\Release\vela-docs\*.*"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Start Menu
    CreateDirectory "$SMPROGRAMS\Vela Docs"
    CreateShortcut  "$SMPROGRAMS\Vela Docs\Vela Docs.lnk"            "$INSTDIR\vela-docs.exe"
    CreateShortcut  "$SMPROGRAMS\Vela Docs\Uninstall.lnk"            "$INSTDIR\Uninstall.exe"
    CreateShortcut  "$DESKTOP\Vela Docs.lnk"                         "$INSTDIR\vela-docs.exe"

    ; Registry keys for Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "DisplayName"     "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "UninstallString" "$\"INSTDIR\Uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "QuietUninstallString" "$\"INSTDIR\Uninstall.exe$\" /S"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "InstallLocation" "$\"$INSTDIR$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "Publisher"       "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "DisplayVersion"  "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "VersionMajor"    "${VERSIONMAJOR}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "VersionMinor"    "${VERSIONMINOR}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "NoModify"        1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "NoRepair"        1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" \
                     "EstimatedSize"   "${INSTALLSIZE}"

    ; File-type association for .docx (opens with Vela Docs)
    WriteRegStr HKCR ".docx" "" "VelaDocs.Document"
    WriteRegStr HKCR "VelaDocs.Document" "" "Word Document"
    WriteRegStr HKCR "VelaDocs.Document\DefaultIcon" "" "$\"$INSTDIR\vela-docs.exe$\",0"
    WriteRegStr HKCR "VelaDocs.Document\shell\open\command" "" \
                     "$\"$INSTDIR\vela-docs.exe$\" $\"%1$\""

SectionEnd

; ──────────────────────────────────────────────────────────────────────────
; Optional: dictionaries
; ──────────────────────────────────────────────────────────────────────────

Section "Hunspell dictionaries (English, Spanish, French, German)" SecDicts
    SetOutPath "$INSTDIR\dictionaries"
    File /r "..\resources\dictionaries\*.*"
SectionEnd

; ──────────────────────────────────────────────────────────────────────────
; Section descriptions (shown on the components page)
; ──────────────────────────────────────────────────────────────────────────

LangString DESC_SecVelaDocs ${LANG_ENGLISH} \
    "The core Vela Docs application and Qt runtime libraries (required)."

LangString DESC_SecDicts ${LANG_ENGLISH} \
    "Spell-check dictionaries for English, Spanish, French, and German. Skip if you only need basic spell-check."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecVelaDocs} $(DESC_SecVelaDocs)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDicts}   $(DESC_SecDicts)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

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
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    DeleteRegKey HKCR ".docx"
    DeleteRegKey HKCR "VelaDocs.Document"
SectionEnd
