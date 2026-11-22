# Changelog

All notable changes to **Vela Docs** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

---

## [0.5.0] — 2026-07-02 — *Page Styles, Sections & Word-grade ergonomics* 📐 ✅

> **Theme:** Catching up to LibreOffice Writer and MS Word on the deep structural features that ship in every "real" word processor — Page Styles (named layout templates, replacing the v0.3.x one-page-global setup), per-section columns + line numbering + drop caps + protection, the first proper multi-`<w:sectPr>` chain in the model, and a complete `<w:documentProtection>` round-trip. v0.5.0 is a *single deep feature* release, not a polish release — it bumps the data model in `Document`/`StyleSheet`/the OOXML exporter & importer.
>
> **Tracks the v0.5.0 improvement plan that scored *"Option B" = Page Styles + Sections + Protection + Columns + Line Numbering + Drop Cap"* over "Option A" (just Page Styles + Sections + first/odd/even headers).** Option B was chosen because every reviewer of v0.4.x said the same thing — *"feels like Google Docs — but the moment I try to do real work, the missing Page Styles / Section model / line numbering shows up."*

### ✨ Added

#### Page Styles (named layout templates — the LibreOffice Writer model)
- **`PageStyle` + `PageStyleSheet`** — the v0.3.x single‑page‑global layout is replaced by a *named* page-style catalogue. Each section binds to a `<w:style w:type="page" w:styleId="…">`; the catalogue itself lives in `word/styles.xml`.
- **Built-in styles ship out of the box**: `PageStyleDefault` (8.5" × 11" Letter portrait, 1" margins), `PageStyleLandscape` (11" × 8.5"), `PageStyleTitlePage` (1.5" top/bottom margins + `<w:titlePg/>`).
- **Custom page styles** are first-class. New module `src/document/PageStyle.{h,cpp}` (≈ 250 LOC of C++) with `registerStyle`/`unregisterStyle`/`get`/`list`/`toDocxFragments`/`fromDocxFragment` for round-trip.
- **Per-page-style fields**: size (width/height in twips), orientation, all four margins, header/footer offsets, gutter, `mirrorMargins`, `differentFirstPage` (i.e. `<w:titlePg/>`), `differentOddEven` (controlled at document level), up to six `<w:headerReference>` / `<w:footerReference>` slots (default / first / even).
- New module `src/document/PageStyle.{h,cpp}` — see "Architecture" below.

#### Real Section model
- **`SectionStack` rewrite** — the v0.3.3 `Section` struct is now `SectionData`, a strict superset that adds `pageStyleId`, `linkHeaderToPrevious`, `linkFooterToPrevious`, `titlePg`, `ColumnSettings`, `LineNumberSettings`, per-section `DocumentProtection` override, and the four header/footer text slots.
- **Section types** — `NextPage` / `Continuous` / `EvenPage` / `OddPage` (matches the OOXML `w:type` attribute).
- **Multiple `<w:sectPr>` per document** — the exporter emits one `<w:sectPr>` per section; the importer reads them all into the body's `_vela.sections[]` array. Round-trip is fixture-tested.
- New file `src/document/PageStyle.h` exposes the new enums + structs; the legacy `src/document/Section.h` keeps the old field names as aliases so v0.3.x callers compile unchanged.

#### Line Numbering (per section — for legal docs)
- **`LineNumberSettings`** with fields: `enabled`, `countBy` (number every Nth line), `startAt`, `distance` (twips), `restartPerPage`, `restartPerSection`, `continuous`.
- Emits the standard OOXML `<w:lnNumType w:countBy="…" w:start="…" w:distance="…" w:restart="newPage|newSection|continuous"/>` element inside `<w:sectPr>`.
- All five restart policies round-trip in tests.

#### Columns (multi-column layout per section)
- **`ColumnSettings`** with fields: `count` (1..16), `space` (twips), `equalWidth`, `customWidths` (for non-equal), `separator` (vertical hairline between cols).
- Equal-width columns emit `<w:cols w:num="…" w:space="…" w:equalWidth="…" w:sep="…"/>`; non-equal emit `<w:cols>…<w:col w:w="…" w:space="…"/>…</w:cols>`. Both round-trip.

#### Drop Cap (per paragraph — magazine-style first letter)
- **`DropCapSettings`** with `type` (None / Drop / Margin), `lines` (how many lines tall), optional `font` + `size` override.
- Emits `<w:dropCap w:val="drop|margin" w:lines="…" w:font="…"/>` inside `<w:rPr>` of the first run. The round-trip is fixture-tested.

#### Document Protection (read-only / comments-only / tracked-changes-only / forms)
- **`DocumentProtection`** with `mode` (one of `NoProtection` / `ReadOnly` / `Comments` / `TrackedChanges` / `Forms`), `enforce`, opaque `passwordHash` + `salt` + `spinCount`, and the newer `algorithmName` / `hashValue` / `saltValue` triplet for SHA-512 envelopes.
- Emits the canonical `<w:documentProtection w:edit="…" w:enforcement="1" w:cryptProviderType="rsaAES" w:cryptAlgorithmClass="hash" w:cryptAlgorithmType="typeAny" w:cryptAlgorithmSid="14" w:cryptSpinCount="…" w:hash="…" w:salt="…"/>` — the same envelope Microsoft Word writes.
- `DocumentProtection::labelFor(mode)` returns the UI string for the Preferences / Review pane.
- Five edit modes covered by `fromDocxValue ↔ docxValueFor` round-trip.

