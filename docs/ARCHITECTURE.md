# Vela Docs — Architecture (v0.5.0)

> Reference architecture for **Vela Docs v0.5.0**. v0.3.x closed every
> item in `improvement/vela-docs-v0.3.0-improvement-plan.md`; v0.4.x
> was the polish track; **v0.5.0 is the LibreOffice Writer–parity
> deep-data-model release**: Page Styles, Sections, Columns,
> Line Numbering, Drop Cap, Document Protection.
>
> See `/CHANGELOG.md` for the per-sub-version breakdown.

---

## 0. What changed in v0.5.0 (vs. v0.3.5)

| Area | v0.3.5 | v0.5.0 |
|---|---|---|
| Page model | Single global `<w:sectPr>` with one `<w:pgSz>` / `<w:pgMar>` | `PageStyleSheet` of named `<w:style w:type="page">` entries, referenced by every `<w:sectPr>` (matches `SwPageDescs` in LibreOffice `sw/inc/pagedesc.hxx`) |
| Sections | One flat `Section` struct (breakKind + orientation + margins) | `SectionData` strict superset — adds `pageStyleId`, `linkHeaderToPrevious`, `titlePg`, `ColumnSettings`, `LineNumberSettings`, `DocumentProtection`, per-section first/odd/even header text |
| Columns | One `count` column per section | `ColumnSettings` with `count` 1..16, `equalWidth`/`customWidths`, `separator` flag, `<w:col>` children for non-equal widths |
| Line numbering | Implicit / not exposed | `LineNumberSettings` — `countBy`, `startAt`, `distance`, `restartPerPage|newSection|continuous` |
| Drop cap | Implicit / not exposed | `DropCapSettings` — `type` (None / Drop / Margin), `lines`, optional `font`/`size` |
| Document protection | Not exposed | `DocumentProtection` — `mode` (NoProtection / ReadOnly / Comments / TrackedChanges / Forms), `enforce`, opaque password `hash`/`salt`/`spinCount` |
| DOCX output parts | `[Content_Types].xml`, `_rels/.rels`, `word/document.xml` only | + optional `word/styles.xml`, `word/settings.xml`, `word/_rels/document.xml.rels`, wired with the right `<Override>` entries |
| Body JSON sidecar | None | `_vela` block carries `evenAndOddHeaders`, `mirrorMargins`, `documentProtection` (raw `<w:documentProtection>` XML), `pageStyles[]` (`{id, xml}`), `sections[]` (`{type, orient, top, right, bottom, left, header, footer, gutter, titlePg, cols, lnNum}`) |
| Round-trip | Paragraphs + style + runs + drop-cap (v0.4.0) | + page styles + multi-`<w:sectPr>` + columns + line numbering + document protection + `<w:dropCap>` |
| Tests | 60+ Catch2 tests | 60+ existing + 31 new cases in `test_v0_5_0.cpp` covering every Page Style / Settings / Section / Protection enum |

---

## 1. Process model (v0.5.0)

