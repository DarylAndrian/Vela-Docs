# Building Vela Docs

A complete guide to compiling Vela Docs on **Windows 10/11** *(v0.5.0+)*.

> **First-time setup (v0.5.0+):** the app is fully offline — every asset is
> bundled in the installer. There is **one** network step at build time
> (optional; the file is already in the repo if you cloned it after v0.4.1):
> ```powershell
> python tools\fetch-material-symbols.py
> ```
> Without arguments the script drops the Material Symbols font into
> `resources/fonts/material-symbols-outlined.woff2` and
> `editor-playground/src/assets/material-symbols-outlined.woff2` (≈ 312 KB).
> The script is **idempotent** (skips downloads that already match) and
> falls back gracefully if you have no network.

---

## 1. Prerequisites

| Tool                | Version             | Notes                                      |
|---------------------|---------------------|--------------------------------------------|
| **Qt 6**            | 6.10.3              | Required by CMakeLists.txt (6.10 minimum) |
| **MSVC**            | VS 2022 (17.x)      | Community edition is fine                  |
| **CMake**           | 3.24+               | Bundled with VS Installer? Get a fresh one |
| **Git**             | latest              | For source + submodules                    |
| **NSIS** (optional) | 3.08+               | Only for building the installer            |

### 1.1 Install Visual Studio 2022

Download [Visual Studio Community](https://visualstudio.microsoft.com/downloads/).

In the Visual Studio Installer, select:
- **Desktop development with C++** (gives you MSVC, CMake, Windows SDK)

### 1.2 Install Qt 6

Download the [Qt Online Installer](https://www.qt.io/download-qt-installer).

When prompted, select **Qt 6.10.3** with these components:
- ✅ MSVC 2022 64-bit
- ✅ Qt WebEngine and Qt WebChannel
- ✅ Qt PDF
- ✅ Qt Multimedia
- ✅ Qt SVG
- ✅ Sources (helpful for debugging)

Default install path: `C:\Qt\6.10.3\msvc2022_64`. Note this path.

### 1.3 Install CMake (if not already)

```powershell
winget install Kitware.CMake
# Or download from https://cmake.org/download/
```

Verify:
```powershell
cmake --version    # Should print 3.24+
```

---

## 2. Clone and configure

```powershell
git clone https://github.com/your-org/vela-docs.git
cd vela-docs
```

---

## 3. Configure with CMake

### Option A: Command line

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/6.10.3/msvc2022_64"
```

> ⚠️ Adjust `-DCMAKE_PREFIX_PATH` to match your Qt install directory.
> On Windows the path uses forward slashes.

### Option B: Qt Creator

1. Open Qt Creator → File → Open File or Project…
2. Select `CMakeLists.txt`
3. Choose the **Desktop Qt 6.10.3 MSVC2022 64-bit** kit
4. Click **Configure Project**

---

## 4. Build

### Command line
```powershell
cmake --build build --config Release --parallel
```

The output binary will be at:
```
build/bin/Release/vela-docs.exe
```

### Qt Creator
Click the green ▶ Run button.

---

## 5. Run

```powershell
.\build\bin\Release\vela-docs.exe
```

From the welcome screen, choose **New document** or **Open a file…**.
Each document opens in its own tab. Use **File** for PDF export, preview, and printing.

### Command-line file open
```powershell
.\build\bin\Release\vela-docs.exe path\to\document.docx
```

### Verbose logging
```powershell
.\build\bin\Release\vela-docs.exe --verbose
```
Logs are written to `%APPDATA%\Vela Docs\logs\vela-YYYYMMDD.log`.

---

## 6. Run tests

```powershell
$env:Path = 'C:\Qt\6.10.3\msvc2022_64\bin;' + $env:Path
ctest --test-dir build -C Release --output-on-failure
```

Or directly:
```powershell
.\build\bin\Release\vela-tests-unit.exe
```

---

## 7. Build the installer

Requires [NSIS 3.08+](https://nsis.sourceforge.io/Download).

```powershell
cmake --build build --config Release
& "C:/Program Files (x86)/NSIS/makensis.exe" installer/vela-docs.nsi
```

The installer `VelaDocs-Setup-0.5.4.exe` will be in `installer/`. Pass
`/DVELA_PER_USER` to `makensis` to build `VelaDocs-Setup-0.5.4-User.exe`
for the current user without administrator access.

---

## 8. Troubleshooting

### "Could NOT find Qt6"
Make sure `-DCMAKE_PREFIX_PATH` points to your Qt install (e.g. `C:/Qt/6.10.3/msvc2022_64`).

### LNK2019 / unresolved external symbol
Rebuild with `--clean-first`:
```powershell
cmake --build build --config Release --clean-first
```

### `moc_*.cpp` not found
Qt's MOC isn't running. Ensure `CMAKE_AUTOMOC=ON` (default in our CMakeLists).

### "Permission denied" running the .exe
Windows SmartScreen may block unsigned binaries. Right-click → Properties →
"Unblock", or sign the binary.

### cmark-gfm clone fails
Requires network access to GitHub. If you're behind a proxy, set:
```powershell
$env:HTTPS_PROXY="http://your-proxy:8080"
cmake -S . -B build ...
```

### Want a fully static build?
Not officially supported. Qt is dynamically linked (LGPL requirement).
For a single-`.exe` portable build, use the NSIS installer script with
`/S` flag for silent install.
