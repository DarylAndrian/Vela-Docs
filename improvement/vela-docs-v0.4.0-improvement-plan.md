# Vela Docs v0.4.0 — Improvement Plan

> **Benchmark:** Microsoft Word, Google Docs
> **Theme:** *Refinement & polish — making the basics feel like a real word processor*
> **Date:** 2026-07-01
> **Status:** Planning → Implementation in progress

---

## Table of Contents

- [Executive Summary](#executive-summary)
- [v0.3.5 Status Recap](#v035-status-recap)
- [Goals for v0.4.0](#goals-for-v040)
- [F1 — Files Tab (Microsoft Word Backstage)](#f1--files-tab-microsoft-word-backstage)
- [F2 — Better Icons (Material Symbols)](#f2--better-icons-material-symbols)
- [F3 — Custom Color Picker (HEX + RGB)](#f3--custom-color-picker-hex--rgb)
- [F4 — Heading System (Title, Subtitle, H1–H5, Custom, "Update to Match")](#f4--heading-system-title-subtitle-h1h5-custom-update-to-match)
- [F5 — Tab Stops (Google Docs-style Ruler)](#f5--tab-stops-google-docs-style-ruler)
- [Cleanup — Status Bar & Top Bar](#cleanup--status-bar--top-bar)
- [Implementation Order](#implementation-order)
- [File-Level Change Lists](#file-level-change-lists)
- [Phase Checklists](#phase-checklists)
- [Risks & Open Questions](#risks--open-questions)

---

## Executive Summary

Vela Docs v0.3.5 is a feature-complete word processor. v0.4.0 is the **first polish release** — it does not add new categories of features, it instead makes the existing categories feel right.

The five user-facing improvements in this release were chosen because they address the **most jarring rough edges** that prevent a user from forgetting they're using a young project:

1. **A real "Files" tab** (Microsoft Word backstage) so Save / Save As / Print Preview / Options stop being scattered across a top bar gear icon and a File menu.
2. **Material Symbols** throughout so the alignment/list/heading buttons stop being letters (`L`, `C`, `R`, `J`, `UL`, `OL`, `¶`, `H1`) and start being recognizable icons.
3. **A real color picker** so the user can pick *any* color (HEX or RGB), not just one of eight fixed swatches.
4. **A complete heading system** — Title, Subtitle, Heading 1–5 out of the box; user-creatable custom levels; "Update 'X' to match selection" so styles can be tweaked document-wide by example.
5. **Tab stops** with a draggable ruler so the editor behaves like Google Docs for tables of contents, formal letters, and column-aligned layouts.

Plus a small **cleanup pass** that removes the "Phase 0" badge and wires the zoom slider, language label, and page-info widget in the status bar.

This is a **6–8 week release**, dominated by the heading system (2–3 weeks) and the backstage widget (1 week). The other items are each ≤ 1 week.

---

## v0.3.5 Status Recap

| Area | v0.3.5 state |
|---|---|
| Architecture | Qt 6.11 + QWebEngine + TipTap/ProseMirror (no CDN) |
| Editing features | All 14 TipTap extensions, multi-tab, auto-save, headers/footers, find/replace, styles, tables, images, spell-check, comments, track changes, TOC, footnotes, citations, equations, watermarks, sections, shapes |
| Format fidelity | 90 %+ DOCX round-trip |
| Production polish | Templates shipped, CI matrix, code signing, auto-update, crash reports, i18n, accessibility WCAG 2.1 AA, mail merge, document comparison, EPUB export, macro scripting |
| Known rough edges | No backstage UI; text-label icons in toolbar; fixed 8-color swatches; limited heading set; no tab stops; placeholder "Phase 0" badge in status bar |

---

## Goals for v0.4.0

| # | Goal | User-visible win |
|---|---|---|
| G1 | Replace the File menu with a Microsoft Word-style **Files (backstage) tab** | "Open the app, hit `Alt+F`, and it feels right." |
| G2 | Replace text-label buttons with **Material Symbols Outlined icons** | "I don't have to read the tooltips to know what `L` does." |
| G3 | Add a **custom color picker** with HEX + RGB inputs | "I can pick the exact brand color, not the closest swatch." |
| G4 | Ship a full **heading system** with Title, Subtitle, H1–H5, custom levels, and **"Update 'X' to match selection"** | "I can restyle every heading at once by formatting one paragraph." |
| G5 | Add **tab stops** with a draggable ruler | "I can build a real table of contents and column-aligned lists." |
| G6 | **Clean up** the status bar and top bar | "Nothing in the UI looks broken or stubbed-out anymore." |

---

## F1 — Files Tab (Microsoft Word Backstage)

### Behavior

A colored **"📄 Files"** tab at the **far-left of the ribbon** (Word 2010+ style). Clicking or pressing `Alt+F` opens a full-width **Backstage overlay** that slides over the editor area (animated, ~180 ms slide).

```
┌────────────────────────────────────────────────────────────────────┐
│  📄 Files │  Home │  Insert │  Table │  ────  [↶] [↷]   [◐]      │  ← Files tab tinted in accent color when active
├────────────────────────────────────────────────────────────────────┤
│ ┌─────────────┬──────────────────────────────────────────────────┐ │
│ │  💾 Save     │                                                  │ │
│ │  📁 Save As  │                                                  │ │
│ │  📂 Open     │   Active panel content                           │ │
│ │  ─────       │                                                  │ │
│ │  🖨 Print    │   • Print Preview renders the document live       │ │
│ │  👁 Preview  │   • Save As shows a file dialog w/ format select  │ │
│ │  ─────       │   • Recent shows a thumbnail grid                │ │
│ │  ⚙ Options   │   • Options opens the existing PreferencesDialog │ │
│ │  ❓ Help      │                                                  │ │
│ │  ─────       │                                                  │ │
│ │  🕘 Recent   │                                                  │ │
│ └─────────────┴──────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

### Keyboard

| Key | Action |
|---|---|
| `Alt+F` | Toggle Files tab |
| `Esc` | Close Files tab |
| `Tab` / `Shift+Tab` | Cycle through rail items |
| `Enter` | Activate focused rail item |
| Arrow keys | Navigate rail |

### Panels

| Panel | Right-pane content |
|---|---|
| **Save** | Summary card: filename, format, last modified. Big "Save" button. If unsaved, says "Save changes to this document?" |
| **Save As** | File-format selector (`.docx` / `.odt` / `.rtf` / `.md` / `.pdf` / `.txt`) + Save button (opens `QFileDialog::getSaveFileName`) |
| **Open** | Recent files grid + "Browse…" button (opens `QFileDialog::getOpenFileName`) |
| **Print** | Printer selector (uses `QPrinterInfo::availablePrinters`), copies, page range, "Print" button |
| **Print Preview** | Embedded `QPrintPreviewWidget` showing the document with margins, headers/footers, page numbers |
| **Options** | Hosts the existing `PreferencesDialog` (already has Documents / Display / Editing / Privacy / Keybindings / Language / Templates / Mail Merge / About tabs from v0.3.4 PROD-06 + PROD-07) |
| **Help** | About + "Open user guide" + "Open changelog" + keyboard shortcuts cheat-sheet |
| **Recent** | Grid of recent files with first-page thumbnails (POLISH-01 from v0.3.5) |

### Implementation

#### New C++ classes

| File | Purpose |
|---|---|
| `src/app/Backstage.h/.cpp` | The overlay widget; manages show/hide animation; owns the rail + content stack |
| `src/app/BackstageRail.h/.cpp` | Vertical icon list on the left side of the backstage |
| `src/app/BackstagePanel.h` | Base `QWidget` subclass for the right-side content |
| `src/app/BackstageSavePanel.{h,cpp}` | Save summary card |
| `src/app/BackstageSaveAsPanel.{h,cpp}` | Format selector + Save button |
| `src/app/BackstageOpenPanel.{h,cpp}` | Recent grid + Browse button |
| `src/app/BackstagePrintPanel.{h,cpp}` | Printer/copies/range selectors + Print button |
| `src/app/BackstagePrintPreviewPanel.{h,cpp}` | Hosts a `QPrintPreviewWidget` |
| `src/app/BackstageOptionsPanel.{h,cpp}` | Hosts the existing `PreferencesDialog` |
| `src/app/BackstageHelpPanel.{h,cpp}` | About, docs link, shortcuts cheat-sheet |
| `src/app/BackstageRecentPanel.{h,cpp}` | Grid of recent files |

#### MainWindow changes

| File | Change |
|---|---|
| `src/app/MainWindow.h` | Add `QPointer<Backstage> m_backstage;` |
| `src/app/MainWindow.cpp` | In `buildRibbon()`, add the Files button at the far-left with accent-color styling. New `void toggleBackstage(const QString& panel = "Save")` slot. Wire `Alt+F` shortcut. |
| `src/app/Ribbon.cpp` | New `QAction* m_actFiles`; positioned at the start with `setIcon()` + custom `QToolBar::setStyleSheet` for accent color when checked |

#### EditorBridge changes

| Method | Purpose |
|---|---|
| `Q_INVOKABLE void openBackstagePanel(const QString& name)` | JS can request a panel change |
| `Q_INVOKABLE void closeBackstage()` | JS can request close (e.g., after Save) |
| `Q_INVOKABLE bool isBackstageVisible()` | For state checks |
| Signal `backstageVisibilityChanged(bool)` | So MainWindow can dim the ribbon |

#### Print preview rendering

Uses `QPrintPreviewDialog` (Qt PrintSupport, already a dependency per `CMakeLists.txt:51`).
The document is rendered via `DocumentController::renderForPrinting(QPrinter*)` —
added in this release as a thin wrapper that converts the current document to HTML
and asks `QWebEngineView::print(QPrinter*)` to render it.

#### Stitch design system

| File | Add |
|---|---|
| `stitch_vela_docs_ui_system/files_backstage/` | New folder with `DESIGN.md`, `code.html` (Tailwind mock), `screen.png` |

---

## F2 — Better Icons (Material Symbols)

### Mapping

| Current text | Material Symbol name | Notes |
|---|---|---|
| `L` (align left) | `format_align_left` | |
| `C` (align center) | `format_align_center` | |
| `R` (align right) | `format_align_right` | |
| `J` (justify) | `format_align_justify` | |
| `UL` (bullet) | `format_list_bulleted` | |
| `OL` (numbered) | `format_list_numbered` | |
| `☐` (task) | `check_box_outline_blank` | Already icon-like — keep |
| `Tx` (clear format) | `format_clear` | |
| `¶` (normal) | `notes` | |
| `H1` … `H5` | `title` | Each level shown as `title` + a subscript number badge |
| 💾 (save) | `save` | |
| ⬇ (export) | `download` | |
| ⌫ (clear) | `delete_sweep` | |
| ◐ (theme) | `light_mode` / `dark_mode` | Swaps on toggle |
| ❓ (help) | `help` | |
| ⚙ (preferences) | `settings` | Moves to Backstage |

### Implementation

#### Playground (HTML/CSS)

The Material Symbols Outlined font is **already bundled** in the Qt resources per the v0.3.4 CHANGELOG ("Material Symbols 3.0.0"). The playground mirrors production.

`editor-playground/src/styles.css` — add at the top:
```css
@font-face {
  font-family: 'Material Symbols Outlined';
  font-style: normal;
  font-weight: 400;
  src: url('/src/assets/material-symbols-outlined.woff2') format('woff2');
}

.tool, .tool-pill, .topbar-btn, .status-bar button {
  font-family: 'Material Symbols Outlined', sans-serif;
  font-weight: normal;
  font-style: normal;
  font-size: 18px;
  line-height: 1;
  letter-spacing: normal;
  text-transform: none;
  display: inline-block;
  white-space: nowrap;
  word-wrap: normal;
  direction: ltr;
}
```

`editor-playground/index.html` — swap text labels:
```html
<button data-cmd="align" data-arg="left"    class="tool" title="Align left">format_align_left</button>
<button data-cmd="align" data-arg="center"  class="tool" title="Align center">format_align_center</button>
<!-- …etc… -->
```

#### Production Qt shell

`src/app/Ribbon.cpp`:
- Switch from `Qt::ToolButtonTextOnly` to `Qt::ToolButtonIconOnly` for formatting buttons
- Bundle SVG icons in `resources/icons/material/` (24×24 viewBox, single-color)
- Each `makeButton` call gets an `addAction`-style icon load via `QIcon(":/icons/material/format_align_left.svg")`

`resources/resources.qrc` — new entries under `<file>icons/material/*.svg</file>`

`stitch_vela_docs_ui_system/main_editor_*/code.html` — update reference HTML to match

---

## F3 — Custom Color Picker (HEX + RGB)

### Current state

`editor-playground/index.html:72-95` has two `<div class="color-menu">` blocks — text color (8 swatches) and highlight (8 swatches). No custom color input.

### New behavior

Replace each `<div class="color-menu">` with `<div class="color-picker" data-target="textColor">` populated by a new module:

```
┌──────────────────────────────────────┐
│  Standard                            │
│  ⚫ ⚫ ⚫ ⚫ ⚫ ⚫ ⚫ ⚫                   │   ← 8 standard palette (preserved)
├──────────────────────────────────────┤
│  Tints                              │
│  ▫ ▫ ▫ ▫ ▫ ▫ ▫ ▫                      │   ← 8 shades of last-used color
├──────────────────────────────────────┤
│  ▾ Custom color                      │   ← expand inline
│  ┌─────────────┐  R [255]  G [  0]  B [  0]   │
│  │  gradient   │  HEX [#FF0000]               │
│  │  picker     │                              │
│  └─────────────┘  [Cancel]    [Apply]         │
├──────────────────────────────────────┤
│  Recent                              │
│  ● ● ● ● ● ● ● ●                      │   ← last 8 custom colors (persisted)
└──────────────────────────────────────┘
```

### Implementation

#### New module: `editor-playground/src/color-picker.js`

Exports:
```js
export function mountColorPicker(target, anchorButton, { onApply, onClose })
```

Internal:
- Builds the popover DOM dynamically
- 8 standard swatches (data list)
- Tints generated via HSL math from the last-applied color
- Custom section: HTML5 `<input type="color">` + 3 numeric inputs + HEX field
- HEX ↔ RGB sync
- Recent colors in `localStorage` (key `vela.recentColors.<target>`)
- Keyboard: `Esc` closes, `Enter` applies, `Tab` cycles inputs

#### Playground HTML

Replace lines 72–95 of `index.html`:
```html
<button data-cmd="textColor" class="tool has-swatch" id="btn-text-color" title="Text color">
  <span class="swatch-letter">format_color_text</span>
  <span class="swatch-bar" style="background: var(--text)"></span>
</button>
<div class="color-picker" data-target="textColor" hidden></div>

<button data-cmd="highlight" class="tool has-swatch" id="btn-highlight" title="Highlight color">
  <span class="swatch-letter" style="background: #fff3a0">border_color</span>
  <span class="swatch-bar" style="background: #fff3a0"></span>
</button>
<div class="color-picker" data-target="highlight" hidden></div>
```

#### main.js wiring

```js
import { mountColorPicker } from './color-picker.js';

mountColorPicker('textColor', document.getElementById('btn-text-color'), {
  onApply: (color) => execCommand('pickColor', { target: 'textColor', color })
});
mountColorPicker('highlight', document.getElementById('btn-highlight'), {
  onApply: (color) => execCommand('pickColor', { target: 'highlight', color })
});
```

#### Production persistence

Recent colors stored in `localStorage` (web) and synced to `%APPDATA%/Vela Docs/recent-colors.json` (production) via `EditorBridge::getRecentColors()` / `setRecentColor(QString,QString)`.

#### DOCX

Text color and highlight already round-trip via `<w:color>` and `<w:highlight>` (v0.3.1 CRIT-04). No DOCX changes needed for this feature.

---

## F4 — Heading System (Title, Subtitle, H1–H5, Custom, "Update to Match")

This is the biggest feature of v0.4.0.

### Built-in styles

| Style | Default visual | Notes |
|---|---|---|
| Normal Text | body | `<w:pStyle w:val="Normal"/>` |
| **Title** | 26pt, bold, centered | `<w:pStyle w:val="Title"/>` |
| **Subtitle** | 16pt, italic, gray | `<w:pStyle w:val="Subtitle"/>` |
| Heading 1 | 22pt, bold | `<w:pStyle w:val="Heading1"/>` |
| Heading 2 | 18pt, bold | `<w:pStyle w:val="Heading2"/>` |
| Heading 3 | 16pt, bold | `<w:pStyle w:val="Heading3"/>` |
| Heading 4 | 14pt, bold | `<w:pStyle w:val="Heading4"/>` |
| Heading 5 | 12pt, bold, uppercase | `<w:pStyle w:val="Heading5"/>` |
| Quote | indented, italic | `<w:pStyle w:val="Quote"/>` |
| Code | monospaced, gray bg | `<w:pStyle w:val="Code"/>` |
| List Paragraph | slight indent | `<w:pStyle w:val="ListParagraph"/>` |

### Custom levels

The dropdown has **`+ New Style…`** at the bottom. Opens an inline editor:

```
┌──────────────────────────────────────┐
│  Create new style                    │
│                                      │
│  Name          [Heading 6        ]   │
│  Based on      [Heading 4       ▼]   │
│  Type          ( ) Inline  (•) Para  │
│  Font family   [Inter          ▼]    │
│  Font size     [18  ] pt              │
│  ☑ Bold   ☐ Italic   ☐ Underline     │
│  Text color    [color chip  ]        │
│  Alignment     (•) Left ( ) Center   │
│                 ( ) Right ( ) Justify │
│  Spacing before [0 ] pt  after [0 ]pt│
│                                      │
│  [Cancel]                  [Create]  │
└──────────────────────────────────────┘
```

User can create **Heading 6, Heading 7, Heading 10, or any name** — e.g. "Caption", "Sidebar heading", "API method".

Custom styles appear under a **`Custom Styles ▸`** submenu in the dropdown, sorted alphabetically.

### "Update '[Style Name]' to match"

When the cursor is in a styled paragraph, the dropdown shows:

```
┌──────────────────────────────────────┐
│  ▸ Active: Heading 1                 │   ← bold; current style
│  Update 'Heading 1' to match         │   ← italic; grayed if no selection
│          selection                   │
│  Modify 'Heading 1'…                 │   ← italic
│  ────                                │
│  ▸ Built-in Styles                   │
│      Normal Text                     │
│      Title                           │
│      …                               │
│  ▸ Custom Styles                     │
│      Heading 6                       │
│      Caption                         │
│  ────                                │
│  + New Style…                        │
└──────────────────────────────────────┘
```

- **"Update 'Heading 1' to match selection"** — captures font family, size, weight, italic, underline, color, alignment, spacing from the cursor paragraph and **writes them into the style**. Every paragraph with that style adopts the new settings.
- **"Modify 'Heading 1'…"** — opens the same style editor as `+ New Style…` but pre-filled with the style's current values, titled "Modify Heading 1".

### DOCX persistence

`DocxExporter` builds a `word/styles.xml` part. v0.3.3 already writes the basic styles (MAJOR-04). v0.4.0 extends:

```xml
<w:style w:type="paragraph" w:styleId="Heading1">
  <w:name w:val="heading 1"/>
  <w:basedOn w:val="Normal"/>
  <w:next w:val="Normal"/>
  <w:qFormat/>
  <w:pPr>
    <w:keepNext/>
    <w:spacing w:before="480" w:after="120"/>
    <w:outlineLvl w:val="0"/>
  </w:pPr>
  <w:rPr>
    <w:rFonts w:ascii="Inter" w:hAnsi="Inter"/>
    <w:b/>
    <w:color w:val="3525CD"/>
    <w:sz w:val="44"/>
  </w:rPr>
</w:style>
```

Custom user-defined styles become:
```xml
<w:style w:type="paragraph" w:customStyle="1" w:styleId="UserStyle1">
  <w:name w:val="Caption"/>
  <w:basedOn w:val="Normal"/>
  <w:qFormat/>
  <!-- …rPr/pPr captured from "Update to match"… -->
</w:style>
```

`DocxImporter::parseStylesXml()` extended to read custom styles so documents round-trip perfectly. Per-paragraph reference: `<w:pStyle w:val="UserStyle1"/>`.

### Implementation

#### New TipTap extensions

| File | Purpose |
|---|---|
| `editor-playground/src/extensions/title.js` | `Title` Node — `<w:pStyle w:val="Title"/>` |
| `editor-playground/src/extensions/subtitle.js` | `Subtitle` Node |
| `editor-playground/src/extensions/custom-style.js` | Generic `CustomStyle` Node for arbitrary user styles |

All three are thin Node extensions that:
- Extend the `paragraph` schema with a `styleId` attribute
- Render with a `<p data-style-id="…">` element
- Provide a `setStyle(id)` command

The existing `heading` extension stays (with `levels: [1, 2, 3, 4, 5, 6]`) — `Heading 6` already works in v0.3.5.

#### New module: `editor-playground/src/style-manager.js`

Exports:
```js
export const StyleManager = {
  register(styleDef)         // Add a new style at runtime
  unregister(id)             // Remove a custom style
  get(id)                    // Get current definition of a style
  list()                     // [{ id, name, builtin, basedOn, ... }, ...]
  captureFromNode(node)      // Extract font/size/color/weight/etc from a ProseMirror node
  updateToMatch(id, node)    // Write the captured values into the style definition
  toDocx()                   // Serialize to `<w:style>` XML fragments
  fromDocx(fragments)        // Parse incoming DOCX `<w:style>` fragments
  apply(editor, id)          // Apply style to current selection
}
```

Internal state: an in-memory Map of style id → style def. Default styles seeded at startup from a built-in list. Custom styles persisted in `localStorage` (web) and via `EditorBridge` to `%APPDATA%/Vela Docs/custom-styles.json` (production).

#### main.js wiring

```js
import { StyleManager } from './style-manager.js';

// At startup
StyleManager.seedBuiltinStyles();

// Style dropdown becomes a custom <details>-based menu
mountStyleDropdown(document.getElementById('style-dropdown'), {
  onApply:    (id) => editor.chain().focus().setStyle(id).run(),
  onUpdate:   (id) => StyleManager.updateToMatch(id, /* current node */),
  onModify:   (id) => openStyleEditor(id),
  onCreate:   (def) => { StyleManager.register(def); refreshDropdown(); }
});
```

#### Playwright-style style editor

A small modal dialog (`editor-playground/src/modals.js` gets `openStyleEditor(existingDef?)`) — uses the same DOM as the inline editor above.

#### Qt side

`EditorBridge` gains:
- `Q_INVOKABLE QStringList getStyleIds()` — for ribbon's style dropdown sync
- `Q_INVOKABLE QString getStyleDefinition(const QString& id)` — JSON of the style def
- `Q_INVOKABLE void registerCustomStyle(const QString& id, const QString& jsonDef)` — persists to disk

#### Ribbon.cpp

The current `QComboBox` paragraph-style dropdown gets extended to show the same custom menu structure (with a small overlay menu, since QComboBox can't show arbitrary widgets).

`src/document/StyleSheet.{h,cpp}` already exists (per v0.3.3 MAJOR-04) — it's the C++ mirror of `style-manager.js`. We extend it with:
- `registerStyle(StyleDef)`
- `updateStyle(id, StyleDef)`
- `applyStyleToSelection(id)`
- DOCX serialization helpers

---

## F5 — Tab Stops (Google Docs-style Ruler)

### Behavior

A **horizontal ruler** above the page canvas, displaying margins and tab-stop markers.

```
    ◀──── 0.5" ──── 1.0" ──▼─ 1.5" ──── 2.0" ──── 2.5" ──── 3.0" ──▶
    [margin]│        [margin]│  ▌tab     [margin]│               [margin]
             ├──────────┤     │            ├─────────────────┤
                              ↑
                              tab stop at 1.5", type = Center, leader = dots
```

### Tab-stop types

| Type | DOCX | Visual |
|---|---|---|
| Left (default) | `w:val="left"` | `▌` |
| Center | `w:val="center"` | `⊤` |
| Right | `w:val="right"` | `▐` |
| Decimal | `w:val="decimal"` | `⊥` |
| Bar | `w:val="bar"` | `│` |

### Leaders

None (default), dots (`w:leader="dot"`), dashes (`w:leader="hyphen"`), underscores (`w:leader="underscore"`).

### Interactions

| Action | Result |
|---|---|
| Click ruler | Add tab stop at that x |
| Drag a tab stop | Move it (snaps to 0.05" grid) |
| Double-click a tab stop | Open "Tab Stop" dialog (position, type, leader) |
| Right-click a tab stop | Context menu (Left / Center / Right / Decimal / Bar / Clear this / Clear all) |
| Drag margin markers | Adjust left/right margins |

### Default behavior

Until user adds a custom stop, default stops every **0.5"** are applied (matches DOCX default).

### Per-paragraph

DOCX `<w:tabs>` lives inside `<w:pPr>` — each paragraph has its own tab stops. Clearing a paragraph's stops returns to the default 0.5" grid.

### Implementation

#### New module: `editor-playground/src/ruler.js`

Exports:
```js
export function mountRuler(container, { pageWidthInches, marginLeftInches, marginRightInches,
                                       onTabStopChange, onMarginChange })
```

Internal:
- SVG-based ruler with major/minor tick marks
- Margin triangles on the ends (drag to resize)
- Tab stop icons (drag to move, click to add, right-click to clear)
- Snaps to 0.05" grid
- Keyboard: `Delete` removes focused tab stop; `←/→` moves by 0.05"

#### New module: `editor-playground/src/tab-stop.js`

ProseMirror `Decoration` plugin:
- Renders visible tab-stop markers *inside* the document at the active paragraph's stops
- Adds tab characters as proper tab-stop-aligned whitespace
- Keeps the in-editor view in sync with the ruler

Backed by TipTap's paragraph attributes (`tabStops: [{ pos, type, leader }]`).

#### Dialog

`editor-playground/src/modals.js` gets:
```js
export async function openTabStopDialog(currentStops)
```

Returns `{ pos, type, leader } | null`.

#### DOCX persistence

`src/formats/DocxExporter.cpp` — when writing a paragraph, if it has `tabStops`, emit:
```xml
<w:pPr>
  <w:tabs>
    <w:tab w:val="center" w:pos="2160" w:leader="dot"/>
    <w:tab w:val="right" w:pos="4320"/>
  </w:tabs>
</w:pPr>
```

`src/formats/DocxImporter.cpp` — parse `<w:tabs>` and store on the paragraph node.

#### Index.html

```html
<div class="page" id="page">
  <div class="ruler" id="ruler"></div>      <!-- NEW -->
  <div class="editor" id="editor"></div>
</div>
```

---

## Cleanup — Status Bar & Top Bar

The user reported "the settings on the bottom of the document is very weird". Confirmed by reading `src/app/StatusBar.cpp`:

| What | v0.3.5 | v0.4.0 |
|---|---|---|
| "Phase 0" badge | Permanent placeholder text, gray, italic — looks broken | **Remove** |
| Zoom slider | `m_zoomSlider->setEnabled(false)` — disabled permanently | **Wire** to `m_webView->setZoomFactor()` |
| "English (US)" label | Static | **Wire** to `SettingsStore::spellLanguage()` (already a v0.3.5 service) |
| Page info "— / —" | Always dashes | **Wire** to actual page count when page-layout view is on (v0.3.2 MAJOR-02) |
| Ready message | Working | Keep |
| Word count | Working | Keep |

### Top bar

The **Preferences gear icon** in the top bar (`MainWindow.cpp:506-510`) moves into the Backstage > Options panel. The top bar's two right-side buttons become:

- **Theme toggle** (light/dark) — kept
- (Preferences moved)

### Implementation

`src/app/StatusBar.{h,cpp}`:
- Remove `m_phaseLabel` (Phase 0 badge)
- Enable `m_zoomSlider`; connect to `MainWindow::zoomSliderChanged(int)` signal
- Connect language label to `SettingsStore::spellLanguageChanged(QString)` signal

`src/app/MainWindow.cpp`:
- Remove the Preferences button from `buildTopBar()` (lines 506–510)
- Add signal/slot wiring for zoom slider + language label

---

## Implementation Order

| # | Item | Effort | Depends on |
|---|---|---|---|
| 1 | Cleanup (StatusBar + top bar) | 1 day | — |
| 2 | Material Symbols in playground | 1 day | Material Symbols font asset |
| 3 | Material Symbols in Qt Ribbon | 2 days | #2 + SVG icons in resources |
| 4 | Custom color picker (HTML only) | 4 days | Material Symbols (#2) |
| 5 | Files tab / Backstage (C++) | 1 week | — |
| 6 | Heading system — built-in styles | 1 week | Material Symbols (#2) |
| 7 | Heading system — custom levels + editor | 1 week | #6 |
| 8 | Heading system — "Update to match" | 3 days | #6 |
| 9 | Heading system — DOCX persistence | 1 week | #6–#8 |
| 10 | Tab stops — ruler | 1 week | Material Symbols (#2) |
| 11 | Tab stops — DOCX persistence | 3 days | #10 |
| 12 | Tests + Playwright + Catch2 | 1 week | #1–#11 |

**Total: ~6–8 weeks.**

Order chosen so each step is independently shippable as a daily/weekly build.

---

## File-Level Change Lists

### C++ (Qt side)

| File | Action | Feature |
|---|---|---|
| `CMakeLists.txt` | `project(VelaDocs VERSION 0.4.0 …)` | versioning |
| `src/app/MainWindow.{h,cpp}` | Add Files button, Backstage pointer, signals | F1, cleanup |
| `src/app/Ribbon.{h,cpp}` | Files button, SVG icons, custom style dropdown | F1, F2, F4 |
| `src/app/StatusBar.{h,cpp}` | Remove Phase 0, wire zoom & language | cleanup |
| `src/app/Backstage.{h,cpp}` | New — overlay widget | F1 |
| `src/app/BackstageRail.{h,cpp}` | New — left icon rail | F1 |
| `src/app/BackstagePanel.h` | New — base class | F1 |
| `src/app/BackstageSavePanel.{h,cpp}` | New | F1 |
| `src/app/BackstageSaveAsPanel.{h,cpp}` | New | F1 |
| `src/app/BackstageOpenPanel.{h,cpp}` | New | F1 |
| `src/app/BackstagePrintPanel.{h,cpp}` | New | F1 |
| `src/app/BackstagePrintPreviewPanel.{h,cpp}` | New | F1 |
| `src/app/BackstageOptionsPanel.{h,cpp}` | New — hosts PreferencesDialog | F1 |
| `src/app/BackstageHelpPanel.{h,cpp}` | New | F1 |
| `src/app/BackstageRecentPanel.{h,cpp}` | New | F1 |
| `src/app/EditorBridge.{h,cpp}` | New Q_INVOKABLE methods | F1, F4, F5 |
| `src/document/StyleSheet.{h,cpp}` | Extended | F4 |
| `src/document/TabStop.{h,cpp}` | New | F5 |
| `src/formats/DocxExporter.{h,cpp}` | Extended | F4, F5 |
| `src/formats/DocxImporter.{h,cpp}` | Extended | F4, F5 |
| `src/services/PrintService.{h,cpp}` | Extended to use QPrintPreviewDialog | F1 |
| `src/services/SettingsStore.{h,cpp}` | New: recentColors(), customStyles() | F3, F4 |
| `installer/vela-docs.nsi` | Bump version | versioning |
| `resources/resources.qrc` | New SVG icons, material symbols woff2 | F2 |

### Web (Playground + Production editor)

| File | Action | Feature |
|---|---|---|
| `editor-playground/package.json` | `version: 0.4.0` | versioning |
| `editor-playground/index.html` | Material Symbols, new dropdown markup | F2, F4, F5 |
| `editor-playground/src/styles.css` | Material Symbols import, color-picker styles, ruler styles | F2, F3, F5 |
| `editor-playground/src/main.js` | Wire up new modules | F2, F3, F4, F5 |
| `editor-playground/src/color-picker.js` | New | F3 |
| `editor-playground/src/style-manager.js` | New | F4 |
| `editor-playground/src/ruler.js` | New | F5 |
| `editor-playground/src/tab-stop.js` | New | F5 |
| `editor-playground/src/modals.js` | Extended (style editor + tab-stop dialog) | F4, F5 |
| `editor-playground/src/extensions/title.js` | New | F4 |
| `editor-playground/src/extensions/subtitle.js` | New | F4 |
| `editor-playground/src/extensions/custom-style.js` | New | F4 |
| `src/editor/index.html` | Mirror playground changes | F2, F3, F4, F5 |
| `src/editor/command-dispatcher.js` | New commands | F3, F4, F5 |

### Documentation

| File | Action |
|---|---|
| `CHANGELOG.md` | New v0.4.0 section |
| `README.md` | Add v0.4.0 row |
| `docs/ARCHITECTURE.md` | New section for Files tab + new modules |
| `improvement/vela-docs-v0.4.0-improvement-plan.md` | This file |
| `stitch_vela_docs_ui_system/files_backstage/` | New Stitch design |

### Tests

| File | Action |
|---|---|
| `tests/unit/test_backstage.cpp` | New |
| `tests/unit/test_color_picker.cpp` | New (C++ bridge for recent colors) |
| `tests/unit/test_style_sheet.cpp` | New |
| `tests/unit/test_tab_stop.cpp` | New |
| `tests/unit/test_docx.cpp` | Extend: heading styles, custom styles, tab stops |
| `tests/integration/test_round_trip.cpp` | Extend: Title/Subtitle/Custom/Heading 6+ round-trip |
| `tests/ui/test_files_tab.spec.ts` | New (Playwright) |
| `tests/ui/test_style_dropdown.spec.ts` | New (Playwright) |
| `tests/ui/test_tab_stops.spec.ts` | New (Playwright) |

---

## Phase Checklists

### Phase A: Quick wins (Week 1)

- [ ] Bump version to 0.4.0 in CMakeLists.txt, package.json, index.html, top-bar label
- [ ] StatusBar: remove "Phase 0" badge, wire zoom slider, wire language label
- [ ] Material Symbols font + 16 SVG icons in `resources/`
- [ ] Playground `index.html` + `styles.css` swap to Material Symbols
- [ ] `Ribbon.cpp` uses SVG icons

### Phase B: Color picker (Week 2)

- [ ] `color-picker.js` module — built-in swatches + tints + custom RGB/HEX + recent
- [ ] `editor-playground/index.html` updated to mount the picker for text + highlight
- [ ] Persistence via `localStorage` (web) and `EditorBridge` (production)
- [ ] Tests: HEX↔RGB sync, persistence, apply commands

### Phase C: Files tab / Backstage (Weeks 3–4)

- [ ] All `Backstage*` C++ classes
- [ ] Files button at far-left of ribbon, accent color when checked
- [ ] `Alt+F` toggle, `Esc` close
- [ ] Each panel wired to existing services (PrintService, RecentFiles, PreferencesDialog)
- [ ] Stitch design `stitch_vela_docs_ui_system/files_backstage/`
- [ ] Tests: open/close, panel switching, keyboard

### Phase D: Heading system (Weeks 4–7)

- [ ] Title, Subtitle, Custom Style TipTap extensions
- [ ] `style-manager.js` with `register / updateToMatch / captureFromNode`
- [ ] Style dropdown UI — built-in / custom / update-to-match / modify
- [ ] `StyleSheet.{h,cpp}` C++ mirror
- [ ] `DocxExporter` writes full `<w:styles>` including custom styles
- [ ] `DocxImporter` reads custom styles
- [ ] `EditorBridge::registerCustomStyle` + persistence in `%APPDATA%`
- [ ] Tests: built-in styles, custom creation, update-to-match, DOCX round-trip

### Phase E: Tab stops (Weeks 7–8)

- [ ] `ruler.js` — draggable tab stops, margin markers
- [ ] `tab-stop.js` ProseMirror Decoration plugin
- [ ] `openTabStopDialog` in `modals.js`
- [ ] `TabStop.{h,cpp}` C++ mirror
- [ ] DOCX `<w:tabs>` export + import
- [ ] Tests: ruler interactions, DOCX round-trip

### Phase F: Polish & ship (Week 8)

- [ ] Manual QA pass — every feature in the README
- [ ] Playwright UI tests for Files tab, color picker, style dropdown, tab stops
- [ ] Updated `CHANGELOG.md`, `README.md`, `ARCHITECTURE.md`
- [ ] Installer version bump
- [ ] Build, sign, package the `.exe`

---

## Risks & Open Questions

| Risk | Mitigation |
|---|---|
| Material Symbols font file size (~150 KB woff2) | Bundled once; sub-set to used glyphs only via `unicode-range` |
| Backstage animation frame rate on low-end hardware | Make animation optional; default 180 ms, 0 ms if `prefers-reduced-motion` |
| "Update to match" loses track of original style if user clicks twice | Confirmation prompt before re-update; undo support |
| Custom heading styles polluting DOCX `<w:styles>` | Namespaced with `customStyle="1"` attribute; Word ignores unknown styles gracefully |
| Tab stop ruler accuracy at non-100% zoom | Pin ruler to the same scale as the page canvas; re-render on zoom |
| Backstage Print Preview not honoring page-layout changes | Render via the v0.3.2 print pipeline (already handles margins/headers/footers) |

### Open questions

1. Should the Backstage overlay dim/blur the editor underneath, or fully cover it (like Word)? *(recommendation: dim + 5 % scale-down, like Word)*
2. Should custom heading styles be **document-local** (stored in DOCX styles part only) or **user-global** (also persisted across documents in `%APPDATA%`)? *(recommendation: document-local for styles the user creates *for that document*, with an "Export to global" option)*
3. Should the Tab Stops ruler be visible always, or only when "Show ruler" is enabled (v0.3.2)? *(recommendation: visible always when in page-layout mode, hidden in focus mode)*

---

*This plan is a living document. Update as items are completed and new requirements are discovered.*