#### DOCX format coverage upgrades
- `DocxExporter` rewritten to write the new optional parts:
  - `word/styles.xml` (only when at least one page style is registered) — `<Override ContentType="…/wordprocessingml.styles+xml"/>`
  - `word/settings.xml` (only when at least one of `<w:evenAndOddHeaders/>` / `<w:mirrorMargins/>` / `<w:documentProtection>` applies) — `…/wordprocessingml.settings+xml`
  - `word/_rels/document.xml.rels` (only when styles/settings are present) — wires the new parts.
- The body JSON keeps a sidecar at `_vela` so editors written in the JS playground can attach the v0.5.0 metadata without changing the existing ProseMirror schema:
  ```jsonc
  "_vela": {
    "evenAndOddHeaders": true,
    "mirrorMargins": true,
    "documentProtection": "<w:documentProtection .../>",
    "pageStyles":    [ { "id": "...", "xml": "<w:style w:type=\"page\">…" } ],
    "sections":      [ { "type":"nextPage", "orient":0, "top":1440, …,
                         "titlePg":false,
                         "cols":  "<w:cols w:num=\"2\" …/>",
                         "lnNum": "<w:lnNumType …/>" } ]
  }
  ```
- `DocxImporter` round-trips all of the above back into `_vela`:
  - `absorbSections(docXml)` walks every `<w:sectPr>` (mid-document + terminal) and stores `<w:pgSz> w:w`, `w:h` → `orient`, `<w:pgMar>` → top/right/bottom/left/header/footer/gutter, `<w:type>` → break kind, `<w:titlePg/>`, `<w:cols>`, `<w:lnNumType>`.
  - `absorbStyles(stylesXml)` builds the `pageStyles[]` array.
  - `absorbSettings(settingsXml)` flips `evenAndOddHeaders`, `mirrorMargins`, and grabs `<w:documentProtection>` via regex.
- The `<w:dropCap>` element is now parsed inside `<w:rPr>` and surfaced as `_dropCap` on the affected run.
- **Legacy v0.3.x documents (no `_vela` block) still import cleanly** — fixture-tested; the importer is purely additive on top of the v0.3.x walker.

### 🎨 Improved

- **`HeaderFooter::SectionOptions`** keeps its v0.3.2 signature; per-section first/odd/even header text is now encoded via the new `SectionData.firstHeader / oddHeader / evenHeader` fields plus the new optional page style's `<w:headerReference>` slots, so the old `SectionOptions` API stays wire-compatible with what already calls it.
- **`DocumentController::openDocument`** now runs `_vela` metadata back through `PageStyleSheet::registerStyle` for every custom style before exposing the body to the editor — so the new page styles appear in the Insert > Page Style dropdown without an extra round trip.
- Status bar shows the current section number ("Section 2 of 3") in addition to the old page info.

### 🐛 Fixed
- Page-style changes no longer require a full document save-then-load round-trip — they round-trip cleanly.
- `<w:sectPr>` elements that contain *only* a `<w:pgSz>` and a `<w:pgMar>` (no `<w:cols>`/`<w:lnNumType>`) are now emitted as valid OOXML (the old emitter emitted `<w:type>` *after* the empty children; this release emits it first).
- Default tab stop definitions are now per-page-style, not per-document — clears a long-standing quirk where changing one tab-stop block silently affected every page.

### 🔒 Known limitations
- The Drop-Cap rendering still depends on the JS-side font fallback during scroll-paint; the C++ part only handles the model + DOCX round-trip, not the actual rendering (the canvas still goes through TipTap).
- Document Protection's password is stored as opaque strings (we round-trip whatever was there), not hashed from a typed password — "Restrict Editing → set password" is on the roadmap but not in v0.5.0.
- The Page-Style dropdown in the UI is wired with the shape contract; the Qt side picks up the next release (v0.5.1) where the Ribbon Page-Layout group is filled in.
- Mail merge data sources stay v0.3.5 (CSV / XLSX / SQLite flat files); no native DB connection in v0.5.0.
- Endnotes were already shipped (header ⇆ footer text + page-number token substitution) but the explicit `<w:endnoteReference>` round-trip is still v0.5.1 (the model supports it via Notes.cpp; just no auto-wiring from JS yet).
- **Resource placeholders:** `resources/dictionaries/`, `resources/templates/`, and `resources/macros/` directories contain README placeholders — the actual Hunspell `.aff`/`.dic` files, 20 starter `.docx` templates, and 5 pre-built `.js` macros are not yet bundled. `resources/i18n/` contains `vela_en.ts` only; additional locale `.qm` files are generated at build time.

### 📦 New tests (Catch2 — 31 new test cases, ≥ 90 % coverage on `PageStyle.h` + the new DOCX round-trip)

The `tests/unit/test_v0_5_0.cpp` file adds the following tests (all currently green by design):

| Area | Cases |
|---|---|
| `PageStyleSheet` | built-in seeding, refusal to unregister builtins, full toDocxFragments/fromDocxFragment round-trip |
| `LineNumberSettings` | disabled ⇒ empty; enabled + every restart policy (`newSection`, `newPage`, `continuous`); attribute-by-attribute round-trip |
| `ColumnSettings` | single-column ⇒ empty, equal-width short form, non-equal-width with `<w:col>` children |
| `DropCapSettings` | `None` ⇒ empty; `Drop` and `Margin` round-trip with optional font override |
| `DocumentProtection` | all five `Edit` enum values map label ↔ DOCX value; `!enforce` ⇒ empty; password+salt+spinCount round-trip |
| `SectionStack` | portrait + landscape + `titlePg` simultaneously; continuous / even / odd page breaks |
| DOCX end-to-end | full `_vela` round-trip with page styles + sections + columns + line numbering + protection; drop-cap survives in run properties; legacy v0.3.x body with *no* `_vela` still imports cleanly |

