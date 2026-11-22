@echo off
REM ════════════════════════════════════════════════════════════════════════════
REM Vela Docs — Windows build helper (Qt 6.11.1+)
REM ════════════════════════════════════════════════════════════════════════════

setlocal enabledelayedexpansion

set "PROJECT_ROOT=%~dp0"
set "BUILD_DIR=%PROJECT_ROOT%build"
set "BUILD_TYPE=Release"
set "QT_PATH=C:\Qt\6.11.1\msvc2022_64"

if defined VELA_QT_PATH set "QT_PATH=%VELA_QT_PATH%"

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=build"

if /i "%ACTION%"=="debug" (
    set "ACTION=build"
    set "BUILD_TYPE=Debug"
)
if /i "%ACTION%"=="clean" goto :clean
if /i "%ACTION%"=="run"    goto :run
if /i "%ACTION%"=="test"   goto :test
if /i "%ACTION%"=="installer" goto :installer

REM ─── Verify Qt ─────────────────────────────────────────────────────────────
if not exist "%QT_PATH%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo.
    echo ERROR: Qt 6 not found at: %QT_PATH%
    echo.
    echo Set VELA_QT_PATH environment variable to your Qt install dir.
    echo Example:  set VELA_QT_PATH=C:\Qt\6.11.1\msvc2022_64
    echo.
    exit /b 1
)

REM ─── Locate Visual Studio ──────────────────────────────────────────────────
where cl >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
    ) else (
        echo ERROR: MSVC not found. Install VS 2022 with C++ workload.
        exit /b 1
    )
)

echo.
echo ════════════════════════════════════════════════════════════════════
echo  Vela Docs ^<%BUILD_TYPE%^>  |  Qt %QT_PATH%
echo ════════════════════════════════════════════════════════════════════
echo.

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" ^
      -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
      -DCMAKE_PREFIX_PATH="%QT_PATH%"
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 exit /b 1

echo.
echo ✓ Build succeeded.
echo   Executable: %BUILD_DIR%\bin\%BUILD_TYPE%\vela-docs.exe
echo.

exit /b 0

:clean
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Cleaned.
    exit /b 0

:run
    call "%~dp0build.bat" build || exit /b 1
    "%BUILD_DIR%\bin\%BUILD_TYPE%\vela-docs.exe" %2 %3 %4 %5
    exit /b %errorlevel%

:test
    call "%~dp0build.bat" build || exit /b 1
    cd /d "%BUILD_DIR%"
    ctest -C %BUILD_TYPE% --output-on-failure
    exit /b %errorlevel%

:installer
    call "%~dp0build.bat" build || exit /b 1
    if not exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        echo NSIS not found. Download from https://nsis.sourceforge.io/Download
        exit /b 1
    )
    "C:\Program Files (x86)\NSIS\makensis.exe" "%PROJECT_ROOT%installer\vela-docs.nsi"
    exit /b %errorlevel%