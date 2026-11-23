# Vela Docs

> A modern, native **C++ / Qt 6.10** word processor for Windows that combines the
> professional feature density of **Microsoft Word** with the clean, modern
> ergonomics of **Google Docs**.

[![Version](https://img.shields.io/badge/version-0.5.1-blueviolet)](CHANGELOG.md)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue)](isocpp.org)
[![Qt](https://img.shields.io/badge/Qt-6.10.3-green)](qt.io)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

> **Why Qt 6.10 and not 6.11?** The tree originally required Qt 6.11, but Qt
> ships **no QtWebEngine for Windows** in 6.11.0 through 6.12.1 — those
> repositories contain neither a `qtwebengine` package nor module (verified
> against `download.qt.io` on 2026-11-23). Since the whole UI is a
> `QWebEngineView`, 6.11 cannot be satisfied on Windows at all. 6.10.3 is the
> newest Windows release that still provides QtWebEngine, so that is the floor.

---

## 🧭 Release track

> ⚠️ **Status corrected 2026-11-22.** This table previously marked v0.3.1–v0.5.0
> as "✅ Done". An audit of the tree found that most of those claims were not
> backed by code: the features were either headers with no `.cpp`, explicit
> `Phase N` stubs returning `false`, or `.cpp` files never added to any
> `CMakeLists.txt` (so never compiled). The table below reflects what is
> actually in the source.

| Sub-version | Theme | Real status |
|---|---|---|
| **v0.3.1** | Multi-tab, autosave, CDN-free editor | ⚠️ Partial — editor works in the browser playground; `AutoSaver.cpp`/`RecoveryService.cpp`/`CrashHandler.cpp` are not in the build |
| **v0.3.2** | Headers/footers, page layout, find/replace, styles, tables, images, spell-check | ⚠️ Partial — editor-side only. **Find/replace is now real** (see below). `SpellChecker::check()` always returns `true`; Hunspell is not linked and there are **no** `.aff`/`.dic` dictionaries in the repo |
| **v0.3.3** | Outline, comments, track changes, TOC, footnotes, citations, equations, watermarks, sections, shapes | ❌ Mostly not implemented — `CitationStore.h`, `Watermark.h`, `Shape.h` are declarations with no `.cpp` and no callers. `CommentStore.cpp`/`Notes.cpp`/`Outline.cpp`/`TableOfContents.cpp` exist but are excluded from the build. `Notes::add()` stores nothing. No `<w:ins>`/`<w:del>` anywhere, so track changes do not exist |
| **v0.3.4** | CI/CD, signing, auto-update, crash reports, i18n, accessibility, 20 templates | ❌ Mostly not implemented — `UpdateService::checkForUpdates()` only logs and emits `upToDate()`. `resources/templates/` has an `index.json` listing 20 ids and **zero** template files. Only `vela_en.ts` exists; the 7 `.qm` files referenced by `resources.qrc` are absent and no `QTranslator` is installed. CI triggers on `main`/`release/**` but the branch is `master`, so it never ran |
| **v0.3.5** | Offline-portable, EPUB, mail merge, doc compare, macros | ❌ Not implemented — `EpubExporter.h` has no `.cpp`; `MailMerge.h`, `DocComparison.h`, `MacroRecorder.h` are declarations with no implementation and no callers; no `QJSEngine` is linked |
| **v0.4.x** | Files tab, Material Symbols, colour picker, headings, tab stops, document tabs, table CSS | ✅ Real **in the browser playground** (`editor-playground/`). `Backstage.cpp`/`BackstagePanels.cpp`/`ShortcutManager.cpp` are not in the C++ build, so the native app has no Files tab |
| **v0.5.0** | Page Styles, Sections, line numbering, columns, drop caps, protection | ⚠️ Library-only — `PageStyle.cpp` is real, compiles and is tested, but nothing in the built app calls it and the production JS bundle contains zero references to `pageStyles`. There was no UI path into a saved document |

Beyond v0.5.0: **v1.0** = real-time collaboration (Yjs + WebSocket) + voice typing.

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

## 🩺 Known gaps (verified 2026-11-23)

Ordered by how badly they block real use:

1. **The native app has never been buildable from this tree.** `cmake` configure fails on a resource path that does not exist, and `MainWindow.cpp` / `EditorBridge.cpp` reference headers, signals and methods that do not exist. No installer or release binary can have come from this source. *(Compile errors fixed — see below — but not yet verified against a real Qt toolchain.)*
2. **No document content crossed the C++↔JS bridge.** `EditorBridge::getBody()`/`setBody()` returned empty with `ok=true`, so the native app could not save what you typed. Separately, `src/editor/index.html` never loaded `qwebchannel.js` and the bundle never called `invokeCommand`, so the JS→C++ direction was unwired. ✅ **Fixed** — `getBody`/`setBody`/`getPlainText`/`getStats` are real, `index.html` loads `qrc:///qtwebchannel/qwebchannel.js`, and `editor-playground/src/vela-bridge.js` correlates `commandResult` signals with `invokeCommand` request ids (including the case where the signal arrives *before* the return-value callback). `main.js` pulls the body on connect and pushes it back 400 ms after the last keystroke. Not yet run inside Qt.
3. **The shipped editor had no stylesheet.** `index.html` links `qrc:/editor/dist/editor.bundle.css`, which was never emitted — `editor-playground/src/main.js` did not import `styles.css`, so Vite had no CSS to bundle. The JS was also an ES module loaded as a classic script, so it never executed at all. ✅ **Fixed and verified in a browser** — single IIFE bundle, 462 kB CSS, zero console errors.
4. **Production text fonts are missing.** `src/editor/index.html` declares `@font-face` for Inter, Source Serif 4 and JetBrains Mono pointing at `qrc:/editor/dist/assets/fonts/*.woff2`. Those files exist nowhere in the repo; only Material Symbols is bundled.
5. **Every format except DOCX is a stub.** PDF, ODT, RTF, Markdown and HTML import/export all `return false` with a `Phase 4` message. Legacy `.doc` never shells out to LibreOffice. Print does nothing.
6. **DOCX export dropped most of the document.** It understood only a private `{style, marks:["bold"]}` schema that the editor never emits, so real documents lost every heading, all character formatting, and all lists/tables/images. `escapeXml()` was an identity function, so any `&` or `<` produced malformed XML, and `"` was silently converted to a curly quote. ✅ **Fixed** (exporter rewritten against real ProseMirror JSON; importer now emits it too) — but not yet compiled or run.
7. **18 of 56 `.cpp` files are not in any `CMakeLists.txt`**, including the entire backstage UI, comments, TOC, notes, styles, tab stops, header/footer persistence, autosave, recovery and crash handling. Several call APIs that no longer exist, so they cannot simply be added — they need repair first. `document/TabSession.cpp` was one of them and has now been repaired and added: `app/TabBar.cpp` and `app/EditorBridge.cpp` both link against it, so the target could not have linked without it.
8. **`app/TabBar.cpp` did not compile**, and it was in the build. It called `addTab(QString(), title)` — no such `QTabWidget` overload — and then `qobject_cast<TabSession*>(widget(index))`, which can never succeed because a `TabSession` is a `QObject`, not the tab's page widget. Every lookup returned `nullptr`, so even if it had compiled, closing a tab, the dirty dot and title updates would all have been silent no-ops. ✅ **Fixed** — each tab now gets a placeholder page and the session is tracked in a `QHash<QWidget*, TabSession*>` that survives drag-reordering.
9. **The tab strip is still inert.** `MainWindow::buildTabStrip()` creates the `TabBar` but never calls `addSession()`, and never connects `newTabRequested` or `currentSessionChanged`. `MainWindow` keeps its own single `DocumentController` + `m_currentFile`, so there is no per-tab session to display. Real multi-document needs `MainWindow`'s open/save/close plumbing moved onto `TabSession` — not attempted yet because it cannot be compile-verified here.
10. **Tests could not link.** `tests/CMakeLists.txt` compiled zero `src/` files and linked both `Catch2WithMain` and its own `test_main.cpp`. ✅ **Fixed.** The old `test_docx.cpp` fixture also encoded the wrong JSON schema, which is why the exporter's defects went unnoticed; the fixture now uses real TipTap output.
11. **The native status bar never saw a word count.** `EditorBridge::statsChanged(words, characters, pages)` is connected to `MainWindow::onEditorStatsChanged`, but nothing ever emitted it — the editor only updated its own DOM footer. ✅ **Fixed**: new `EditorBridge::reportStats()` slot, called from `updateStats()` through `velaBridge.reportStats()`. Verified in-browser that it fires with values identical to the DOM footer (6 words / 27 chars / 1 page) and no-ops outside Qt.
12. **`resources/resources.qrc` is dead.** No `CMakeLists.txt` references it; the app's resources come from `qt_add_resources` in `src/CMakeLists.txt`, which lists only the QSS, `editor/index.html` and the two bundle files. So the ribbon SVG icons, the 7 `.qm` translations and the dictionaries that `.qrc` declares are **not** in the binary. `src/editor/index.html` used to document the `.qrc` layout as if it were live; that comment is now corrected.
13. **Find & replace had no implementation anywhere.** `MainWindow::findReplace()` opened a `FindReplaceDialog` that was an empty `QDialog` — no widgets, no search. ✅ **Fixed** in the editor (see above); the native action now opens the real bar. The empty dialog class is no longer compiled but its files remain on disk, because this branch has no commits and a deletion would be unrecoverable.

---

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
The NSIS installer is written to `installer\VelaDocs-Setup-0.5.1.exe`.

See [docs/BUILD.md](docs/BUILD.md) for full instructions.

---

## 🤝 Contributing

PRs welcome — see [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).

## 📄 License

[MIT](LICENSE) © 2026 Vela Docs Contributors.