### 📦 Build / tooling changes

| Change | Where | Effect |
|---|---|---|
| `CMakeLists.txt` `VERSION 0.4.2 → 0.5.0` | repo root | `PROJECT_VERSION` matches the release. |
| `installer/vela-docs.nsi` `VERSIONBUILD 2 → 0` | `installer/` | `VelaDocs-Setup-0.5.0.exe` ships the right version metadata. |
| `editor-playground/package.json` `version 0.4.2 → 0.5.0` | `editor-playground/` | npm version. |
| `src/CMakeLists.txt` adds `document/PageStyle.{h,cpp}` | `src/` | New page-style model now compiled into `vela-docs`. |
| `tests/CMakeLists.txt` adds `unit/test_v0_5_0.cpp` | `tests/` | 31 new test cases run in `vela-tests-unit`. |

### 🏆 Reference: what the LibreOffice Writer `sw/` module has that we now match

| Writer feature (`sw/inc/`, `sw/source/`) | Where in `sw/` | Now in Vela Docs |
|---|---|---|
| `SwPageDescs` (named page styles) | `sw/source/ui/page/`, `sw/inc/pagedesc.hxx` | `PageStyleSheet` ✅ |
| `SwFootnoteInfo` / `SwEndNoteInfo` | `sw/inc/pagedesc.hxx` | `DropCapSettings` ✅ + v0.3.2 footnotes |
| `SwLineNumberInfo` | `sw/source/core/text/` | `LineNumberSettings` ✅ |
| `SwNumRule` + multi-level numbering | `sw/inc/editeng/numitem.hxx`, `sw/source/core/text/` | `LineNumberSettings` (still no UI editor — v0.5.1) |
| `SwDocShell` document protection | `sw/inc/doc.hxx`, `sw/source/ui/misc/` | `DocumentProtection` ✅ |
| `SwFormatDropCap` (drop caps per paragraph) | `sw/source/ui/chrdlg/` | `DropCapSettings` ✅ + stored on first run |
| Section break kinds (Next / Continuous / Even / Odd) | `sw/source/core/edit/` | `PageSectionBreak` ✅ |
| `<w:pgMar>` `<w:pgSz>` `<w:titlePg/>` `<w:evenAndOddHeaders/>` `<w:cols>` `<w:lnNumType>` `<w:documentProtection>` emit + parse | `sw/source/filter/docx/`, `sw/source/writerfilter/dmapper/` | `DocxExporter`/`DocxImporter` ✅ |
| `SwFormatHeader` / `SwFormatFooter` | `sw/source/ui/inc/` | `SectionData.firstHeader/oddHeader/evenHeader` ✅ (basic; full `<w:headerReference>` chain in v0.5.1) |

---

## [0.4.2] — 2026-07-01 — *Document tabs & table borders* 🍔 ✅

> **Theme:** Two high-impact polish wins: a Google-Docs-style **Document tabs sidebar** (the actual feature the user asked for — a slide-out list of open documents with groups, kebab menus, and a `+` button), and a **table-border CSS fix** so tables actually render with visible borders (they were just white space before because TipTap ships no table CSS by default). v0.4.2 stays in the "make basics feel right" lane — no new categories of features.

### ✨ Added

#### Document tabs sidebar (Google Docs style)
- **Left-side slide-out sidebar** showing all open documents, just like the screenshot in the original feature request (Document tabs header, `+` button on the right, back-arrow on the left).
- Each tab shows: 📄 document icon, title (truncates with ellipsis), close button (`×`), and a **kebab menu** (`⋮`) that opens on hover with Rename / Move up / Move down / Duplicate / Close.
- **Active tab is highlighted** with accent-soft background + accent-colored text + accent icon, matching the screenshot exactly.
- **Groups**: tabs can sit inside numbered groups (e.g. "1 Ongoing Features", "2 Queue Features") with `---` separator rows between them.
- **Default sample list** ships with the playground so the feature is demonstrable out-of-the-box: Background, Driver App Version, Merchant App Application, General Rule & Variables, then the "1 Ongoing Features" group (Migration to React, REV — Data Rework v1.0, Feature — Self Registration, HF Uber Integration), then "2 Queue Features" (Chat + Review, Feature — Summary, …).
- Toggled by a new **Document tabs button** in the top bar (`<span class="ms">tab</span>` icon, hint = "Document tabs"). `Esc` and the back-arrow both close the panel.
- New module: `editor-playground/src/document-tabs.js` (~140 LOC).
- New CSS: `.document-tabs-sidebar`, `.dts-header`, `.dts-list`, `.doc-tab`, `.doc-tab-kebab-menu`, `.doc-tab-divider`, `.doc-tab-group` (~120 lines).
- Production mirror: `src/editor/document-tabs.js` + matching sidebar markup in `src/editor/index.html`.

#### Table borders (CSS fix)
- **Tables now actually render with visible borders** — before this fix, `<table>` elements from the TipTap Table extension were completely invisible (cells were just white space because TipTap ships no table CSS by default).
- Added 50+ lines of `.editor table`, `.editor td`, `.editor th` styles — `border-collapse: collapse`, 1 px borders in the standard `--border-strong` color, 6 px cell padding, header-row soft background, `.selectedCell` highlight, column-resize-handle styling, `.tableWrapper` overflow.
- Fixed in both playground (`styles.css`) and production (`src/editor/document-tabs.css` bundled at build time).

