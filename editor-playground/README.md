# Vela Docs — Editor Playground v0.4.1

A Vite + TipTap dev server for iterating on the word-processor **editor surface**
with hot reload, **before** baking it into the Qt/C++ desktop app.

This lets you see your changes live (no C++ compile, no MSVC, no Qt build) in a
regular browser — just `npm install && npm run dev`.

## 🚀 Quick start

```powershell
cd "D:\ai-and-code\Vela-Docs\editor-playground"
npm install
npm run dev
```

Opens `http://localhost:5173/` automatically. Edits to any file → hot reload.

> **First run only:** `npm run dev` triggers the `predev` hook, which calls
> `python ../tools/fetch-material-symbols.py`. The script downloads the
> Material Symbols Outlined font (≈ 312 KB) to
> `editor-playground/src/assets/material-symbols-outlined.woff2` (and a
> matching copy in `../resources/fonts/`). It's fully offline afterwards —
> no network requests at runtime.

## ✨ What's in v0.4.1

- **Icons are finally here** — Material Symbols Outlined font is bundled locally at
  `editor-playground/src/assets/material-symbols-outlined.woff2`. Toolbar buttons
  now show `format_align_left`, `format_bold`, `format_list_bulleted` glyphs
  instead of `L`, `B`, `UL` letters.
- **Files tab redesigned** — anchored to the topbar + ribbon like Word 2010+;
  drops a soft shadow when the Backstage overlay is open.
- **Ribbon groups** — the Home panel is split into proper groups (Font / Paragraph /
  Styles / Editing) with subtle dividers and 11 px uppercase labels.
- **Welcome / empty-state overlay** matches `welcome_to_vela_docs`. First launch
  shows Blank / Resume / Letter / Report / Invoice tiles.
- Brand says **Vela Docs 0.4.1** in the topbar and a `v0.4.1` chip in the status bar.

## ✨ What's in v0.4.0 (carried forward)

### 3-tab ribbon (Google Docs / MS Word style)
- **Home tab** — fonts, sizes, **B/I/U/S**, text + highlight colors (8 each),
  paragraph styles (¶/H1–H4), alignment (L/C/R/J), lists (bullet/numbered/task),
  indent, **clear formatting**
- **Insert tab** — **image** (URL), **table** (rows × cols picker),
  **link**, **page break** (horizontal rule), **horizontal rule**, **footnote**,
  special-character palette
- **Table tab** — appears enabled only when cursor is inside a table:
  insert/delete row above/below, insert/delete column left/right,
  merge/split cells, toggle header row, delete table

### Keyboard shortcuts
| Shortcut          | Action                    |
|-------------------|---------------------------|
| `Ctrl+B / I / U`  | Bold / Italic / Underline |
| `Ctrl+Shift+S`    | Strikethrough             |
| `Ctrl+1..4`       | Heading 1..4               |
| `Ctrl+0`          | Normal text                |
| `Ctrl+Shift+L / O / 7` | Bullet / Numbered / Task list |
| `Ctrl+] / [`      | Indent / Outdent           |
| `Ctrl+K`          | Insert link                |
| `Ctrl+Shift+K`    | Insert table               |
| `Ctrl+Alt+I`      | Insert image               |
| `Ctrl+S`          | Save (round-trip to .docx) |
| `Ctrl+O`          | Open .docx                 |
| `Alt+F`           | Toggle the Files (Backstage) tab |
| `Esc`             | Close Backstage / Welcome / dropdowns |

### Live features
- ✅ Word count, character count, reading time, page estimate
- ✅ Active-state highlighting on toolbar buttons (B is lit when cursor is on bold)
- ✅ Table cell coordinates (e.g. `R3 C2`) shown in status bar when in a table
- ✅ Light ↔ Dark mode toggle (top-right)
- ✅ Save / Export-as-HTML / Clear document actions
- ✅ Pre-loaded sample document showcasing all formatting features
- ✅ Welcome / empty-state overlay (closable, remembered for 7 days)

## 🎯 Why this exists

The real app uses C++ + Qt + QtWebEngine to host this same editor inside a
native window. This playground is a **development accelerator**: every visual
change you make here gets reviewed in seconds, then the file gets vendored
into `src/editor/index.html` for the Qt app to load via QWebChannel.

## 🔄 Workflow

1. **Tweak** `src/styles.css` or `src/main.js` → browser refreshes in <100ms
2. **Iterate** until it looks right
3. **Copy** the working code into `../src/editor/` (the production target)
4. **Build** the Qt app with `build.bat run` to embed it natively

## 🗂️ File layout

```
editor-playground/
├── package.json              # TipTap 2.4 + 9 extensions + Vite 5
├── vite.config.js            # Dev server config
├── vite.production.config.js # Production bundle config
├── index.html                # 3-tab ribbon layout (top bar says "Vela Docs 0.4.1")
├── src/
│   ├── main.js               # TipTap setup, toolbar wiring, keyboard shortcuts
│   ├── modals.js             # Insert Table / Insert Image / Style editor / Tab stop dialogs
│   ├── welcome.js            # ← NEW v0.4.1 — first-run welcome / empty-state overlay
│   ├── styles.css            # Vela Precision design tokens + Material Symbols
│   ├── backstage.js          # Microsoft Word-style Files (Backstage) overlay
│   ├── color-picker.js       # ← v0.4.0 — custom HEX/RGB picker
│   ├── style-manager.js      # ← v0.4.0 — Title/Subtitle/Headings + "Update to match"
│   ├── ruler.js              # ← v0.4.0 — tab-stop ruler
│   ├── tab-stop.js           # ← v0.4.0 — ProseMirror Decoration plugin
│   └── extensions/
│       ├── title.js          # ← v0.4.0
│       ├── subtitle.js       # ← v0.4.0
│       └── custom-style.js   # ← v0.4.0
└── README.md
```

## 🔧 Behind the scenes

| Layer              | Technology                                    |
|--------------------|-----------------------------------------------|
| Dev server         | Vite 5                                         |
| Editor framework   | TipTap 2 (ProseMirror)                          |
| Document schema    | ProseMirror JSON (standard)                    |
| Typography        | Inter / Source Serif 4 / JetBrains Mono        |
| Icon font          | Material Symbols Outlined (locally bundled)    |
| Design system      | "Vela Precision" — indigo accent, 4px grid    |

See `../src/editor/index.html` for the version that gets embedded in Qt.
See `../stitch_vela_docs_ui_system/` for the original Stitch export that
inspired this design.
