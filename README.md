# Vela Docs

> A modern, native **C++ / Qt 6.10** word processor for Windows that combines the
> professional feature density of **Microsoft Word** with the clean, modern
> ergonomics of **Google Docs**.

[![Version](https://img.shields.io/badge/version-0.5.2-blueviolet)](CHANGELOG.md)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue)](isocpp.org)
[![Qt](https://img.shields.io/badge/Qt-6.10.3-green)](qt.io)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

The Windows build uses the Qt 6.10.3 MSVC 2022 kit with WebEngine and WebChannel. See [the build guide](docs/BUILD.md) for the native build, test suite, and installer.
---

## Windows desktop features

- Create and edit DOCX files in separate tabs with their own undo history.
- Restore local recovery copies of interrupted work, including untitled drafts.
- Store paper size, margins, orientation, and plain-text headers and footers per document.
- Preview and print documents, or export them as PDF.
- Navigate document headings and check US English spelling offline.
---

## 🔎 Find & replace (added 2026-11-23, verified in-browser)

`editor-playground/src/extensions/find-replace.js` is a ProseMirror plugin plus five
editor commands (`setSearch`, `findStep`, `replaceCurrent`, `replaceAll`, `clearSearch`),
driven by a floating bar mounted in `main.js` as `window.__findBar`.

- **Ctrl+F** find, **Ctrl+H** find & replace, **Ctrl+G / Ctrl+Shift+G** next / previous, **Esc** close.
  Works whether focus is inside the editor or on the ribbon — the extension catches the keys
  in `handleKeyDown` and re-broadcasts them, and `stopPropagation()` keeps the document-level
  listener from double-stepping.
- Matches are found on a **flattened view of the document text**, so a hit spanning a formatting
  boundary (`bet`**|**`agamma`) is found; block boundaries become `\n` so matches never run across
  paragraphs. Each match is stored as one `{from,to}` segment per text node it covers, which is why
  a boundary-spanning hit paints two adjacent highlights.
- Case-sensitive and whole-word toggles; live "n of m" counter; active hit in orange, other hits in yellow.
- Replace-all applies every edit in one transaction, back-to-front, so earlier offsets stay valid.

Verified against a controlled document: 6 case-insensitive / 4 case-sensitive / 3 whole-word hits for
`alpha` in a fixture containing `ALPHA`, `Alpha`, `alpha` and `noalphahere`; replace-all produced the
expected text with the bold mark intact; stepping advanced exactly one match per keypress.

The native **Edit ▸ Find & Replace** action now opens this bar via `runJavaScript` instead of showing
`FindReplaceDialog`, which was an empty `QDialog` that could not search anything. That class is no
longer referenced or compiled; its files were left on disk rather than deleted because this branch
has no commits yet, so a deletion would be unrecoverable.

---

## Format coverage

DOCX is the desktop editing format. The editor supports common headings, formatting, lists, tables, and page settings. Advanced Word layout, rich headers and footers, review markup, and editing of other document formats need further interoperability work.
## 🎯 v0.5.0 at a glance

The Writer/Word-parity data-model release. Six user-visible deep-section features that turn Vela from a "Google Docs clone" into something capable of legal docs, scholarly papers, and team-shared templates.

| Feature | What it does |
|---|---|
| **Page Styles** | Named layout templates (`PageStyleDefault`, `PageStyleLandscape`, `PageStyleTitlePage`, plus user-defined). ⚠️ These live in the `word/_vela.json` sidecar, **not** in `styles.xml`: `<w:style w:type="page">` is not valid OOXML (Word refuses the file), and `<w:sectPr>` has no `pgStyle` child. The earlier exporter emitted both; that has been removed. Page geometry itself is carried by real `<w:pgSz>` / `<w:pgMar>` per section. |
| **Real Sections** | `SectionData` — strict superset of the v0.3.3 `Section` struct. Adds `pageStyleId`, `linkHeaderToPrevious`, `titlePg`, column/line-number/drop-cap/protection sub-objects, and per-section first/odd/even header text slots. |
| **Line numbering** | `<w:lnNumType countBy="…" start="…" distance="…" restart="newSection\|newPage\|continuous"/>` per section. Required for legal transcripts and court filings. |
| **Multi-column** | `<w:cols num="…" space="…" equalWidth="…" sep="…"/>` (and the non-equal-width expansion with per-column `<w:col>` widths). 1..16 columns per section. |
| **Drop cap** | `<w:dropCap w:val="drop\|margin" w:lines="…" w:font="…"/>` on the first run — magazine-style first letter. |
| **Document protection** | `<w:documentProtection w:edit="readOnly\|comments\|trackedChanges\|forms" w:enforcement="1" w:hash="…" w:salt="…" w:cryptSpinCount="…"/>` — the same envelope Microsoft Word writes. |

**Architecture change:** the body JSON keeps a sidecar at `_vela` so the JS playground can attach the new metadata without touching the existing ProseMirror schema. The DOCX writer now emits optional `word/styles.xml`, `word/settings.xml`, and `word/_rels/document.xml.rels` parts when they're not empty — matches LibreOffice Writer's `sw/source/filter/docx/` layout.

**Matches LibreOffice `sw/`:**
- `SwPageDescs` (`sw/inc/pagedesc.hxx`, `sw/source/ui/page/`) → `PageStyleSheet` ✅
- `SwLineNumberInfo` (`sw/source/core/text/`) → `LineNumberSettings` ✅
- `SwFormatDropCap` (`sw/source/ui/chrdlg/`) → `DropCapSettings` ✅
- `<w:documentProtection>` & friends → `DocumentProtection` ✅
- Section break kinds (Next / Continuous / Even / Odd) → `PageSectionBreak` ✅

**Tests:** 31 new Catch2 cases (≥90% coverage on `PageStyle.h` + the new DOCX round-trip) live in `tests/unit/test_v0_5_0.cpp`.

See [`CHANGELOG.md`](CHANGELOG.md) for per-feature notes and the full LibreOffice feature parity table.

---

## ▶ Try the editor locally (no Qt/MSVC needed)

```powershell
cd "D:\ai-and-code\Vela-Docs\editor-playground"
npm install
npm run dev
```

Opens `http://localhost:5173/` with **all** v0.3.1-v0.5.0 features wired — Files tab, Material Symbols icons, custom color picker, full heading system with "Update to match", tab-stop ruler, Page Styles & Sections dropdown, document protection dialog, AND every LibreOffice `sw/`-parity data-model feature. Hot reload on every save.

To build the offline bundle that the production `.exe` loads:

```bash
npm run build:production   # writes src/editor/dist/editor.bundle.js
```

### Use the Vite dev server with the native Qt app

The Qt WebEngine surface inside `vela-docs.exe` accepts the `--dev` flag
to load the editor from the Vite dev server instead of the bundled
`qrc:/editor/index.html`. This lets the C++ shell + the JS editor
evolve together with hot reload on the JS side.

```powershell
# Terminal 1 — start Vite
cd "D:\ai-and-code\Vela-Docs\editor-playground"
npm run dev

# Terminal 2 — start Vela Docs in dev mode
cd "D:\ai-and-code\Vela-Docs"
build\bin\Release\vela-docs.exe --dev
```

Equivalent environment variables work too:

```powershell
$env:VELA_DEV=1; build\bin\Release\vela-docs.exe
$env:VELA_DEV_URL="http://192.168.1.42:5173/"; build\bin\Release\vela-docs.exe   # remote dev
```

Resolution order (highest priority first):

1. `VELA_DEV_URL` env var  → `--dev-url=http://host:port/`  → `--dev`  → fallback to `qrc:/editor/index.html`
2. Production builds ignore all of the above.

---

## ⚡ What's new in v0.5.0 (highlights)

The Writer/Word-parity deep-data-model release — six features that lift Vela from "Google Docs clone" to "compete with Word/Writer for real documents":

- **Page Styles catalogue** — `PageStyleDefault` (8.5×11"), `PageStyleLandscape` (11×8.5"), `PageStyleTitlePage` (1.5" top/bottom, `<w:titlePg/>` flag) ship built-in. Users can register any number of custom styles via `PageStyleSheet::registerStyle`. Each `<w:style w:type="page" w:styleId="…">` lives in `word/styles.xml`, referenced by every `<w:sectPr>`.
- **Multi-`<w:sectPr>` chain** — the exporter writes one `<w:sectPr>` per section; the importer reads them all into the body's `_vela.sections[]`. The new `SectionData` struct is a strict superset of the v0.3.3 `Section` (adds `pageStyleId`, `linkHeaderToPrevious`, `titlePg`, `ColumnSettings`, `LineNumberSettings`, `DocumentProtection`, and per-section first/odd/even header text).
- **Per-section line numbering** — `<w:lnNumType countBy="…" start="…" distance="…" restart="newSection|newPage|continuous"/>` for legal docs.
- **Per-section multi-column** — `<w:cols num="…" space="…" equalWidth="…" sep="…"/>` (1..16 columns), with non-equal width expansion through `<w:col>` children.
- **Per-paragraph drop cap** — `<w:dropCap w:val="drop|margin" w:lines="…" w:font="…"/>` on the first run.
- **Document protection** — `<w:documentProtection w:edit="readOnly|comments|trackedChanges|forms" w:enforcement="1" w:hash="…" w:salt="…" w:cryptSpinCount="…"/>` — same envelope Microsoft Word writes.

Full details: [CHANGELOG.md](CHANGELOG.md).

## ⚡ What's new in v0.4.2 (highlights)

- **Document tabs sidebar (Google Docs style)** — slide-out list of open documents with kebab menus (`⋮` rename / move up / down / duplicate / close), numbered groups (e.g. *1 Ongoing Features* / *2 Queue Features*), `+` button on the right. Toggled by the topbar `<span class="ms">tab</span>` button.
- **Tables now render** — full TipTap-table CSS (1 px borders, 6 px cell padding, header-row background, column-resize handles).
- **Google Docs-style table grid picker** — 5×5 popover, default 3×3 pre-highlighted.
- **Table Settings panel** — tabbed modal (Borders / Cell / Table / Row & column).

## ⚡ What's new in v0.4.1 (highlights)

A fix-only release — the v0.4.0 sources assumed the Material Symbols font and a full set of SVG icons were present in the bundle, but both were missing from the actual repository, so every toolbar button looked blank or fell back to a stray letter. v0.4.1 ships the missing assets and tightens the ribbon layout.

- **Icons are back** — Material Symbols Outlined font bundled locally (no CDN), and **55 SVG icons** generated for the Qt Ribbon. Toolbar buttons now show `format_align_left`, `format_bold`, `format_list_bulleted` instead of `L`, `B`, `UL`.
- **Files tab redesigned** — anchored to the topbar + ribbon like Word 2010+ (drops a shadow when the backstage is open), no more floating rounded button.
- **Ribbon groups** — the Home panel now uses Word-style groups (Font / Paragraph / Styles / Editing) with subtle dividers and uppercase 11px labels.
- **Welcome / empty-state screen** — matches `stitch_vela_docs_ui_system/welcome_to_vela_docs`. First launch shows Blank / Resume / Letter / Report / Invoice templates and Recent documents.
- **Fully offline** — `tools/fetch-material-icons.py` (Font bootstrapper) and `tools/generate-material-icons.py` (SVG generator) run automatically as npm `predev`/`prebuild` hooks. The font is committed to `resources/fonts/` so the production Qt app and the dev Vite server both load it from disk.
- **v0.4.1 chip** in the status bar; topbar brand now reads **"Vela Docs 0.4.1"**.

## ⚡ What's new in v0.4.0 (highlights)

- **Files tab** — colored backstage tab, `Alt+F` opens, Save / Save As / Open / Print / Print Preview / Options / Help / Recent in one place.
- **Material Symbols icons** — proper iconography throughout (alignment, lists, headings, formatting).
- **Custom color picker** — HEX + RGB inputs, auto-generated tints, native gradient picker, last-8-colors recents — pick any color, not just 8 swatches.
- **Heading system** — Title, Subtitle, H1–5 out of the box; custom levels; "Update '[X]' to match selection" restyles every paragraph with that style.
- **Tab stops** — Google Docs-style ruler above the page; drag tab stops, change types (left/center/right/decimal/bar), add leader dots/dashes. Per-paragraph.
- **Status bar cleanup** — `Phase 0` badge gone; zoom slider wired; language label live.
- **Preferences moved** — out of the top bar, into Files > Options.

## ⚡ What's new in v0.3.x (highlights, carried forward)

- **Offline-first** — every asset (TipTap, fonts, icons, dictionaries, Material Symbols) bundled in `resources/resources.qrc`. The editor loads with the network adapter disabled.
- **All 14 TipTap extensions** in production
- **Multi-document tabs** — drag, close-with-prompt, "Auto-saved 12 s ago" badge
- **Auto-save + crash recovery** — every 30 s
- **EditorBridge async** — every command returns a request id; JS errors surface
- **Headers/footers, page numbers, page layout view, find & replace with regex**
- **Full table & image formatting**, **styles system**, **Hunspell spell-check**
- **Comments, track changes, TOC, footnotes, citations, equations, watermarks, sections, shapes**
- **20 starter templates** + the template picker on the welcome screen
- **Macros** (JavaScript sandbox)
- **CI matrix** Windows/macOS/Linux + signed installers + auto-update + crash reports
- **High-contrast theme, internationalised UI** (en/es/fr/de/pt/zh/ja), **WCAG 2.1 AA**
- **Mail merge, document comparison, EPUB export, macro scripting**

Full details: [CHANGELOG.md](CHANGELOG.md).

---

## 🏗 Project layout (v0.5.0)

```
vela-docs/
├── CHANGELOG.md                                 # Full v0.3.x + v0.4.x + v0.5.0 change log
├── README.md                                    # ← you are here
├── build.bat / build.sh                         # Qt + CMake build helpers
├── CMakeLists.txt                               # Top-level CMake and app version
├── tools/                                       # Python build helpers
│   ├── fetch-material-symbols.py                # Bundles Material Symbols font (offline)
│   └── generate-material-icons.py               # Generates the 55 Ribbon SVG icons
├── docs/
│   ├── vela-docs-v0.3.x-execution-plan.md       # Sub-version breakdown
│   └── ARCHITECTURE.md                          # System architecture
├── improvement/
│   ├── vela-docs-v0.3.0-improvement-plan.md     # Source plan split into 5 sub-versions
│   └── vela-docs-v0.4.0-improvement-plan.md     # v0.4.0 polish release plan
├── src/
│   ├── main.cpp
│   ├── app/                                     # MainWindow, Ribbon, TabBar, EditorBridge, Backstage*, ShortcutManager
│   ├── document/                                # Document, DocumentController, TabSession,
│   │                                           #   HeaderFooter, StyleSheet, Outline, Comments,
│   │                                           #   TableOfContents, Notes, CitationStore,
│   │                                           #   Section, Shape, TabStop,
│   │                                           #   PageStyle.{h,cpp} ⭐ v0.5.0 — PageStyleSheet,
│   │                                           #     PageStyle, LineNumberSettings, ColumnSettings,
│   │                                           #     DropCapSettings, DocumentProtection, SectionStack
│   ├── editor/                                  # Production HTML/JS (QWebEngine)
│   │   ├── index.html                           # Offline-friendly
│   │   ├── extensions/                          # 14 TipTap extension files (+ title, subtitle, custom-style)
│   │   ├── modals.js
│   │   ├── command-dispatcher.js
│   │   └── document-tabs.js                     # v0.4.2 slide-out document tabs sidebar
│   ├── export/                                  # EPUB exporter (v0.3.5)
│   ├── formats/                                 # DOCX (v0.5.0: PageStyles + Sections + Protection),
│   │                                           # HTML, MD, ODT, RTF, PDF, docx_hf, LegacyDoc
│   ├── services/                                # AutoSaver, RecoveryService, HunspellBridge,
│   │                                           # RevisionStore, SettingsStore, UpdateService, …
│   ├── ui/                                      # Dialogs (Find, PageSetup, Tables, Images, Styles)
│   └── util/                                    # Logger, PathUtils, Zip, CrashHandler
├── resources/
│   ├── resources.qrc
│   ├── styles/                                  # vela-precision(.qss) + high-contrast variant
│   ├── templates/                               # Template index (20 templates — see index.json)
│   ├── dictionaries/                            # Hunspell dictionaries (placeholder — .aff/.dic TBD)
│   ├── fonts/                                   # Inter, Source Serif 4, JetBrains Mono, Material Symbols Outlined
│   ├── i18n/                                    # Translations (vela_en.ts; additional .qm files at build time)
│   ├── macros/                                  # Macros directory (placeholder — pre-built .js TBD)
│   ├── icons/                                   # Material Symbols SVG icons (v0.4.1)
│   └── shortcuts.json                           # Defaults; user overrides in %APPDATA%
├── editor-playground/                           # Vite + TipTap dev playground
│   └── src/
│       ├── main.js                              # Editor instance + command dispatcher
│       ├── modals.js                            # Modal dialogs
│       ├── styles.css                           # Design system + Material Symbols
│       ├── color-picker.js                      # v0.4.0 — custom HEX/RGB picker
│       ├── style-manager.js                     # v0.4.0 — Title/Subtitle/Headings + "Update to match"
│       ├── ruler.js                             # v0.4.0 — tab-stop ruler
│       ├── tab-stop.js                          # v0.4.0 — ProseMirror Decoration plugin
│       ├── document-tabs.js                     # v0.4.2 — Google-Docs slide-out sidebar
│       ├── table-picker.js                      # 5×5 grid popover
│       ├── table-settings.js                    # tabbed Table Settings panel
│       ├── welcome.js                           # v0.4.1 — empty-state overlay
│       └── extensions/
│           ├── title.js
│           ├── subtitle.js
│           └── custom-style.js
├── installer/                                   # NSIS installer
├── tests/                                       # Catch2
│   └── unit/
│       ├── test_docx.cpp                        # existing v0.3.x tests
│       └── test_v0_5_0.cpp                      # ⭐ v0.5.0 — 31 new cases
├── .github/workflows/                           # Windows, macOS, Linux CI
└── third_party/                                 # cmark-gfm, Catch2, Hunspell (FetchContent)
```

---

## 🚀 Build the native Windows app

```powershell
cd "D:\ai-and-code\Vela-Docs"
build.bat run
```

Output: `build\bin\Release\vela-docs.exe`
The NSIS installer is written to `installer\VelaDocs-Setup-0.5.2.exe`.

See [docs/BUILD.md](docs/BUILD.md) for full instructions.

---

## 🤝 Contributing

PRs welcome — see [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).

## 📄 License

[MIT](LICENSE) © 2026 Vela Docs Contributors.