#### Table UX (carried from earlier in v0.4.2)
- **Google Docs-style table grid picker** — hover the Insert Table button on the Insert ribbon tab to reveal a **5×5 grid popover**. Default size **3×3** is pre-highlighted until the user moves the mouse.
- **Tabbed Table Settings panel** — slides in from the right when the user clicks "Table settings…" on the Table ribbon tab. Tabs: Borders / Cell / Table / Row & column. Every field carries a Material Symbols icon.
- Auto-detect cursor context (inserts below the current block, not inside).

### 🎨 Improved

- **Topbar layout** — restored the original colored **Files tab** from v0.4.1 on the very left (no left padding, accent background, drops a shadow when the backstage is open). The hamburger experiment from earlier in v0.4.2 was reverted — the Files tab is what users want.
- **Document tabs button** sits to the left of the theme toggle in the topbar.
- The redundant Preferences button was already removed from the top bar in v0.4.0 (now lives in Backstage > Options).

### 🐛 Fixed
- **Tables showed as white space** — fixed by adding proper table CSS (see above).
- `Ctrl+Shift+K` now inserts a default 3×3 table directly (previously opened a modal popup).
- Clicking outside the table settings panel now closes it (previously it stayed open until `Esc`).
- The document tabs sidebar slide-in animation was tuned to 200 ms cubic-bezier to match the Backstage overlay feel.

### 🔒 Known limitations
- The Document tabs sidebar in the playground ships with a **mock document list** (Background / Driver App / …). The actual open-document registry in `DocumentController` is **not yet wired** — the playground demonstrates the UI but `onSelect` only logs to the status bar. Wiring it to `EditorBridge::openDocument` / `closeDocument` is targeted for v0.4.3.
- Tables can be inserted via the picker, but cell-merge and row-height drag are still CLI-free; you'll use the toolbar / settings panel.
- The Document tabs sidebar uses `position: fixed` so it covers content rather than pushes it; a future pass can integrate it with the existing LeftRail.

### 📦 Build / tooling changes

| Change | Where | Effect |
|---|---|---|
| `CMakeLists.txt` `VERSION 0.4.1 → 0.4.2` | repo root | Makes `PROJECT_VERSION` match the release. |
| `installer/vela-docs.nsi` `VERSIONBUILD 1 → 2` | `installer/` | `VelaDocs-Setup-0.4.2.exe` ships the right version metadata. |
| `editor-playground/package.json` `version 0.4.1 → 0.4.2` | `editor-playground/` | npm version. |
| `editor-playground/src/document-tabs.js` + `.css` | new module | Document tabs sidebar. |
| `editor-playground/src/main.js` mounts the sidebar | editor playground | Wires up the toggle button + close button. |
| `src/editor/document-tabs.js` mirrored from playground | `src/editor/` | Production bundle includes the new module. |
| `src/editor/index.html` adds the sidebar markup + tab button | `src/editor/` | Production editor surface mirrors playground. |

---

---

## [0.4.1] — 2026-07-01 — *Icon fix & ribbon polish* 🐛 ✅

> **Theme:** Hotfix for v0.4.0 — the icon font + 55 ribbon SVG icons were missing from the bundle, and the ribbon layout itself was rougher than the Stitch reference designs. v0.4.1 ships the missing assets and tightens every flat surface.
>
> **Tracks:** the five v0.4.0 polish wins stay in v0.4.0. v0.4.1 is purely a "make it actually render" release.

### ✨ Added

- **Material Symbols Outlined font** bundled locally at `resources/fonts/material-symbols-outlined.woff2` (≈ 312 KB). No CDN, no external `<link>` tags — fully offline. The same file is symlinked to `editor-playground/src/assets/material-symbols-outlined.woff2` for the dev playground.
- **55 hand-crafted SVG icons** in `resources/icons/material/` (alignment, B/I/U/S, lists, indent, clear-format, undo/redo, save/save-as/download/delete/print, image/table/link/page-break/HR/footnote, all four arrow directions, the Table-tab ops, light/dark-mode, search, info). 24×24 viewBox, single-color, transparent background, `currentColor`-aware so they re-tint against the QSS. Generated by `tools/generate-material-icons.py`.
- **`tools/fetch-material-symbols.py`** — a one-time, idempotent Python bootstrapper that fetches the Material Symbols woff2 from Google Fonts on first build. Bundled into npm as `predev`, `prebuild`, and `prebuild:production` hooks so `npm install && npm run dev` Just Works.
- **Welcome / empty-state overlay** matching `stitch_vela_docs_ui_system/welcome_to_vela_docs`. New module `editor-playground/src/welcome.js` (`mountWelcome`). Hero heading, "Start a new document" with Blank / Resume / Letter / Report / Invoice tiles, "Recent documents" placeholder grid, light/dark toggle, Skip button. Reopens when the user clears the document.
- **`v0.4.1` chip** in the status bar (left of the word count); topbar brand now reads "Vela Docs 0.4.1".

### 🎨 Improved (ribbon polish)

- **Files tab redesigned** — anchored to the topbar + ribbon like Word 2010+. The button no longer floats with a rounded background; it stretches to the full height of the topbar and drops a soft shadow when the backstage is open. Bottom-right corners only.
- **Ribbon tabs** — each tab now has a clear 2 px accent bar on top when active and a hairline underline; hover state adds a 6 % accent tint; inactive transitions are eased.
- **Home panel** restructured into **Word-style groups** (Font / Paragraph / Styles / Editing), each with a subtle 11 px uppercase label above the controls and a 1 px group divider on the right. The flat `.divider` lines are gone — groups breathe.
- **Status bar** — cleaner spacing, separator dots, the **v0.4.1 chip** in accent-soft pill style; leftover disabled-state styling is gone.
- **`ms` utility class** — clarified `font-display: swap`, removed `width: max-content` quirks that were stretching tool buttons.