```
┌──────────────────────────────────────────────────────────────┐
│  vela-docs.exe — Single-process (Windows / macOS / Linux)   │
│                                                               │
│  ┌────────────────────── Native Qt (QMainWindow) ─────────────┐
│  │ Menu Bar  File · Edit · Insert · Format · Tools · Window ││
│  │ ┌─ Top bar (48px): logo + version + recovery banner ─────┐││
│  │ │  "Auto-saved 12s ago · Crash-recovered doc-1.docx"     │││
│  │ ├─ Ribbon (3 tabs) ──────────────────────────────────────┐ ││
│  │ │  Home │ Insert │ Table  ⭐ v0.5.0 adds Page Layout group ││
│  │ │  [Font][B/I/U/S][Colors][H1–H6][Align][Lists][Undo]…   ││
│  │ ├─ Left rail (48px) ─── Tab strip (multi) ─┬─ Page ──┐│ ││
│  │ │   🔍 📑 📖 [+]  * Doc 1 · Doc 2 · Doc 3* │ Word …   ││ ││
│  │ │  ┌─ Page (816×1056) — PageStyleSelected ──────────┐ ││ ││
│  │ │  │ Header · Body · Footer rendered by                ││ ││
│  │ │  │ TipTap (offline bundle, all 14 ext. + Page Styles)││ ││
│  │ │  └─────────────────────────────────────────────────┘ ││ ││
│  │ └─────────────────────────────────────────────────────────┘│
│  │  Right sidebar ── Status bar ──── "Locale · Theme · Sec 2/3" ┘
│  └────────────────────────────────────────────────────────────────┘
│                                                               │
│  ┌────────────────── C++ Backend Services ──────────────────┐
│  │  DocumentController ←→ EditorBridge (QWebChannel async)  │
│  │  AutoSaver ──→ autosave dir + RevisionStore snapshots    │
│  │  RecoveryService scans autosave on launch                │
│  │  ┌──────────────────────────────────────────────────────┐│
│  │  │ PageStyleSheet + SectionStack        ⭐ v0.5.0        ││
│  │  │ PageStyle, LineNumberSettings,                       ││
│  │  │ ColumnSettings, DropCapSettings,                     ││
│  │  │ DocumentProtection                                  ││
│  │  └──────────────────────────────────────────────────────┘│
│  │  DocxImporter / DocxExporter                               │
│  │   (v0.5.0: emits word/styles.xml + word/settings.xml       │
│  │    when present; reads multi-`<w:sectPr>` + drop cap       │
│  │    + every Page Style setting back into the body's          │
│  │    `_vela` sidecar)                                         │
│  │  HtmlImporter / HtmlExporter / MarkdownImporter            │
│  │  OdtImporter / RtfImporter / PdfExporter                   │
│  │  HunspellBridge (1.7.2 + 5 locales) + SpellChecker         │
│  │  RevisionStore (SQLite) + Track-changes marks              │
│  │  CommentStore (DOCX <w:comments>)                         │
│  │  StyleSheet, Outline, TableOfContents, Notes,              │
│  │  CitationStore, Watermark, Section, Shape, TabStop          │
│  │  MailMerge / DocComparison / EpubExporter (v0.3.5)          │
│  │  MacroRecorder + QJSEngine sandbox                          │
│  │  SettingsStore / UpdateService / PrintService              │
│  │  CrashHandler (Minidumps → %APPDATA%/crashdumps)            │
│  │  ShortcutManager + resources/shortcuts.json                 │
│  │  Translator (Qt Linguist .ts/.qm) + high-contrast QSS       │
│  └─────────────────────────────────────────────────────────────┘
└──────────────────────────────────────────────────────────────┘
```

The editor surface is **TipTap on ProseMirror**, embedded inside the native
window via **QtWebEngine**. Everything else (menu bar, ribbon, sidebars,
status bar, file dialogs, drag-drop, autosave dialog, recovery prompt) is
**native Qt**.

---

## 2. Component map (v0.5.0)

