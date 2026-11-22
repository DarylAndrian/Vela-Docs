# Files / Backstage — Design System (v0.4.0)

> Source-of-truth design tokens for the Microsoft Word-style Files tab in
> Vela Docs v0.4.0. This document is the canonical reference; the
> implementation in `src/app/Backstage.{h,cpp}` and the playground
> counterpart in `editor-playground/index.html` both consume the values
> defined here.

---

## At a glance

The Files tab is a full-width overlay that opens on top of the editor. It
contains a left rail of action items and a right content area that swaps
panels based on the selection. The default first panel is **Save**.

```
┌────────────────────────────────────────────────────────────────────┐
│  📄 Files │  Home │  Insert │  Table │  ────  [↶] [↷]   [◐]      │  ← ribbon (Files tab in accent color)
├────────────────────────────────────────────────────────────────────┤
│ ┌─────────────┬──────────────────────────────────────────────────┐ │
│ │  Save       │                                                  │ │
│ │  Save As    │                                                  │ │
│ │  Open       │   Save                                          │ │
│ │  ─────      │   Your document is up to date.                  │ │
│ │  Print      │   [ ✓ Save ]                                    │ │
│ │  Print      │                                                  │ │
│ │  Preview    │   Save as another format                        │ │
│ │  ─────      │   [Word (.docx)] [PDF] [MD] [HTML] [ODT] [RTF]  │ │
│ │  Options    │                                                  │ │
│ │  Help       │                                                  │ │
│ │  ─────      │                                                  │ │
│ │  Recent     │                                                  │ │
│ └─────────────┴──────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

---

## Tokens

All values are copied from `vela_precision/DESIGN.md` so the Backstage
visually matches the rest of the app.

| Token             | Value       | Usage                            |
|-------------------|-------------|----------------------------------|
| `--accent`        | `#3525CD`   | Active rail item, primary button |
| `--accent-hover`  | `#2A1FB0`   | Hover state of primary button    |
| `--accent-soft`   | `#E1E8FD`   | Hover state of secondary button  |
| `--bg-rail`       | `#2A2A3A`   | Left rail background              |
| `--bg-soft`       | `#F7F7FA`   | Backstage background             |
| `--text`          | `#222222`   | Primary text                      |
| `--text-soft`     | `#464555`   | Secondary text                    |
| `--text-muted`    | `#777587`   | Tertiary text                     |
| `--border`        | `#D8D8E0`   | Button / card border              |
| `--radius`        | `4px`       | Border radius                     |

---

## Components

### Rail item (left side)

```
.vBackstageItem {
    display: flex;
    align-items: center;
    gap: 10px;
    width: 100%;
    padding: 8px 12px;
    background: transparent;
    border: none;
    border-radius: 4px;
    color: #C8C8D8;            /* default text */
    font-size: 13px;
    cursor: pointer;
}
.vBackstageItem:hover {
    background: #3A3A4D;
    color: #FFFFFF;
}
.vBackstageItem.isActive {
    background: #3525CD;        /* accent */
    color: #FFFFFF;
}
```

Visual size: 36 px tall × 100% wide. Icon size: 16 px.

### Primary button

```
.vBtnPrimary {
    background: #3525CD;
    color: white;
    border: 1px solid #3525CD;
    border-radius: 4px;
    padding: 6px 14px;
    font-weight: 600;
    text-align: left;
}
.vBtnPrimary:hover { background: #2A1FB0; }
```

### Secondary button

```
.vBtnSecondary {
    background: #FFFFFF;
    color: #222222;
    border: 1px solid #D8D8E0;
    border-radius: 4px;
    padding: 6px 14px;
    text-align: left;
}
.vBtnSecondary:hover {
    background: #E1E8FD;
    color: #3525CD;
    border-color: #3525CD;
}
```

### Panel header (h2 + summary)

```
.vPanelHeader {
    font-size: 26px;
    font-weight: 300;
    color: #222222;
    margin: 0 0 8px;
}
.vPanelSummary {
    color: #777587;
    margin: 0 0 16px;
}
```

### Recent card (Recent panel + Open panel)

```
.vRecentCard {
    background: #FFFFFF;
    border: 1px solid #D8D8E0;
    border-radius: 4px;
    padding: 8px;
    cursor: pointer;
    width: 140px;
    height: 120px;
    text-align: left;
}
.vRecentCard:hover {
    border-color: #3525CD;
    background: #F5F5FA;
}
```

---

## Panels

| ID              | Title          | Content                                       |
|-----------------|----------------|-----------------------------------------------|
| `save`          | Save           | Status text + Save button + format grid       |
| `saveAs`        | Save As        | Format grid (7 options)                       |
| `open`          | Open           | Browse button + Recent grid                   |
| `print`         | Print          | Printer combo + copies + page range + Print   |
| `printPreview`  | Print Preview  | Zoom controls + QPrintPreviewWidget           |
| `options`       | Options        | "Open Preferences…" button (links to dialog)  |
| `help`          | Help           | Help links + keyboard cheat-sheet table       |
| `recent`        | Recent         | Grid of recent files (lazy-populated)         |

---

## Keyboard

| Key      | Action                    |
|----------|---------------------------|
| `Alt+F`  | Toggle the Files tab     |
| `Esc`    | Close the Files tab      |
| `Tab`    | Cycle through rail items  |
| `Enter`  | Activate focused item    |

---

## Implementation mapping

| Design token / class   | C++ implementation                                |
|------------------------|---------------------------------------------------|
| `--accent`             | `Backstage::BackstageRail` QSS                    |
| `vBackstageItem`       | `BackstageRail::addItem`                          |
| `vBtnPrimary`          | `BackstagePanels.cpp::makePrimaryButton`          |
| `vBtnSecondary`        | `BackstagePanels.cpp::makeButton`                |
| `vPanelHeader`         | `BackstagePanels.cpp::panelHeader`                |
| `vRecentCard`          | `OpenPanel` and `RecentPanel` grid builders       |
| `Alt+F`                | `m_actFiles` shortcut in `MainWindow::buildActions` |
| `Esc`                  | `QShortcut(Qt::Key_Escape)` in `wireSignals`     |