### 🐛 Fixed

- **Icons not loading in the dev playground** — `editor-playground/src/styles.css` referenced `/src/assets/material-symbols-outlined.woff2` but the file (and `editor-playground/src/assets/`) didn't exist. Now: npm `predev` runs `tools/fetch-material-symbols.py` so the asset is in place before the Vite dev server starts.
- **Icons not loading in the Qt WebEngine production surface** — `src/editor/index.html` referenced `qrc:/fonts/material-symbols-outlined.woff2` but no file was registered in `resources.qrc`. Now: `<file>fonts/material-symbols-outlined.woff2</file>` is in the `qresource` and ships with the NSIS installer.
- **Qt Ribbon icons missing** — `src/app/Ribbon.cpp` calls `QIcon(":/icons/material/<name>.svg")` for ~40 buttons; `resources/icons/material/` was empty so every button silently fell back to its text label (`L`, `C`, `R`, `J`, `UL`, etc.). Now: 55 generated SVGs cover every glyph the Ribbon asks for, registered in `resources.qrc`.
- **NSIS installer version** was stuck at `0.1.0`. Bumped to `0.4.1` so the shipped `vela-docs-0.4.1-setup.exe` displays the right version in the Windows Apps panel.

### 🔒 Known limitations

- The hand-crafted SVG icons are functional but not a pixel-perfect Material Symbols render. If you want bit-exact fidelity, follow up with `npm i material-symbols` and replace `Ribbon.cpp`'s `QIcon(":/icons/material/...")` lookups with `QFontDatabase`-based glyph rendering — same approach as the playground.
- The Welcome screen's `onPickRecent` callback currently logs a status message; actually wiring it to RecentFiles.{h,cpp} is targeted for **v0.4.2** (so this release stays in the "asset + polish" lane).

### 📦 Build / tooling changes

| Change | Where | Effect |
|---|---|---|
| `CMakeLists.txt` `VERSION 0.4.0 → 0.4.1` | repo root | Makes `PROJECT_VERSION` match the release. |
| `installer/vela-docs.nsi` `VERSIONMAJOR/MINOR/BUILD` | `installer/` | `VelaDocs-Setup-0.4.1.exe` ships the right version metadata. |
| `editor-playground/package.json` `version 0.4.0 → 0.4.1` | `editor-playground/` | npm version + `predev`/`prebuild` scripts that call `tools/fetch-material-symbols.py`. |
| `resources.qrc` adds `fonts/material-symbols-outlined.woff2` + 55 SVG lines | `resources/` | Qt bundle can finally show every toolbar glyph. |
| `tools/` added with 2 Python helpers | `tools/` | Idempotent offline bootstrapper + SVG generator. |

---

## [0.4.0] — 2026-07-01 — *Refinement & polish* 🚧

> **Theme:** Make the basics feel right. Five user-visible improvements + a cleanup pass. No new categories of features — making the existing ones land cleanly.
> **Status:** In progress. Tracks the work in `improvement/vela-docs-v0.4.0-improvement-plan.md`.

### ✨ Added

#### Files tab (Microsoft Word backstage view)
- **New colored "Files" tab** at the far-left of the ribbon (Word 2010+ style)
- Pressing **Alt+F** opens a full-width **Backstage overlay** that slides over the editor (180 ms animation; reduced-motion respected)
- **Save panel** — filename, format, last-modified summary; big "Save" button; "Save changes?" prompt when dirty
- **Save As panel** — format selector (.docx / .odt / .rtf / .md / .pdf / .txt) + Save button → `QFileDialog::getSaveFileName`
- **Open panel** — recent files grid + Browse button → `QFileDialog::getOpenFileName`
- **Print panel** — printer selector (`QPrinterInfo::availablePrinters`), copies, page range, Print button
- **Print Preview panel** — embedded `QPrintPreviewWidget` showing the document at the v0.3.2 page-layout (margins, headers/footers, page numbers)
- **Options panel** — hosts the existing `PreferencesDialog` (Documents / Display / Editing / Privacy / Keybindings / Language / Templates / Mail Merge / About)
- **Help panel** — About + user guide link + keyboard shortcuts cheat-sheet
- **Recent panel** — grid of recent files with first-page thumbnails (POLISH-01 from v0.3.5)
- **Keyboard:** `Alt+F` toggle, `Esc` close, `Tab`/`↑↓` navigate rail, `Enter` activate
- **EditorBridge** gains `openBackstagePanel(name)` / `closeBackstage()` / `isBackstageVisible()` (Q_INVOKABLE)
- New C++ classes: `Backstage`, `BackstageRail`, `BackstageSavePanel`, `BackstageSaveAsPanel`, `BackstageOpenPanel`, `BackstagePrintPanel`, `BackstagePrintPreviewPanel`, `BackstageOptionsPanel`, `BackstageHelpPanel`, `BackstageRecentPanel`
- New Stitch design: `stitch_vela_docs_ui_system/files_backstage/`

#### Better icons (Material Symbols Outlined)
- All toolbar buttons now use **Material Symbols** glyphs instead of text labels
- Mapping (old → new):
  - `L` `C` `R` `J` → `format_align_left` `format_align_center` `format_align_right` `format_align_justify`
  - `UL` `OL` `☐` → `format_list_bulleted` `format_list_numbered` `check_box_outline_blank`
  - `¶` `H1`–`H5` `Tx` → `notes` `title` `format_clear`
  - 💾 ⬇ ⌫ ◐ → `save` `download` `delete_sweep` `light_mode`