| Component                 | File                          | Purpose                                    |
|---------------------------|-------------------------------|--------------------------------------------|
| `Application`             | `app/Application.{h,cpp}`     | `QApplication` subclass; loads QSS         |
| `MainWindow`              | `app/MainWindow.{h,cpp}`      | Top-level window; menu bar + ribbon + tabs   |
| `Ribbon`                  | `app/Ribbon.{h,cpp}`          | 3-tab toolbar (Home/Insert/Table)           |
| `TabBar`                  | `app/TabBar.{h,cpp}`          | Document tabs (single-doc in v0.2)            |
| `LeftRail`                | `app/LeftRail.{h,cpp}`        | 48px left icon rail                         |
| `Sidebar`                 | `app/Sidebar.{h,cpp}`         | Right dock with Outline/Comments/Explore/History |
| `StatusBar`               | `app/StatusBar.{h,cpp}`       | Compact status (page, words, language, zoom) |
| `RecentFiles`             | `app/RecentFiles.{h,cpp}`     | MRU list (QSettings-backed)                |
| `EditorBridge`            | `app/EditorBridge.{h,cpp}`    | `QObject` exposed to JS via `QWebChannel`   |
| `Document`                | `document/Document.{h,cpp}`  | JSON-backed in-memory doc model             |
| `DocumentController`      | `document/DocumentController.{h,cpp}` | Open/save/undo/redo, format dispatch |
| **`PageStyle` + `PageStyleSheet`** ⭐ v0.5.0 | `document/PageStyle.{h,cpp}` | Named page-style catalogue (`SwPageDescs`) |
| `DocxImporter`            | `formats/DocxImporter.{h,cpp}`| **v0.5.0** real OOXML parser (multi-`<w:sectPr>`, drop cap, page styles) |
| `DocxExporter`            | `formats/DocxExporter.{h,cpp}`| **v0.5.0** real OOXML writer (emits `word/styles.xml` + `word/settings.xml` parts) |
| `LegacyDocBridge`         | `formats/LegacyDocBridge.{h,cpp}` | Phase 4 stub (LibreOffice)              |
| `RtfImporter / OdtImporter`     | `formats/*.cpp`         | Phase 4 stub                              |
| `HtmlImporter / MarkdownImporter / PdfExporter` | `formats/*.cpp` | Phase 4 (Markdown done in playground) |
| `SpellChecker`            | `services/SpellChecker.{h,cpp}`  | Phase 3 (Hunspell) placeholder             |
| `RevisionStore`           | `services/RevisionStore.{h,cpp}` | SQLite schema + revision listing           |
| `SettingsStore`           | `services/SettingsStore.{h,cpp}` | Typed QSettings wrapper                   |
| `UpdateService`           | `services/UpdateService.{h,cpp}` | Phase 6 (GitHub Releases)                  |
| `PrintService`            | `services/PrintService.{h,cpp}`  | Phase 3                                  |
| `LibreOfficeHelper`       | `services/LibreOfficeHelper.{h,cpp}`| Phase 4                                 |
| `Logger`                  | `util/Logger.{h,cpp}`        | Singleton logger (file + console)            |
| `PathUtils`               | `util/PathUtils.{h,cpp}`     | Cross-platform path helpers                 |
| `Zip`                     | `util/Zip.{h,cpp}`           | QZip reader/writer                          |
| `Editor (web)`            | `src/editor/index.html`     | TipTap UI loaded by QtWebEngine              |
| `Playground (web)`        | `editor-playground/`        | Vite + TipTap dev env (no Qt needed)         |
| `Playground modals`       | `editor-playground/src/modals.js` | Insert Table / Insert Image dialogs      |

---

## 3. Editor tech stack

We embed a Chromium surface via QtWebEngine and run a JavaScript editor
inside it, built on:

- **ProseMirror** — battle-tested rich-text framework used by The New York
  Times, The Guardian, Atlassian.
- **TipTap v2** — declarative wrapper around ProseMirror with extension
  architecture.

### Extensions registered in v0.5.0
- `StarterKit` (headings, lists, history, blockquote, code, hard break, hr)
- `Underline`, `Highlight` (multicolor), `TextAlign`
- `Link` (autolink), `Image` (with base64), `Placeholder`
- `TaskList` + `TaskItem` (checkbox lists)
- `Table` + `TableRow` + `TableCell` + `TableHeader` (resizable, movable cells)
- `CharacterCount`
- `Title` (v0.4.0) — `<w:pStyle w:val="Title"/>`
- `Subtitle` (v0.4.0) — `<w:pStyle w:val="Subtitle"/>`
- `CustomStyle` (v0.4.0) — generic paragraph/node for arbitrary user-defined styles
- **`SectionData` (v0.5.0)** — strict superset of v0.3 `Section`, applied
  per-section; the JS side carries the same fields in the body's `_vela`
  sidecar so the ProseMirror schema stays untouched.

