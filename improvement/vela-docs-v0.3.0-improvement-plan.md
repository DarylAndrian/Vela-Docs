# Vela Docs v0.3.0 — Improvement Plan

> **Benchmark:** Microsoft Word, Google Docs
> **Goal:** Offline-first word processor that can genuinely replace Word for everyday users
> **Date:** 2026-06-30
> **Status:** Planning

---

## Table of Contents

- [Executive Summary](#executive-summary)
- [Current State Assessment](#current-state-assessment)
- [🔴 Critical Issues (App Is Barely Functional)](#-critical-issues-app-is-barely-functional)
- [🟠 Major Gaps (Features Every User Expects)](#-major-gaps-features-every-user-expects)
- [🟡 Important Features (Good → Great)](#-important-features-good--great)
- [🔵 Production Readiness (What's Needed to Ship)](#-production-readiness-whats-needed-to-ship)
- [🟢 Polish & Differentiation (What Makes Users Switch)](#-polish--differentiation-what-makes-users-switch)
- [Playground vs Production Gap](#playground-vs-production-gap)
- [Recommended Implementation Order](#recommended-implementation-order)
- [Phase Checklists](#phase-checklists)

---

## Executive Summary

Vela Docs has a solid architectural foundation — **Qt 6.11 + TipTap/ProseMirror via QtWebEngine** is a sound hybrid approach for an offline word processor. However, the **production app is a skeleton**. The `npm run dev` playground is significantly more functional than the shipped `.exe`. The test suite is essentially 5 tests across the entire application.

The single highest-impact action is: **port every TipTap extension from the playground into production and wire up the UI components that are already designed but not connected.** That alone would 3x the feature set with minimal architectural work.

---

## Current State Assessment

### Architecture
| Layer | Technology |
|-------|-----------|
| Application framework | Qt 6.11 (Core, Gui, Widgets, Network, Sql, PrintSupport, Svg, Xml) |
| Editor rendering | Qt WebEngine (Chromium) + QWebChannel |
| Rich-text engine | TipTap v2 / ProseMirror (JS, in-browser) |
| Markdown parsing | cmark-gfm 0.29.0 (C library, via FetchContent) |
| Document formats | DOCX (primary), HTML, Markdown, ODT, RTF, PDF export |
| Testing | Catch2 v3.5.0 |
| Installer | NSIS (Windows only) |
| Editor playground | Vite + vanilla JS |

### Feature Inventory
| Category | Status |
|----------|--------|
| Basic formatting (B/I/U/S) | ✅ Working in production |
| Headings (H1-H3) | ✅ Working in production |
| Text alignment | ✅ Working in production |
| Bullet/ordered lists | ✅ Working in production |
| Links | ✅ Working in production |
| Dark/light theme | ⚠️ CSS media query only (no toggle) |
| Multi-tab documents | ❌ Single "Welcome" tab |
| Auto-save / crash recovery | ❌ Not implemented |
| Find & Replace | ❌ UI designed, not wired |
| Page layout view | ❌ Not implemented |
| Headers / Footers | ❌ Not implemented |
| Tables | ❌ In playground only |
| Images | ❌ In playground only |
| Highlights | ❌ In playground only |
| Task lists | ❌ In playground only |
| Spell check | ⚠️ Class exists, integration unclear |
| Track changes | ❌ Not implemented |
| Comments | ❌ UI designed, not wired |
| Print support | ⚠️ Service exists, unknown quality |
| File association (.docx) | ❌ Not registered |
| Code signing | ❌ Not implemented |
| Auto-update | ⚠️ Service class exists, unknown status |
| Test coverage | ❌ 5 tests total |

### Codebase Inventory

**Source modules (46 C++ files, 11 JS files):**

| Directory | Classes | Purpose |
|-----------|---------|---------|
| `src/app/` | Application, MainWindow, EditorBridge, Ribbon, TabBar, LeftRail, Sidebar, StatusBar | App shell, UI chrome |
| `src/document/` | Document, DocumentController | Document model, undo/redo |
| `src/editor/` | index.html (production TipTap) | Web-based editor surface |
| `src/formats/` | DocxExporter, DocxImporter, HtmlExporter, HtmlImporter, MarkdownImporter, OdtImporter, PdfExporter, RtfImporter, LegacyDocBridge | Format converters |
| `src/services/` | SpellChecker, RevisionStore, SettingsStore, UpdateService, PrintService, LibreOfficeHelper | Background services |
| `src/ui/` | PreferencesDialog, PageSetupDialog, FindReplacePanel, AboutDialog | Qt dialogs |
| `src/util/` | Zip, FileWatcher, RecentFiles, Platform | Utilities |

---

## 🔴 Critical Issues (App Is Barely Functional)

### CRIT-01: Production Editor Missing 9 TipTap Extensions

**The #1 quick win.** The playground (`editor-playground/src/main.js`) loads 14 TipTap extensions. The production app (`src/editor/index.html`) loads only 5. Features that exist in the playground but NOT in the production .exe:

| Extension | Playground | Production |
|-----------|:---:|:---:|
| starter-kit | ✅ | ✅ |
| underline | ✅ | ✅ |
| text-align | ✅ | ✅ |
| link | ✅ | ✅ |
| placeholder | ✅ | ✅ |
| **highlight** | ✅ | ❌ |
| **image** | ✅ | ❌ |
| **table** | ✅ | ❌ |
| **table-row** | ✅ | ❌ |
| **table-header** | ✅ | ❌ |
| **table-cell** | ✅ | ❌ |
| **task-list** | ✅ | ❌ |
| **task-item** | ✅ | ❌ |
| **character-count** | ✅ | ❌ |

**Additionally, the production app is missing these commands that the playground has:**
- `taskList`, `blockquote`, `codeBlock` toggles
- `indent` / `outdent`
- `insertLink` with prompt dialog
- `textColor` (8-color picker)
- `highlight` (8-color picker)
- `openInsertTable` (modal → insert table)
- `openInsertImage` (modal → insert image)
- `pageBreak`, `hr`
- `footnote` insertion
- `insertChar` (special characters)
- 10 table operations (addRowBefore, addRowAfter, deleteRow, addColBefore, addColAfter, deleteCol, mergeCells, splitCell, toggleHeader, deleteTable)
- `clearFormatting`
- `export` (download as HTML)
- `clear` (clear document)
- `toggleTheme` (dark/light mode)
- Ribbon tab switching (Home / Insert / Table)
- Font family and font size selectors
- Active state tracking (buttons highlight when format is active)
- Cell coordinate display
- Reading time estimate

**Action:**
1. Set up a proper Vite/Rollup build step to produce a single JS bundle for the production editor
2. Port all 14 extensions from playground to production
3. Port the modal system (`modals.js`) for table/image/link insertion
4. Port the full command dispatcher from playground
5. Remove CDN dependencies (the production app currently requires internet to load TipTap from `cdn.jsdelivr.net` — unacceptable for an "offline" word processor)

**Priority:** 🔴 Critical — **Do this first**
**Effort:** 1-2 weeks

---

### CRIT-02: Single Document Tab — No Multi-Document Support

`TabBar` is Phase 0: a single "Welcome" tab with a disabled "+" button. Word and Docs both support multiple documents. You can't even open a second file without closing the first.

**Action:**
1. Implement full multi-tab document support
2. Per-tab `DocumentController` with independent undo/redo stacks
3. Dirty state tracking per tab (unsaved indicator)
4. Tab reordering via drag
5. Tab close button with "save before closing?" prompt
6. "Open in new tab" vs "Open in current tab" behavior

**Priority:** 🔴 Critical
**Effort:** 2-3 weeks

---

### CRIT-03: No Auto-Save / Crash Recovery

If the app crashes, all work is lost. Word auto-saves every few minutes. Google Docs saves on every keystroke.

**Action:**
1. Implement periodic auto-save (configurable, default 30 seconds)
2. Save to a temp file in the app data directory (not the user's file)
3. On restart after crash, detect temp files and offer to recover
4. Use the existing `RevisionStore` (SQLite) for revision snapshots
5. Show "Last saved: 2 minutes ago" in the status bar
6. Add auto-save toggle in Preferences

**Priority:** 🔴 Critical
**Effort:** 1-2 weeks

---

### CRIT-04: Format Import/Export Is Barely Working

Only DOCX round-trip has tests (3 tests). The importers for HTML, Markdown, ODT, and RTF exist as classes but have zero tests. No DOCX styles/themes support. No `.doc` import.

**Action:**
1. Add comprehensive test suites for every format converter
2. Prioritize DOCX fidelity:
   - Styles (Normal, Heading 1-6, Title, Subtitle, Quote, List Paragraph)
   - Headers and footers
   - Page breaks
   - Embedded images (inline and floating)
   - Tables (borders, cell shading, merged cells)
   - Font embedding/substitution
   - Track changes (import at minimum)
3. Improve HTML import/export to handle CSS styles
4. Improve Markdown import/export to handle all elements
5. ODT: handle styles, images, tables
6. RTF: handle character formatting, paragraphs, tables
7. Consider adding `.doc` import via LibreOffice conversion (existing `LibreOfficeHelper`)

**Priority:** 🔴 Critical — Format compatibility is the #1 reason users will accept or reject Vela Docs
**Effort:** 4-6 weeks

---

### CRIT-05: Virtually Zero Test Coverage

| What's Tested | Tests |
|---------------|-------|
| `Zip::write` / `Zip::openForReading` | 2 tests |
| `DocxExporter` → `DocxImporter` round-trip | 3 tests |
| **Total** | **5 tests** |

| What's NOT Tested |
|-------------------|
| HtmlImporter, HtmlExporter, MarkdownImporter, OdtImporter, RtfImporter, PdfExporter |
| SpellChecker, RevisionStore, SettingsStore, UpdateService, PrintService |
| Document model, DocumentController (undo/redo) |
| EditorBridge (C++ ↔ JS communication) |
| MainWindow, Ribbon, TabBar, LeftRail, Sidebar, StatusBar |
| All UI dialogs |
| All utility classes |

**Action:**
1. Target minimum 60% code coverage
2. Priority order:
   - Format converters (failing = data loss)
   - Document model (failing = corruption)
   - EditorBridge (failing = broken UI)
   - Services (failing = broken features)
   - UI (failing = broken UX)
3. Add integration tests for end-to-end document workflows
4. Add UI tests using Qt Test

**Priority:** 🔴 Critical
**Effort:** Ongoing, 2 weeks initial sprint

---

### CRIT-06: Editor Bridge Has No Error Handling

`EditorBridge` uses `runJavaScript("window.__exec('cmd')")` fire-and-forget. No return values, no error callbacks, no confirmation that commands executed. If JavaScript throws, C++ never knows.

**Action:**
1. Implement proper async callbacks via QWebChannel for all bridge calls
2. Every bridge call should return success/failure
3. Add error logging and user-visible error reporting
4. Add timeout handling for unresponsive JS calls

**Priority:** 🔴 Critical
**Effort:** 1 week

---

## 🟠 Major Gaps (Features Every User Expects)

### MAJOR-01: Headers, Footers, and Page Numbers

Not implemented. Every school paper, business document, and report needs these. Word has had them since the 1980s.

**Action:**
1. Implement header/footer editing UI (double-click top/bottom of page)
2. Page number insertion with format options (1, Page 1, 1 of N, Roman numerals)
3. Different first page header/footer
4. Odd/even page headers/footers
5. Section-specific headers/footers
6. Persist in DOCX format

**Priority:** 🟠 Major
**Effort:** 3-4 weeks

---

### MAJOR-02: Page Layout / Print Layout View

No WYSIWYG page view. The editor is a continuous scroll web view. Word shows actual page boundaries with margins, headers/footers, and page breaks. The `PageSetupDialog` exists in the UI system but isn't wired up.

**Action:**
1. Implement page boundary rendering in the editor
2. Show margins with draggable margin markers
3. Page break visualization (both manual and automatic)
4. Ruler with margin/indent/tab stops
5. Wire up the existing `PageSetupDialog`
6. Support portrait/landscape per section
7. Print preview

**Priority:** 🟠 Major
**Effort:** 4-6 weeks

---

### MAJOR-03: Find & Replace

The `FindReplacePanel` exists in the stitch UI system but is not fully integrated.

**Action:**
1. Wire up the existing FindReplacePanel UI
2. Implement find with highlight all matches
3. Implement replace / replace all
4. Add regex support
5. Add "match case" and "whole word" options
6. Add "find in selection" option
7. Show match count ("3 of 17 matches")
8. Keyboard shortcuts: Ctrl+F (find), Ctrl+H (replace), F3 (find next)

**Priority:** 🟠 Major
**Effort:** 1-2 weeks

---

### MAJOR-04: Styles System

Word's killer feature. The Ribbon has a heading selector, but there's no styles pane, no custom styles, no style inheritance, no "Update Style to Match Selection."

**Action:**
1. Implement built-in styles: Normal, Heading 1-6, Title, Subtitle, Quote, List Paragraph, Code
2. Implement styles pane (sidebar or floating panel)
3. Allow custom style creation/editing
4. "Update Style to Match Selection" — modify a style based on current formatting
5. Style inheritance (e.g., Heading 2 inherits from Heading 1)
6. Style sets (collections of styles that can be applied together)
7. Persist styles in DOCX format (w:styles.xml)

**Priority:** 🟠 Major
**Effort:** 3-4 weeks

---

### MAJOR-05: Table Support

TipTap table extensions exist in the playground but aren't in production. Even when enabled, you need advanced table features.

**Action:**
1. Port table extensions to production (see CRIT-01)
2. Add visual column/row resize handles (drag to resize)
3. Add table properties dialog:
   - Table width and alignment
   - Cell padding and spacing
   - Border style, color, width
   - Cell background/shading
4. Table auto-fit (fit to window, fit to contents, fixed column width)
5. Cell merge/split with visual indicators
6. Nested tables (Word supports this)
7. Table caption support
8. Convert table to text and text to table
9. Table sorting

**Priority:** 🟠 Major
**Effort:** 3-4 weeks

---

### MAJOR-06: Image Support

TipTap image extension exists in playground but isn't in production. Beyond basic insertion:

**Action:**
1. Port image extension to production (see CRIT-01)
2. Resize handles with aspect ratio lock
3. Text wrapping modes:
   - In line with text
   - Square wrap
   - Tight wrap
   - Behind text
   - In front of text
4. Image cropping
5. Alt text for accessibility
6. Caption support
7. Drag-and-drop from file manager
8. Paste from clipboard (screenshot, copied image)
9. Image properties dialog (size, position, wrapping, borders, effects)
10. Image compression options (for file size management)

**Priority:** 🟠 Major
**Effort:** 3-4 weeks

---

### MAJOR-07: Spell & Grammar Checking

`SpellChecker` class exists but integration is unclear.

**Action:**
1. Implement real-time spell checking with red wavy underlines
2. Implement grammar checking with blue wavy underlines
3. Right-click context menu: suggestions, "Add to Dictionary", "Ignore All"
4. Custom dictionary management
5. Multi-language support (at minimum: English, Spanish, French, German, Portuguese)
6. Integrate with existing Hunspell dictionaries (likely already bundled in `resources/dictionaries/`)
7. Spell check dialog (F7 shortcut)
8. AutoCorrect options (capitalize first letter, fix common typos, smart quotes)

**Priority:** 🟠 Major
**Effort:** 2-3 weeks

---

### MAJOR-08: Undo/Redo Completeness

`DocumentController` has undo/redo but it's unclear if it covers:
- Formatting changes
- Image insertions/deletions
- Table modifications
- Style changes
- Multi-step operations (e.g., "replace all")

**Action:**
1. Audit all operations to ensure they're captured in the undo stack
2. Ensure undo/redo works across all document elements
3. Add undo history panel (show list of operations)
4. Add Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z keyboard shortcuts
5. Ensure undo state persists across save/load (undo after Ctrl+Z should work)

**Priority:** 🟠 Major
**Effort:** 1-2 weeks

---

### MAJOR-09: Comprehensive Keyboard Shortcuts

No comprehensive keyboard shortcut system visible. The production app only has Ctrl+S.

**Action:**
1. Implement the full shortcut set from the playground:
   - Ctrl+B (bold), Ctrl+I (italic), Ctrl+U (underline)
   - Ctrl+Z (undo), Ctrl+Y (redo), Ctrl+Shift+Z (redo)
   - Ctrl+S (save), Ctrl+Shift+S (save as)
   - Ctrl+O (open), Ctrl+N (new)
   - Ctrl+P (print), Ctrl+F (find), Ctrl+H (replace)
   - Ctrl+A (select all), Ctrl+Home/End (document start/end)
   - Ctrl+K (insert link)
   - Ctrl+Shift+L (bullet list), Ctrl+Shift+O (ordered list)
   - Ctrl+1-4 (heading levels), Ctrl+0 (normal paragraph)
   - F7 (spell check)
   - Ctrl+]/[ (indent/outdent)
2. Add keyboard shortcuts customization in Preferences
3. Show shortcut keys in tooltips
4. Ensure shortcuts work both in Qt shell AND propagate to TipTap

**Priority:** 🟠 Major
**Effort:** 1 week

---

## 🟡 Important Features (Good → Great)

### IMP-01: Document Outline / Navigation Pane

`LeftRail` has an Outline button and `Sidebar` exists, but the outline needs to auto-populate from headings.

**Action:**
1. Auto-populate outline from heading elements (H1-H6)
2. Show hierarchy (indented tree)
3. Click to navigate to heading
4. Real-time update as document changes
5. Drag headings to reorder sections (optional, advanced)
6. Collapse/expand sections

**Priority:** 🟡 Important
**Effort:** 1-2 weeks

---

### IMP-02: Comments / Review System

`CommentThreadUI` exists in the stitch system.

**Action:**
1. Wire up the existing CommentThreadUI
2. Inline comment anchoring to text ranges
3. Comment threads with replies
4. Resolve/reopen comments
5. Comment persistence in DOCX format
6. Comment sidebar panel
7. "New Comment" in right-click context menu
8. Ctrl+Alt+M shortcut

**Priority:** 🟡 Important
**Effort:** 2-3 weeks

---

### IMP-03: Revision History / Track Changes

`RevisionStore` (SQLite) exists but UI is not wired.

**Action:**
1. Wire up the RevisionHistoryPanel
2. Browse revisions with timestamp and diff view
3. Restore a previous revision
4. Track changes mode:
   - Insertions shown in color with underline
   - Deletions shown in color with strikethrough
   - Formatting changes marked
5. Accept/reject individual changes
6. Accept/reject all changes
7. Compare two documents side by side

**Priority:** 🟡 Important
**Effort:** 4-5 weeks

---

### IMP-04: Table of Contents

**Action:**
1. Auto-generate TOC from headings
2. Updateable (refresh when document changes)
3. Page numbers (requires page layout view)
4. Customizable levels (show H1-H3 only, etc.)
5. Click TOC entry to navigate
6. TOC styles (Modern, Classic, Formal, etc.)
7. Persist in DOCX format

**Priority:** 🟡 Important
**Effort:** 2-3 weeks

---

### IMP-05: Footnotes & Endnotes

**Action:**
1. Insert footnote (Ctrl+Alt+F)
2. Insert endnote (Ctrl+Alt+D)
3. Auto-numbering (1, 2, 3... or i, ii, iii...)
4. Footnote area at bottom of page
5. Endnote area at end of document
6. Click footnote number to jump between reference and footnote text
7. Custom numbering format options
8. Persist in DOCX format

**Priority:** 🟡 Important
**Effort:** 2-3 weeks

---

### IMP-06: Citations / Bibliography

**Action:**
1. Citation insertion dialog (APA, MLA, Chicago, IEEE styles)
2. Bibliography/Works Cited auto-generation
3. Import from Zotero/BibTeX
4. Citation manager (add, edit, delete sources)
5. Auto-update bibliography when citations change

**Priority:** 🟡 Important
**Effort:** 3-4 weeks

---

### IMP-07: Equation Editor

**Action:**
1. LaTeX input field (inline and display mode)
2. Live preview of rendered equation
3. Common equation templates (fraction, integral, matrix, etc.)
4. MathML export for DOCX compatibility
5. KaTeX or MathJax rendering in the editor

**Priority:** 🟡 Important
**Effort:** 2-3 weeks

---

### IMP-08: Watermarks

**Action:**
1. Text watermarks (confidential, draft, sample, custom)
2. Image watermarks
3. Diagonal, horizontal, or custom placement
4. Opacity and size controls
5. Persist in DOCX format

**Priority:** 🟡 Important
**Effort:** 1 week

---

### IMP-09: Section Breaks / Column Layout

**Action:**
1. Section break types: next page, continuous, even page, odd page
2. Per-section page orientation (portrait/landscape)
3. Per-section margins
4. Multi-column layout (2, 3, or custom)
5. Column breaks
6. Persist in DOCX format

**Priority:** 🟡 Important
**Effort:** 3-4 weeks

---

### IMP-10: Drawing / Shapes

**Action:**
1. Basic shapes: rectangles, circles, arrows, lines, callouts
2. Shape formatting: fill color, outline, shadow, effects
3. Text in shapes
4. Shape connectors (for flowcharts)
5. Canvas/drawing layer
6. SmartArt equivalent (org charts, process diagrams, lists)

**Priority:** 🟡 Important
**Effort:** 4-6 weeks

---

## 🔵 Production Readiness (What's Needed to Ship)

### PROD-01: No CI/CD Pipeline

No GitHub Actions, no automated builds, no automated tests running on push.

**Action:**
1. Set up GitHub Actions for:
   - Windows build (MSVC + Qt 6.11)
   - macOS build (Clang + Qt 6.11)
   - Linux build (GCC + Qt 6.11)
2. Run test suite on every push/PR
3. Automated installer generation
4. Automated release publishing

**Priority:** 🔵 Production
**Effort:** 1 week

---

### PROD-02: No Code Signing

The NSI installer doesn't include code signing. Windows SmartScreen will block the app. macOS Gatekeeper will refuse to open it.

**Action:**
1. Obtain a code signing certificate (Windows Authenticode)
2. Obtain an Apple Developer ID certificate (macOS)
3. Sign the executable and installer
4. Notarize the macOS build with Apple
5. Consider Linux packaging (AppImage, .deb, .rpm)

**Priority:** 🔵 Production
**Effort:** 1 week (+ certificate procurement time)

---

### PROD-03: No Auto-Update Mechanism

`UpdateService` class exists but implementation status is unknown.

**Action:**
1. Implement background update checks (daily, configurable)
2. Show "Update available" notification with changelog
3. Download and install updates with user confirmation
4. Delta updates (don't re-download the entire app)
5. Rollback capability if update fails

**Priority:** 🔵 Production
**Effort:** 2-3 weeks

---

### PROD-04: No Crash Reporting

If the app crashes, there's no crash dump collection, no telemetry.

**Action:**
1. Implement crash dump collection (minidump on Windows, core dump on Linux/Mac)
2. Optional crash report submission (with user consent)
3. Include system info, app version, and stack trace
4. Integrate with a crash reporting service (Sentry, Bugsnag, or self-hosted)

**Priority:** 🔵 Production
**Effort:** 1-2 weeks

---

### PROD-05: Performance Concerns

**Action:**
1. Profile memory usage (QtWebEngine baseline is ~150MB)
2. Test large document handling (100+ pages, 500+ pages)
3. Optimize TipTap JSON serialization for large documents
4. Implement lazy rendering for long documents
5. Optimize startup time (target: <2 seconds cold start)
6. Profile and optimize format conversion speed

**Priority:** 🔵 Production
**Effort:** 2-3 weeks

---

### PROD-06: Accessibility

**Action:**
1. Screen reader support (Qt Accessibility API)
2. Keyboard-only navigation testing
3. High-contrast mode
4. Font size scaling for UI (not just editor)
5. Focus indicators
6. Alt text enforcement for images
7. WCAG 2.1 AA compliance target

**Priority:** 🔵 Production
**Effort:** 2-3 weeks

---

### PROD-07: Internationalization (i18n)

**Action:**
1. Set up Qt translation framework (Qt Linguist / .ts files)
2. Extract all UI strings for translation
3. RTL language support (Arabic, Hebrew)
4. CJK input method handling (Chinese, Japanese, Korean)
5. Locale-aware date/number formatting
6. Initial translations: English, Spanish, French, German, Portuguese, Chinese, Japanese

**Priority:** 🔵 Production
**Effort:** 3-4 weeks

---

### PROD-08: CDN Dependency Removal

The production editor currently loads TipTap from `cdn.jsdelivr.net`. This means the "offline" word processor **requires internet on first load** (or relies on browser cache).

**Action:**
1. Bundle all JS dependencies locally (Vite/Rollup build)
2. Embed the bundle in the Qt resource system (.qrc)
3. Remove all external CDN `<script>` tags
4. Remove external Google Fonts CDN (bundle fonts locally)
5. Verify zero network requests are made during normal operation

**Priority:** 🔵 Production (blocks the "offline" value proposition)
**Effort:** 1 week (combined with CRIT-01)

---

## 🟢 Polish & Differentiation (What Makes Users Switch)

### POLISH-01: Offline-First Philosophy

No accounts, no sync, no cloud dependency. Lean into this as a feature.

**Action:**
1. Robust local file management
2. Recent documents with content thumbnails (partially exists in welcome screen)
3. File association registration (.docx, .odt, .rtf, .md, .txt)
4. "Open with Vela" right-click context menu integration
5. Quick launch / fast startup (target: <2 seconds)
6. Portable mode (run from USB drive)
7. No telemetry by default (opt-in only)

**Priority:** 🟢 Polish
**Effort:** 2-3 weeks

---

### POLISH-02: Template System

`resources/templates/` exists but is empty. The welcome screen shows template cards but they're placeholders.

**Action:**
1. Create 20-30 quality templates:
   - Blank, Resume (2-3 styles), Letter (formal, informal), Report, Invoice
   - Newsletter, Flyer, Brochure, Agenda, Meeting Minutes
   - Research Paper, Thesis, Essay, Lab Report
   - Budget, Calendar, Schedule, To-Do List
   - Business Card, Envelope, Label
2. Template preview thumbnails
3. Template categories and search
4. User-created templates (save current document as template)
5. Template variables (name, date, etc. that prompt on creation)

**Priority:** 🟢 Polish
**Effort:** 2-3 weeks

---

### POLISH-03: Export Quality

**Action:**
1. PDF export: bookmarks, hyperlinks, table of contents, embedded fonts, PDF/A compliance
2. DOCX export: preserve all styles, headers/footers, images, tables, comments, track changes
3. HTML export: clean semantic markup, inline styles, responsive
4. Markdown export: proper handling of all elements (tables, images, code blocks, etc.)
5. EPUB export: chapter splitting, cover image, metadata, TOC
6. ODT export: basic fidelity

**Priority:** 🟢 Polish
**Effort:** 3-4 weeks

---

### POLISH-04: Mail Merge

**Action:**
1. Data source connection (CSV, Excel, SQLite)
2. Merge field insertion ({First Name}, {Address}, etc.)
3. Preview merged document
4. Print merged documents
5. Save individual merged documents
6. Label and envelope printing

**Priority:** 🟢 Polish
**Effort:** 3-4 weeks

---

### POLISH-05: Document Comparison

**Action:**
1. Open two documents side by side
2. Highlight differences (insertions, deletions, modifications)
3. Navigate between differences
4. Merge changes from either document
5. Generate comparison report

**Priority:** 🟢 Polish
**Effort:** 2-3 weeks

---

### POLISH-06: Macro / Scripting Support

**Action:**
1. Simple JavaScript macro recording and playback
2. Macro editor with syntax highlighting
3. Pre-built macros (format table, batch find/replace, etc.)
4. Macro storage and management
5. Keyboard shortcut assignment to macros

**Priority:** 🟢 Polish
**Effort:** 4-6 weeks

---

## Playground vs Production Gap

### The Core Problem

The `editor-playground/` was developed as a standalone web prototype with its own `package.json`, Vite bundler, and full feature set. When the production app was built (`src/editor/index.html`), it was simplified to use CDN UMD imports, and only the basic extensions were ported. The richer extensions and the modal system were never integrated.

### Feature Comparison

| Feature | Playground (`npm run dev`) | Production (.exe) |
|---------|:---:|:---:|
| Tables (full CRUD + merge/split) | ✅ | ❌ |
| Images (insert via modal) | ✅ | ❌ |
| Highlights (8 colors) | ✅ | ❌ |
| Task lists (with nesting) | ✅ | ❌ |
| Character count | ✅ | ❌ |
| Text color (8-color picker) | ✅ | ❌ |
| Code blocks | ✅ | ❌ |
| Blockquotes | ✅ | ❌ |
| Indent/Outdent | ✅ | ❌ |
| Clear formatting | ✅ | ❌ |
| Table operations (10 commands) | ✅ | ❌ |
| Export HTML | ✅ | ❌ |
| Theme toggle (dark/light) | ✅ | ❌ |
| Ribbon tabs (Home/Insert/Table) | ✅ 3 tabs | ❌ Single ribbon |
| Keyboard shortcuts | ✅ 15+ shortcuts | ⚠️ Only Ctrl+S |
| Font family selector | ✅ Wired up | ⚠️ Partially wired |
| Font size selector | ✅ Wired up | ❌ Stubbed |
| Modal dialogs (Table/Image/Link) | ✅ | ❌ |
| Active state tracking | ✅ | ❌ |
| Cell coordinate display | ✅ | ❌ |
| Reading time estimate | ✅ | ❌ |

### Solution

1. Set up a proper Vite/Rollup build step that produces a single JS bundle for the production editor
2. Source the editor JS from a shared codebase (or just use the playground code as the production editor)
3. Embed the bundle in Qt resources (.qrc) so no internet connection is needed
4. Wire up the `window.vela` QWebChannel bridge to the production editor

---

## Recommended Implementation Order

| Phase | Focus | Effort | Impact |
|-------|-------|--------|--------|
| **Phase 1: Make it work** | Port all TipTap extensions to production, multi-tab, auto-save, keyboard shortcuts, remove CDN deps | 2-3 weeks | Barely functional → Usable |
| **Phase 2: Make it a word processor** | Headers/footers, page layout, find/replace, styles, tables, images, spell check | 4-6 weeks | Usable → Good |
| **Phase 3: Make it competitive** | Track changes, comments, TOC, footnotes, format fidelity (DOCX), test coverage | 4-6 weeks | Good → Great |
| **Phase 4: Make it ship** | CI/CD, code signing, auto-update, crash reporting, templates, accessibility, i18n | 3-4 weeks | Great → Shippable |
| **Phase 5: Make it special** | Mail merge, citations, equations, drawing, EPUB, document comparison, macros | Ongoing | Shippable → Differentiated |

---

## Phase Checklists

### Phase 1: Make It Work (Weeks 1-3)

- [ ] Set up Vite/Rollup build for production editor JS bundle
- [ ] Port all 9 missing TipTap extensions to production (highlight, image, table, table-row, table-header, table-cell, task-list, task-item, character-count)
- [ ] Port modal system (modals.js) for table/image/link insertion
- [ ] Port full command dispatcher from playground
- [ ] Port keyboard shortcuts from playground
- [ ] Port ribbon tab switching (Home / Insert / Table)
- [ ] Port font family and font size selectors
- [ ] Port active state tracking for toolbar buttons
- [ ] Port color picker (text color and highlight)
- [ ] Embed all JS/fonts in Qt resources — remove CDN dependencies
- [ ] Implement multi-tab document support
- [ ] Implement auto-save with crash recovery
- [ ] Implement basic Ctrl+Z/Y undo/redo propagation
- [ ] Add comprehensive test suite for format converters
- [ ] Add EditorBridge error handling and callbacks

### Phase 2: Make It a Word Processor (Weeks 4-9)

- [ ] Implement headers, footers, and page numbers
- [ ] Implement page layout / print layout view
- [ ] Wire up and implement Find & Replace panel
- [ ] Implement styles system (built-in + custom styles)
- [ ] Implement full table support (resize, properties, merge/split, auto-fit)
- [ ] Implement full image support (resize, wrapping, crop, alt text, captions)
- [ ] Implement spell checking with red/blue wavy underlines
- [ ] Implement grammar checking
- [ ] Implement AutoCorrect
- [ ] Ensure undo/redo covers all operations
- [ ] Add comprehensive keyboard shortcut system
- [ ] Achieve 60% test coverage

### Phase 3: Make It Competitive (Weeks 10-15)

- [ ] Implement document outline / navigation pane
- [ ] Implement comments system (threads, replies, resolve)
- [ ] Implement track changes (insertions, deletions, accept/reject)
- [ ] Implement revision history UI
- [ ] Implement Table of Contents (auto-generated, updateable)
- [ ] Implement footnotes and endnotes
- [ ] Achieve 90%+ DOCX format fidelity (styles, images, tables, headers/footers)
- [ ] Achieve 80% test coverage

### Phase 4: Make It Ship (Weeks 16-19)

- [ ] Set up CI/CD pipeline (GitHub Actions)
- [ ] Implement code signing (Windows + macOS)
- [ ] Implement auto-update mechanism
- [ ] Implement crash reporting
- [ ] Create 20-30 quality templates
- [ ] Implement file association registration (.docx, .odt, .rtf, .md)
- [ ] Implement "Open with Vela" context menu integration
- [ ] Implement accessibility (screen reader, keyboard nav, high-contrast)
- [ ] Implement internationalization framework
- [ ] Performance optimization (startup time, large documents, memory)

### Phase 5: Make It Special (Ongoing)

- [ ] Implement citations / bibliography
- [ ] Implement equation editor (LaTeX input)
- [ ] Implement watermarks
- [ ] Implement section breaks / multi-column layout
- [ ] Implement drawing / shapes
- [ ] Implement mail merge
- [ ] Implement document comparison
- [ ] Implement EPUB export
- [ ] Implement macro / scripting support
- [ ] Implement SmartArt / diagrams
- [ ] Implement citation manager integration (Zotero)
- [ ] Portable mode (USB drive)

---

*This plan is a living document. Update as items are completed and new requirements are discovered.*