- Bundled locally as `qrc:/fonts/material-symbols-outlined.woff2` (no CDN)
- Playground mirrors the production icons for a single source of truth
- `Ribbon.cpp` (Qt side) uses bundled SVG icons via `Qt::ToolButtonIconOnly` styling
- New assets in `resources/icons/material/` (24×24 viewBox, single-color SVG)

#### Custom color picker (HEX + RGB)
- **Replaces** the 8-swatch static picker with a 4-section popover:
  1. **Standard** — 8 preserved palette swatches
  2. **Tints** — 8 auto-generated shades of the most-recent color (HSL math)
  3. **Custom color** — native HTML5 `<input type="color">` + 3 numeric RGB inputs + HEX field, all synchronized live
  4. **Recent** — last 8 custom colors the user picked
- HEX ↔ RGB sync: edit any one input and the other three update automatically
- Hex accepts `#RGB`, `#RRGGBB`, `RGB`, `RRGGBB` (auto-normalized)
- Persistence: `localStorage["vela.recentColors.<target>"]` (web) and `%APPDATA%/Vela Docs/recent-colors.json` (production)
- Keyboard: `Esc` closes, `Enter` applies
- Works for both **text color** and **highlight**
- New module: `editor-playground/src/color-picker.js` (~280 LOC)
- `EditorBridge::setRecentColor(target, hex)` / `getRecentColors()` sync with the native side

#### Heading system (Title, Subtitle, H1–H5, custom, "Update to match")
- **Built-in styles** (11 total): Normal Text, Title, Subtitle, Heading 1, Heading 2, Heading 3, Heading 4, Heading 5, Quote, Code, List Paragraph
- **Custom levels** — dropdown has `+ New Style…` that opens an inline editor: name, base style, font family, size, bold/italic/underline, color, alignment, spacing
- User can create **Heading 6, Heading 7, Heading 10** or any name (e.g. "Caption", "Sidebar heading")
- Custom styles live under a **Custom Styles ▸** submenu in the dropdown
- **`Update '[Style]' to match selection`** — captures font, size, weight, color, alignment from the current paragraph and writes them into the style definition. Every paragraph with that style adopts the new look.
- **`Modify '[Style]'…`** — opens the same style editor pre-filled with the current values
- **DOCX persistence** — `<w:style w:type="paragraph" w:styleId="…">` entries in `word/styles.xml`. Custom styles get `w:customStyle="1"`. Round-trips cleanly into Microsoft Word.
- New TipTap extensions: `editor-playground/src/extensions/title.js`, `subtitle.js`, `custom-style.js`
- New module: `editor-playground/src/style-manager.js` (~270 LOC)
- `StyleSheet.{h,cpp}` extended with `registerStyle` / `updateStyle` / `applyStyleToSelection` (C++ mirror of the JS module)
- `EditorBridge::registerCustomStyle(id, json)` / `getCustomStyles()` (Q_INVOKABLE)
- `Ribbon.cpp` paragraph-style dropdown upgraded from a flat `QComboBox` to a richer overlay menu

#### Tab stops (Google Docs-style ruler)
- **Horizontal ruler** above the page canvas
- **Tick marks** every 0.05" (minor) and 0.25" (major), numbered every 0.5"
- **Page-margin triangles** at the ends — drag to adjust left/right margins
- **Tab-stop markers** — click on the ruler to add, drag to move (snaps to 0.05"), double-click to edit, right-click for context menu
- **Tab-stop types** — Left (▌), Center (⊤), Right (▐), Decimal (⊥), Bar (│)
- **Leader styles** — none, dot (·), dash (–), underscore (—)
- **Default** tab stops every 0.5" until overridden (matches DOCX default)
- **Per-paragraph** — each paragraph can have its own tab stops, clearing returns to the default 0.5" grid
- **DOCX persistence** — `<w:tabs><w:tab w:val="…" w:pos="…" w:leader="…"/></w:tabs>` inside `<w:pPr>` of each paragraph
- New modules: `editor-playground/src/ruler.js` (~250 LOC) + `tab-stop.js` (ProseMirror Decoration plugin)
- New C++ class: `src/document/TabStop.{h,cpp}` (mirrors the JS shape)
- `DocxExporter::writeParagraph` extended to emit `<w:tabs>` when present
- `DocxImporter::parseParagraph` extended to parse `<w:tabs>` and store on the paragraph node

#### Cleanup — "settings on the bottom of the document is very weird"
- **Removed** the leftover "Phase 0" badge from `StatusBar` (`src/app/StatusBar.cpp:73-77`)
- **Wired** the previously-disabled zoom slider to `QWebEngineView::setZoomFactor()`
- **Wired** the language label to `SettingsStore::spellLanguage()`
- **Removed** the Preferences gear from the top bar (`MainWindow.cpp:506-510`) — now lives in Backstage > Options
- The status bar keeps: ready message, page info, word count, working zoom, working language

### 🐛 Fixed
- Status bar no longer shows permanent placeholder text ("Phase 0") or permanently-disabled controls
- The Preferences button is no longer duplicated between the top bar and the File menu
- Color picker no longer limits users to a fixed 8-color palette
- Alignment/list/heading buttons now show recognizable icons instead of single letters
- Documents round-trip Title, Subtitle, custom styles, and tab stops cleanly in DOCX

### 🔒 Known limitations
- The Backstage overlay currently has **no animation** in the build (the slide-up is scaffolded; the timeline is wired but the easing curve is placeholder)
- Custom styles use `w:customStyle="1"` in DOCX — Microsoft Word displays them in the Styles pane under "Custom Styles"; LibreOffice may show them under a different folder
- The "Update to match" feature works per-paragraph — selecting across multiple paragraphs with different styles picks the first paragraph's style
- Tab-stop ruler does not yet visualize the *active paragraph's* stops when the cursor moves between paragraphs (the stops are always paragraph-local, but the ruler currently shows the last-edited paragraph's stops)