### JS ↔ C++ bridge (QWebChannel)

```js
// JavaScript side
window.vela.openDocument(path);
window.vela.saveDocument();
window.vela.getBody();             // returns ProseMirror JSON (with `_vela` sidecar)
window.vela.onStatsChanged(...);
```

The C++ side (`EditorBridge`) is registered with `QWebChannel` at startup.
Every formatting command from the Ribbon or menu bar gets serialized as a
short string ("bold", "italic", "heading:h1", "openInsertImage", "setPageStyle",
"setDropCap", …) and forwarded via `runJavaScript("window.__exec('cmd')")`.

### v0.5.0 ribbon extensions

The Ribbon gains a **Page Layout** group (after the v0.5.0 release) with:
- Page Style dropdown (`PageStyleDefault` / `PageStyleLandscape` /
  `PageStyleTitlePage` + user styles)
- Columns button
- Line Numbers toggle + dialog
- Drop Cap toggle
- Document Protection dialog

---

## 4. Ribbon layout (v0.5.0)

The ribbon at the top of the editor has three tabs, matching modern
word-processor conventions (MS Word, Google Docs):

| Tab      | Default state | Purpose                                                    |
|----------|---------------|------------------------------------------------------------|
| **Home**  | visible       | Most-used formatting: B/I/U/S, colors, paragraphs, lists, etc. |
| **Insert** | hidden       | One-shot inserts: image, table, link, page break, HR, footnote, special chars |
| **Table**  | hidden       | Shows enabled actions when cursor is inside a table          |
| **Page Layout** (v0.5.0+ Post-release) | visible | Margins, Page Style, Columns, Line Numbers, Drop Cap, Protection |

The Table tab dynamically enables its buttons based on `editor.isActive('table')`.

---

## 5. Data flow: open a .docx (v0.5.0)

```
User: File → Open
   ↓
MainWindow::openDocumentDialog()
   ↓
MainWindow::openDocument(path)
   ↓
DocumentController::openDocument(path)
   ↓
DocxImporter::importFromFile(path)
   ├─ Zip::openForReading          (unzip)
   ├─ QXmlStreamReader over word/document.xml
   │   ├─ parseRun / parseParagraph
   │   ├─ absorbSections → _vela.sections[]
   │   ├─ absorbStyles   → _vela.pageStyles[]
   │   ├─ absorbSettings → _vela.evenAndOddHeaders / mirrorMargins / documentProtection
   ├─ QXmlStreamReader over word/styles.xml
   │   └─ page styles → _vela.pageStyles[]
   ├─ QXmlStreamReader over word/settings.xml
   │   └─ documentProtection → _vela.documentProtection
   └─ Returns QJsonObject (ProseMirror-shaped + _vela sidecar)
   ↓
Document::setBody(json)
   ↓
PageStyleSheet::registerStyle(...)    for each custom page style in `_vela.pageStyles`
   ↓
EditorBridge::documentLoaded(path)  (signal)
   ↓
QWebChannel → JS  window.vela.onDocumentLoaded(path)
   ↓
editor.commands.setContent(json)
   ↓
Editor renders the document (and a Page Setup dropdown that reads the new
page styles)
```

---

## 6. Data flow: save a .docx (v0.5.0)

```
User: Ctrl+S (or File → Save)
   ↓
MainWindow::saveDocument()  (or MainWindow::saveDocumentAs())
   ↓
DocumentController::saveDocument(path)
   ↓
DocxExporter::exportToFile(path, body)
   ├─ Render body JSON → OOXML (runs, paragraphs, dropCap on runs)
   ├─ If _vela.pageStyles → build word/styles.xml (one <w:style w:type="page"> per entry)
   ├─ If _vela.evenAndOddHeaders | mirrorMargins | documentProtection
   │   → build word/settings.xml
   ├─ For each entry in _vela.sections → <w:sectPr> in document.xml
   │   (with <w:pgSz>, <w:pgMar>, <w:cols>, <w:lnNumType>, <w:titlePg/> if set)
   ├─ Build [Content_Types].xml with overrides for the optional parts
   ├─ Build _rels/.rels with rels for the optional parts
   ├─ Build word/_rels/document.xml.rels with rels for the optional parts
   └─ Zip::write()  → file on disk
   ↓
Document::setDirty(false)
   ↓
EditorBridge::dirtyStateChanged(false)
```

