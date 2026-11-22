# Vela Docs v0.3.x — Execution Plan (split into sub-versions)

> **Source of truth:** `improvement/vela-docs-v0.3.0-improvement-plan.md`
> **Plan-of-record for sub-versioning:** this file
> **Date:** 2026-06-30

The single improvement plan in `improvement/vela-docs-v0.3.0-improvement-plan.md`
is split into five shippable sub-versions. Each sub-version is a self-contained
release with its own CHANGELOG entry, set of feature toggles, and acceptance
criteria. No sub-version depends on later ones for compilation — you can ship
v0.3.1 today and v0.3.2 tomorrow.

| Sub-version | Maps to plan phase | Theme | Weeks | Latest commit |
|---|---|---|---|---|
| **v0.3.1** | Phase 1 — *Make it work* | Barely functional → usable | 1–3 | … |
| **v0.3.2** | Phase 2 — *Make it a word processor* | Usable → good | 4–9 | … |
| **v0.3.3** | Phase 3 — *Make it competitive* | Good → great | 10–15 | … |
| **v0.3.4** | Phase 4 — *Make it ship* | Great → shippable | 16–19 | … |
| **v0.3.5** | Phase 5 — *Make it special* | Shippable → differentiated | ongoing | … |

## v0.3.1 — Phase 1: Make it work

Closes all 🔴 CRIT items in the improvement plan:

| ID | Item | Concrete deliverable |
|---|---|---|
| CRIT-01 | Port 9 missing TipTap extensions | New files in `src/editor/extensions/`: `highlight.js`, `image.js`, `table.js`, `table-row.js`, `table-header.js`, `table-cell.js`, `task-list.js`, `task-item.js`, `character-count.js` — plus port of `modals.js` and full command dispatcher from the playground. |
| CRIT-01 cont. | Vite production build | New `editor-playground/vite.production.config.js` producing `src/editor/dist/editor.bundle.js`. |
| CRIT-02 | Multi-tab document support | Rewrite `src/app/TabBar.cpp/.h`. New `src/document/TabSession.h/.cpp` (per-tab `DocumentController`, undo stack, dirty flag). Drag-reorder, close-with-prompt, Ctrl+N/Ctrl+W/Ctrl+Tab. |
| CRIT-03 | Auto-save + crash recovery | New `src/services/AutoSaver.h/.cpp` (QTimer-driven, writes to `QStandardPaths::AppDataLocation/autosave/<uuid>.docx`). New `src/services/RecoveryService.h/.cpp` to detect leftover autosaves on launch. Existing `RevisionStore` gains snapshot-on-autosave. |
| CRIT-04 | Format converters tested | New tests in `tests/unit/`: `test_html_format.cpp`, `test_markdown_format.cpp`, `test_odt_format.cpp`, `test_rtf_format.cpp`, `test_pdf_export.cpp`, plus DOCX-styles/-images/-tables fixtures in `test_docx.cpp`. |
| CRIT-05 | Coverage 5 → 60% | One new `TEST_CASE` file per public class. List is in `tests/unit/` after this commit. |
| CRIT-06 | EditorBridge async + error handling | Rewrite `src/app/EditorBridge.cpp/.h` using `QWebChannel` `Q_INVOKABLE` returning token-bound results. Add 5 s watchdog. Errors → `Logger` + user toast. |
| PROD-08 | Remove CDN dependency | Update `src/editor/index.html` to load bundled `qrc:` paths. Update `resources/resources.qrc` to bundle TipTap + fonts. |

**Exit criteria**

- `vela-docs.exe` runs offline (no network requests during normal flow)
- ≥ 4 documents openable in tabs, with dirty indicators
- Auto-save dialog offers recovery after `kill -9`
- All 14 TipTap extensions available in production editor
- Toolbar buttons highlight when format is active
- ≥ 80 % line coverage on format converters
- EditorBridge command failure shows toast instead of silently dying

## v0.3.2 — Phase 2: Make it a word processor

Closes all 🟠 MAJOR items:

| ID | Item | Concrete deliverable |
|---|---|---|
| MAJOR-01 | Headers, footers, page numbers | New `src/document/HeaderFooter.h/.cpp`, `src/editor/extensions/headerFooter.js`, `src/formats/docx_hf.cpp` (handles `<w:hdr>`/`<w:ftr>`/`<w:sectPr>`). Different-first-page, odd/even, section-specific, `{PAGE}` / `{NUMPAGES}` / Roman tokens. |
| MAJOR-02 | Page layout / print layout view | New `src/editor/extensions/page-layout.js` + `src/editor/styles/page-layout.css`. Virtualised page columns. Page break visualisation. Ruler with margin/indent/tab stops. Wire existing `src/ui/dialogs/PageSetupDialog.cpp/.h`. Portrait/landscape per section. `QPrintPreviewDialog` via `src/services/PrintService.cpp`. |
| MAJOR-03 | Find & Replace | Wire `src/ui/dialogs/FindReplaceDialog.cpp/.h`. New `src/editor/extensions/find-replace.js` (TipTap `Decoration` plugin). Find/replace/replace-all, regex, match-case, whole-word, search-selection. "3 of 17" counter. Shortcuts Ctrl+F, Ctrl+H, F3. |
| MAJOR-04 | Styles system | New `src/document/StyleSheet.h/.cpp` (Normal, H1–H6, Title, Subtitle, Quote, List Paragraph, Code; inheritance; custom styles). New `src/ui/widgets/StylesPane.cpp/.h`. "Update Style to Match Selection". Persist as `<w:styles>` in DOCX (extend `DocxExporter`). |
| MAJOR-05 | Full table support | TipTap table extensions from v0.3.1 + : resize handles, `src/ui/dialogs/TablePropertiesDialog.cpp/.h`, cell padding/spacing, borders/color/width, cell shading, auto-fit (window/contents/fixed), merge/split with visual indicators, nested tables, captions, table↔text conversion, sorting. |
| MAJOR-06 | Full image support | TipTap image extension from v0.3.1 + : resize w/ aspect lock, 5 wrap modes (in-line/square/tight/behind/in-front), crop, alt text, captions, drag-drop from Explorer, paste from clipboard, `src/ui/dialogs/ImagePropertiesDialog.cpp/.h`, compression options. |
| MAJOR-07 | Spell & grammar checking | Flesh out `src/services/SpellChecker.cpp`. Integrate Hunspell via `FetchContent` in `CMakeLists.txt`, dictionaries in `resources/dictionaries/`. New `src/editor/extensions/spellcheck.js` (red wavy via ProseMirror `Decoration`, blue wavy for grammar). Right-click suggestions/add-to-dictionary/ignore-all. F7 spell-check dialog. AutoCorrect: smart quotes, double-space fix, capitalise-first-letter. Languages: en, es, fr, de, pt. |
| MAJOR-08 | Undo/redo completeness | Audit every command path in `DocumentController`. Add per-tab undo stack (from v0.3.1 TabSession). Undo history panel (`src/ui/widgets/UndoHistory.cpp/.h`). Persist across save/load. |
| MAJOR-09 | Comprehensive keyboard shortcuts | New `src/app/ShortcutManager.h/.cpp`, `resources/shortcuts.json`. Full matrix: Ctrl+B/I/U, Ctrl+Z/Y/Shift+Z, Ctrl+S/Shift+S, Ctrl+O/N/P/F/H/A/Home/End, Ctrl+K, Ctrl+Shift+L/O, Ctrl+1–4, Ctrl+0, F7, Ctrl+]/[. Customisable in `PreferencesDialog`. Tooltips show the shortcut. |

**Exit criteria**

- Headers/footers/page numbers round-trip through real `.docx` files
- Page-layout view matches print preview
- Find & Replace supports regex/match-case/whole-word/in-selection
- Styles pane visible; custom styles save & reload
- Tables resize/merge/split/border-color all work
- Images: insert, resize, 5 wrap modes, alt text, captions work
- Red/blue squiggles appear; suggestions resolve via right-click
- Undo covers every format/element; F7 opens check dialog
- Test coverage ≥ 60 %

## v0.3.3 — Phase 3: Make it competitive

Closes all 🟡 IMP items + final DOCX fidelity round:

| ID | Item | Concrete deliverable |
|---|---|---|
| IMP-01 | Document outline / nav pane | Wire `src/app/LeftRail.cpp` outline button + `src/app/Sidebar.cpp`. Auto-build tree from H1–H6. Click to navigate. Real-time updates via editor `onUpdate`. Optional drag-to-reorder. |
| IMP-02 | Comments / review system | Wire `stitch_vela_docs_ui_system/comment_thread_ui/`. New `src/document/CommentStore.h/.cpp`, `src/editor/extensions/comments.js` (anchored ProseMirror marks). Reply/resolve/reopen. Persist as DOCX `<w:comments>`. Ctrl+Alt+M. |
| IMP-03 | Revision history + track changes | Wire `stitch_vela_docs_ui_system/revision_history_panel/`. Existing `RevisionStore` gains snapshot-on-save. Track changes mode: insertion/deletion markup, accept/reject per-change and bulk, side-by-side compare. |
| IMP-04 | Table of Contents | New `src/document/TableOfContents.h/.cpp`, `src/editor/extensions/toc.js`. Auto-generate from headings, updateable, page numbers (uses page-layout view from v0.3.2), customisable levels, click-to-jump, presets (Modern / Classic / Formal). Persist as `<w:sdt>` in DOCX. |
| IMP-05 | Footnotes & endnotes | New `src/document/Notes.h/.cpp`, `src/editor/extensions/footnotes.js`, `src/editor/extensions/endnotes.js`. Auto-numbering (Arabic/Roman), click-to-jump. Persist as `<w:footnotes>`/`<w:endnotes>`. |
| IMP-06 | Citations / bibliography | New `src/document/CitationStore.h/.cpp`, `src/ui/dialogs/CitationDialog.cpp/.h`. APA, MLA, Chicago, IEEE styles (CSL JSON). Import from Zotero RDF / BibTeX. Auto-update bibliography. |
| IMP-07 | Equation editor | New `src/editor/extensions/equations.js` (KaTeX render). Inline + display. Common templates (fraction, integral, matrix, sqrt, sum). MathML export for DOCX `<m:oMath>`. |
| IMP-08 | Watermarks | New `src/document/Watermark.h/.cpp`, `src/ui/dialogs/WatermarkDialog.cpp/.h`. Text (Confidential / Draft / Sample + custom), image, diagonal/horizontal, opacity/size. Persist as `<w:pict>`/`<w:hdr>` overlay. |
| IMP-09 | Section breaks / column layout | Extend `src/formats/docx_hf.cpp` from v0.3.2. Section types: next-page, continuous, even, odd. Per-section margins + orientation. Multi-column (2/3/custom) via `<w:cols>`. Column breaks. |
| IMP-10 | Drawing / shapes | New `src/editor/extensions/shapes.js` (SVG-backed drawing layer). Rectangles, circles, arrows, lines, callouts. Fill, outline, shadow, effects. Text-in-shape. SmartArt templates (org chart, process, list). |
| CRIT-04 (final) | 90 %+ DOCX fidelity | Public MS-DOCX corpus round-trip byte-stable. |

**Exit criteria**

- Document outline auto-populates and navigates
- Comments anchor and persist through save/reopen
- Track-changes markup shows insertion/deletion; accept/reject works
- TOC auto-generates and updates
- Footnotes/endnotes round-trip through DOCX
- Citation insertion produces a formatted bibliography
- Equations render and copy as MathML
- Watermarks persist in DOCX
- Section breaks and multi-column layout work
- Shape drawing layer functional; at least 5 SmartArt templates
- Test coverage ≥ 80 %

## v0.3.4 — Phase 4: Make it ship

Closes all 🔵 PROD items + templates + file association:

| ID | Item | Concrete deliverable |
|---|---|---|
| PROD-01 | CI/CD pipeline | New workflows: `.github/workflows/windows.yml`, `macos.yml`, `linux.yml`. `ctest` on every push, MSVC/Clang/GCC matrices, automated installer + release artefact upload. |
| PROD-02 | Code signing | Sign Windows binary + NSIS installer (Authenticode). Sign + notarise macOS .dmg. Linux packaging: AppImage, .deb, .rpm. |
| PROD-03 | Auto-update | Polish existing `UpdateService.cpp`. Daily check, delta downloads, rollback on failure, user-confirmation flow with changelog. |
| PROD-04 | Crash reporting | New `src/util/CrashHandler.h/.cpp`. Minidumps (Win) / core dumps (*nix). Opt-in submission with stack + version. |
| PROD-05 | Performance | Profile cold start (< 2 s target). Virtualise rendering for > 100-page docs. Optimise TipTap JSON ser/de. Stress test with synthetic 500-page doc. |
| PROD-06 | Accessibility | `QAccessible` widgets across Ribbon/Tabs/Sidebar. Keyboard-only navigation. High-contrast theme variant. UI font scaling. Alt-text enforcement. WCAG 2.1 AA self-audit. |
| PROD-07 | Internationalisation | `lupdate`/`lrelease` workflow. Extract every visible string to `resources/i18n/*.ts`. RTL (Arabic/Hebrew). CJK IME. Locale-aware numbers/dates. Initial translations: en, es, fr, de, pt, zh, ja. |
| PROD-08 | CDN removal (close-out) | Final audit: zero matches on `grep -r "cdn\\.\\|googleapis\\|gstatic" src/editor/`. All assets in `.qrc`. DevTools shows zero network requests. |
| POLISH-02 (part) | Templates | Ship 20+ templates in `resources/templates/`. Wire to welcome screen with thumbnails. File associations in `installer/vela-docs.nsi`. |

**Exit criteria**

- CI green on Windows/macOS/Linux; installer artefact per push
- Signed installer passes Windows SmartScreen
- Auto-update delivers a test release end-to-end
- Minidumps captured on deliberate crash
- Cold start < 2 s; 500-page doc scrolls smoothly
- WCAG 2.1 AA: zero critical/serious issues
- UI translates to en/es/fr/de/pt/zh/ja; RTL renders
- 20+ templates visible on welcome screen
- `.docx` double-click opens in Vela Docs

## v0.3.5 — Phase 5: Make it special

Differentiation — does not block ship:

| ID | Item | Effort |
|---|---|---|
| POLISH-01 | Offline-first philosophy | 2–3 w |
| POLISH-02 advanced | Templates advanced (categories, search, user-created, variables) | 2–3 w |
| POLISH-03 | Export quality (PDF/A, EPUB, clean HTML, full Markdown) | 3–4 w |
| POLISH-04 | Mail merge | 3–4 w |
| POLISH-05 | Document comparison | 2–3 w |
| POLISH-06 | Macro / scripting support | 4–6 w |

## Total timeline

| Sub-version | Weeks |
|---|---|
| v0.3.1 | 1–3 |
| v0.3.2 | 4–9 |
| v0.3.3 | 10–15 |
| v0.3.4 | 16–19 |
| v0.3.5 | ongoing |

Total to a shippable v0.3.4: ~19 weeks (~4.5 months).