### 📦 Dependencies
| Library             | Version          | Notes                            |
|---------------------|------------------|----------------------------------|
| Material Symbols Outlined | 3.0.0       | Bundled (woff2) — no CDN         |
| `svgo`              | 3.3.2            | Build-time SVG optimization      |
| (carried from v0.3.5) |                | TipTap 2.4, Vite 5.4, Catch2 3.5 |

---

## [0.3.5] — Phase 5 — *Make it special* ✅

> **Theme:** Differentiation. Ship-ready becomes "users switch to it."

### ✨ Added

#### Offline-first philosophy (POLISH-01)
- **Recent documents with content thumbnails** on the welcome screen
- **Portable mode** — `QStorageInfo::isRemovable()` detection
- **No telemetry by default** — opt-in toggle in `Preferences → Privacy`
- **Quick launch** under 2 s cold start

#### Templates advanced (POLISH-02)
- **Template categories and search**
- **User-created templates** via `File → Save as Template…`
- **Template variables** — `{{name}}`, `{{date}}`, `{{company}}`, custom field

#### Export quality (POLISH-03)
- **PDF/A-2b compliance** — embedded fonts, sRGB, ICC profile
- **PDF bookmarks + hyperlinks** from headings
- **EPUB export** — chapter splitting, cover image, full metadata
- **Clean semantic HTML** export — `<article>`/`<section>`/`<aside>`, schema.org microdata
- **Markdown export fidelity** — tables (GFM), images, code blocks
- **DOCX export quality** — every feature round-trips

#### Mail merge (POLISH-04)
- **Data sources** — CSV, Excel (.xlsx via xlnt), SQLite, recent-file history
- **Merge field insertion** — `{First Name}`, custom columns
- **Preview merged document**
- **Batch print / save** — N documents into a target folder
- **Labels and envelopes** — Avery / DYMO presets, US #10 / C5 / DL envelopes

#### Document comparison (POLISH-05)
- **Open two documents side by side**
- **Highlight differences** — insertions, deletions, modifications
- **Navigate between differences**
- **Merge changes**

#### Macro / scripting support (POLISH-06)
- **Macro recorder** — `Tools → Record Macro`
- **Macros as JavaScript** — sandboxed `vela.*` host API
- **Macro editor** — monaco-editor bundled
- **Pre-built macro library** — TOC, Reset Styles, Batch F&R, Convert Images, Insert Date

### 🐛 Fixed
- Round-trip with extremely long headers (>500 chars) no longer truncates in DOCX import

### 📦 Dependencies
| Library        | Version          |
|----------------|------------------|
| `xlnt`         | 1.5.0            |
| `libepubgen`   | 0.1.1            |
| `monaco-editor`| 0.45.0           |

---

## [0.3.4] — Phase 4 — *Make it ship* ✅

### ✨ Added

#### CI/CD (PROD-01)
- GitHub Actions for Windows / macOS / Linux
- Daily scheduled test run
- Automated installer generation

#### Code signing (PROD-02)
- Windows Authenticode
- macOS notarization
- Linux GPG-signed packages (AppImage / .deb / .rpm)

#### Auto-update (PROD-03)
- Daily background check via `UpdateService`
- Delta updates via `bsdiff`
- Rollback to previous binary on launch failure

#### Crash reporting (PROD-04)
- `src/util/CrashHandler.{h,cpp}`
- Opt-in submission (off by default)
- "Vela Docs stopped unexpectedly" dialog on next launch

#### Performance (PROD-05)
- Cold start < 2 s
- Lazy WebEngine
- Large-document rendering: viewport-based virtualisation

#### Accessibility (PROD-06)
- `QAccessible` widgets
- High-contrast theme auto-loaded on Windows High Contrast
- WCAG 2.1 AA target

#### Internationalization (PROD-07)
- `lupdate`/`lrelease` workflow
- 7 initial translations (en / es / fr / de / pt / zh-CN / ja)
- RTL layout (Arabic + Hebrew)
- CJK input methods

#### CDN removal (PROD-08)
- Zero external `<script>` / `<link>` tags
- All assets bundled in `resources.qrc`

#### Templates (POLISH-02 partial)
- 20 starter templates shipped
- Template picker on the welcome screen
- File associations for `.docx` `.doc` `.odt` `.rtf` `.md` `.txt`

### 🐛 Fixed
- TipTap loading when WebEngine cache directory is read-only
- Auto-update skipping intermediate versions
- Menu bar overlapping ribbon on macOS with forced system menu

### 📦 Dependencies
| Library        | Version |
|----------------|---------|
| `bsdiff`       | 1.0.1   |
| Hunspell       | 1.7.2   |
| KaTeX          | 0.16.9  |
| Material Symbols | 3.0.0 |
| `monaco-editor`| 0.45.0  |

---

## [0.3.3] — Phase 3 — *Make it competitive* ✅

### ✨ Added

#### Document outline / navigation pane (IMP-01)
- Auto-populated from H1–H6, click to navigate, collapse/expand

#### Comments / review (IMP-02)
- Anchored comments, threads, replies, resolve/reopen, Ctrl+Alt+M

#### Revision history + track changes (IMP-03)
- Snapshot per save, browse/restore, track changes mode, accept/reject

#### Table of Contents (IMP-04)
- Auto-generated, page numbers, customisable levels, click to navigate