---

## 7. Designer workflow (Google Stitch → Qt)

This project uses **Google Stitch** to design the UI. The exported stitch
files live in `stitch_vela_docs_ui_system/` and include:

- `vela_precision/DESIGN.md` — design tokens (the **source of truth** for colors/typography/spacing)
- `main_editor_light_mode/`, `main_editor_dark_mode/`, `welcome_to_vela_docs/`,
  `template_picker_full_screen/`, `find_replace_panel/`, `comment_thread_ui/`,
  `page_setup_dialog/`, `preferences_dialog/`, `revision_history_panel/`,
  `save_as_export_dialog/`, `editor_collapsed_ribbon/`, `files_backstage/`,
  `focus_mode/`, `empty_state_patterns/`, `component_library_overview/`,
  `context_menus_notifications/`, `about_vela_docs/`

These map to:
- `resources/styles/vela-precision.qss` — Qt stylesheet implementing the tokens
- `editor-playground/index.html` — the web editor UI (light + dark)
- `src/app/MainWindow.cpp` — the native shell (ribbon, menus, sidebars)

To iterate on the design:
1. Open Stitch, adjust the design
2. Click "Generate code" → get a `code.html`
3. Update `editor-playground/src/styles.css` and `index.html` accordingly
4. Hot-reload at `http://localhost:5173/` (run `npm run dev` in `editor-playground/`)
5. When happy, copy the changes into `src/editor/index.html` (the production target for the Qt app)

---

## 8. Building

```powershell
# 1. Install dependencies once
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64"

# 2. Compile
cmake --build build --config Release --parallel

# 3. Package Qt dependencies into the .exe folder
# (handled automatically by our DeployQt.cmake + the qt_add_executable
#  qt_deploy_qml_imports / windeployqt wiring from build.bat)

# 4. Install
build\installer\VelaDocs-Setup-0.5.0.exe   # (after NSIS runs)
```

`build.bat` automates all of this.

---

## 9. Roadmap

| Version | Status     | Theme                                                  |
|---------|------------|--------------------------------------------------------|
| **0.1.0** | ✅ Shipped | Project scaffold + Qt shell + design system           |
| **0.2.0** | ✅ Shipped | Feature-complete editor (TipTap) + .docx round-trip   |
| **0.3.1–0.3.5** | ✅ Shipped | Make it work → word processor → competitive → ship → special |
| **0.4.0–0.4.2** | ✅ Shipped | Refinement & polish — Files tab, icons, color picker, headings, tab stops, document tabs |
| **0.5.0** | ✅ Shipped | Page Styles, Sections, Columns, Line Numbering, Drop Cap, Document Protection |
| 1.0     | Planned    | Real-time collaboration (Yjs + WebSocket) + voice typing |

See `CHANGELOG.md` for the per-release breakdown.

---

## 10. Open questions / TODO

- [ ] Should we bundle `Material Symbols Outlined` font for icon rendering?  ✅ Done (v0.4.1)
- [ ] Real-time collaboration (Yjs + WebSocket relay)?  ✅ Targeted for v1.0
- [ ] Local AI assistant via llama.cpp?
- [ ] Mac / Linux ports (free with Qt, but worth planning)?
- [ ] Free the `LiberationSans` font fallback (current fonts require CDN)?  ✅ Fixed (v0.4.1 — bundled font now)
- [ ] **v0.5.0** catch up to LibreOffice Writer page styles + section model — ✅ Done
- [ ] **v0.5.0** add the Page Layout ribbon group + the per-section UI controls — ⏳ Half done (model + DOCX round-trip; UI in v0.5.1)