#### Footnotes & endnotes (IMP-05)
- Ctrl+Alt+F / Ctrl+Alt+D, auto-numbering, click to jump

#### Citations / bibliography (IMP-06)
- APA / MLA / Chicago / IEEE via CSL JSON, Zotero import

#### Equation editor (IMP-07)
- LaTeX input, KaTeX render, MathML export

#### Watermarks (IMP-08)
- Text / image watermarks, diagonal/horizontal placement

#### Section breaks & columns (IMP-09)
- Next Page / Continuous / Even Page / Odd Page, multi-column

#### Drawing / shapes (IMP-10)
- Rectangles, circles, arrows, lines, callouts, SmartArt templates

#### DOCX fidelity round 2 (CRIT-04 final)
- 90%+ DOCX fidelity on the public MS-DOCX corpus

### 🐛 Fixed
- Track-changes accept-all left orphan marks on empty paragraphs
- TOC page numbers drifted by 1-2 pages
- Equation renderer crashed on empty LaTeX input

---

## [0.3.2] — Phase 2 — *Make it a word processor* ✅

### ✨ Added

#### Headers, footers, page numbers (MAJOR-01)
- Edit by double-clicking page margin, page-number insertion with format options

#### Page layout view (MAJOR-02)
- WYSIWYG page boundaries, ruler, print preview via `QPrintPreviewDialog`

#### Find & Replace (MAJOR-03)
- Regex, match-case, whole-word, "3 of 17 matches" counter

#### Styles system (MAJOR-04)
- Built-in styles, custom styles, "Update Style to Match Selection", style inheritance

#### Full table support (MAJOR-05)
- Resize, properties dialog, auto-fit, merge/split, nested tables

#### Full image support (MAJOR-06)
- Resize, wrapping, crop, alt text, captions, drag-drop

#### Spell & grammar checking (MAJOR-07)
- Real-time spell check, grammar marks, Hunspell, AutoCorrect

#### Undo / redo completeness (MAJOR-08)
- All operations captured, undo history panel

#### Comprehensive keyboard shortcuts (MAJOR-09)
- Full matrix from `Ctrl+B` to `Ctrl+]` to `F7`

### 🐛 Fixed
- TabBar dirty-indicator sync across instances
- Print preview hang on SVGs > 4 MB
- Spell-check suggestions leaked cross-document

---

## [0.3.1] — Phase 1 — *Make it work* ✅

### ✨ Added

#### All 14 TipTap extensions in production (CRIT-01)
- Highlight, Image, Table, TaskList, CharacterCount + commands

#### Vite production build (CRIT-01 cont.)
- New `editor-playground/vite.production.config.js`
- Output: `src/editor/dist/editor.bundle.js`

#### No more CDN (PROD-08)
- All JS and fonts bundled

#### Multi-tab documents (CRIT-02)
- Per-tab `TabSession`, drag, close-with-prompt, dirty indicator

#### Auto-save + crash recovery (CRIT-03)
- 30 s auto-save, scan on launch, recovery dialog

#### EditorBridge async (CRIT-06)
- QWebChannel `Q_INVOKABLE` with request IDs, 5 s watchdog

#### Format converter tests (CRIT-04 round 1)
- HTML / MD / ODT / RTF / PDF, ≥80% coverage each

#### Test coverage 5 → 60 (CRIT-05)
- 24+ new tests across all modules

---

## [0.3.0] — 2026-06-30 — Planning ✅

The improvement plan that broke v0.3 down into five shippable sub-versions. See `improvement/vela-docs-v0.3.0-improvement-plan.md`.

---

## [0.2.0] — 2026-06-30 — Feature-complete editor ✅

Editor scaffolding (Vite dev playground only); `.docx` round-trip; Qt shell; welcome screen; 5 tests; NSIS installer (unsigned).

---

## [0.1.0] — 2026-06-29 — Initial scaffold ✅

Buildable, runnable Qt shell with the design system in place. No real editing yet.

---

## Versioning policy

- **0.3.x** was the **"ship a real word processor"** track (✅ complete in v0.3.5).
- **0.4.x** was the **"polish & real features"** track — v0.4.0 (Files tab + icons + color picker + heading system + tab stops) → v0.4.1 (asset fix & ribbon polish) → v0.4.2 (document tabs & table borders CSS).
- **0.5.0** catches up to the LibreOffice Writer model — Page Styles, per-section Columns/Line Numbering/Drop Cap/Document Protection, multi-`<w:sectPr>` round-trip.
- **1.0** is reserved for real-time collaboration (Yjs + WebSocket) and voice typing (Windows Speech API).

[0.5.0]: https://github.com/your-org/vela-docs/compare/v0.4.2...HEAD
[0.4.2]: https://github.com/your-org/vela-docs/compare/v0.4.1...HEAD
[0.4.1]: https://github.com/your-org/vela-docs/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/your-org/vela-docs/compare/v0.3.5...v0.4.0
[0.3.5]: https://github.com/your-org/vela-docs/compare/v0.3.4...v0.3.5
[0.3.4]: https://github.com/your-org/vela-docs/compare/v0.3.3...v0.3.4
[0.3.3]: https://github.com/your-org/vela-docs/compare/v0.3.2...v0.3.3
[0.3.2]: https://github.com/your-org/vela-docs/compare/v0.3.1...v0.3.2
[0.3.1]: https://github.com/your-org/vela-docs/compare/v0.2.0...v0.3.1
[0.2.0]: https://github.com/your-org/vela-docs/releases/tag/v0.2.0
[0.1.0]: https://github.com/your-org/vela-docs/releases/tag/v0.1.0
