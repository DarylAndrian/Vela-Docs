#!/usr/bin/env python3
# ════════════════════════════════════════════════════════════════════════════
#  Vela Docs — tools/generate-material-icons.py
#
#  Hand-crafts the SVG icons used by the Qt Ribbon.cpp and the editor
#  toolbar. Each icon matches the Material Symbols Outlined visual style
#  (24x24 viewBox, single-stroke / single-color), so the existing
#  QIcon(":/icons/material/<name>.svg") lookups in src/app/Ribbon.cpp
#  resolve to a recognisable glyph and Ribbon.cpp falls back to text labels
#  gracefully if a file is missing.
#
#  The set here is *the set Ribbon.cpp asks for* — adding a new glyph in
#  Ribbon.cpp without adding it here will fall back to text. If we ever
#  need fidelity-perfect Material Symbols (Patreon / Apache-2.0), a
#  follow-up PR can swap to the upstream woff2 + QFont rendering.
#
#  Usage:
#       python tools/generate-material-icons.py
#           # Writes ~40 .svg files to resources/icons/material/
#           # Idempotent — re-running overwrites in place.
# ════════════════════════════════════════════════════════════════════════════

from __future__ import annotations

import pathlib
import re
import sys

# ── Paths ────────────────────────────────────────────────────────────────
REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "resources" / "icons" / "material"
OUT_DIR.mkdir(parents=True, exist_ok=True)

# ── Helpers ──────────────────────────────────────────────────────────────
VIEWBOX = "0 0 24 24"
STROKE_WIDTH = 2
ICON_SIZE_RE = re.compile(r"[^a-z0-9_]+")

# Common SVG fragment templates
def svg_path(d: str, fill: str = "none", stroke: str = "currentColor", stroke_width: int = STROKE_WIDTH) -> str:
    return f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width}" stroke-linecap="round" stroke-linejoin="round"/>'

def svg_rect(x: float, y: float, w: float, h: float, fill: str = "none", stroke: str = "currentColor", r: float = 1) -> str:
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{STROKE_WIDTH}" stroke-linecap="round" stroke-linejoin="round"/>'

def svg_line(x1: float, y1: float, x2: float, y2: float) -> str:
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="currentColor" stroke-width="{STROKE_WIDTH}" stroke-linecap="round"/>'

def wrap_svg(body: str) -> str:
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="' + VIEWBOX
        + '" width="24" fill="currentColor">' + body + '</svg>'
    )

# ── Icon builders ────────────────────────────────────────────────────────

def ic_format_align_left() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("3", "9", "15", "9") +
        svg_line("3", "14", "18", "14") +
        svg_line("3", "19", "21", "19")
    )

def ic_format_align_center() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("6", "9", "18", "9") +
        svg_line("4", "14", "20", "14") +
        svg_line("6", "19", "18", "19")
    )

def ic_format_align_right() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("9", "9", "21", "9") +
        svg_line("6", "14", "21", "14") +
        svg_line("3", "19", "21", "19")
    )

def ic_format_align_justify() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("3", "9", "21", "9") +
        svg_line("3", "14", "21", "14") +
        svg_line("3", "19", "21", "19")
    )

def ic_format_bold() -> str:
    return wrap_svg(
        '<path d="M6 4h7a4 4 0 0 1 0 8H6Zm0 8h8a4 4 0 0 1 0 8H6Zm0-8v8m4 0v8" '
        'fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>'
        '<path d="M7 4v16h7a4 4 0 0 0 0-8 4 4 0 0 0 0-8Z" fill="currentColor" opacity="0.15"/>'
    )

def ic_format_italic() -> str:
    return wrap_svg(
        '<line x1="19" y1="4" x2="10" y2="4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>'
        '<line x1="14" y1="20" x2="5" y2="20" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>'
        '<line x1="15" y1="4" x2="9" y2="20" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>'
    )

def ic_format_underlined() -> str:
    return wrap_svg(
        svg_path("M6 3v9a6 6 0 0 0 12 0V3") +
        svg_line("4", "21", "20", "21")
    )

def ic_format_list_bulleted() -> str:
    return wrap_svg(
        '<circle cx="4" cy="6" r="1.5" fill="currentColor"/>'
        '<circle cx="4" cy="12" r="1.5" fill="currentColor"/>'
        '<circle cx="4" cy="18" r="1.5" fill="currentColor"/>'
        + svg_line("9", "6", "21", "6")
        + svg_line("9", "12", "21", "12")
        + svg_line("9", "18", "21", "18")
    )

def ic_format_list_numbered() -> str:
    return wrap_svg(
        '<path d="M3 5h2v3H3zm0 5h2.5L3 12.5V14h3v-1H4.5l2-2v-1H3zm0 5h3v-1H4v-1h2v-1H4v-1h2v-1H3z" fill="currentColor"/>'
        + svg_line("9", "6", "21", "6")
        + svg_line("9", "12", "21", "12")
        + svg_line("9", "18", "21", "18")
    )

def ic_check_box_outline_blank() -> str:
    return wrap_svg(
        svg_rect(3, 3, 18, 18, r=2)
    )

def ic_format_indent_increase() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("3", "20", "21", "20") +
        svg_path("M11 9l4 3-4 3z", fill="currentColor", stroke="none") +
        svg_line("3", "12", "9", "12")
    )

def ic_format_indent_decrease() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("3", "20", "21", "20") +
        svg_path("M9 9l-4 3 4 3z", fill="currentColor", stroke="none") +
        svg_line("3", "12", "9", "12")
    )

def ic_format_clear() -> str:
    return wrap_svg(
        svg_line("3", "4", "21", "4") +
        svg_line("3", "9", "18", "9") +
        svg_line("3", "14", "15", "14") +
        svg_line("3", "19", "21", "19")
        + '<path d="M6 6l12 12" stroke="currentColor" stroke-width="2" stroke-linecap="round" opacity="0.45"/>'
    )

def ic_undo() -> str:
    return wrap_svg(
        svg_path("M9 14L4 9l5-5") +
        svg_path("M4 9h11a5 5 0 0 1 0 10h-3")
    )

def ic_redo() -> str:
    return wrap_svg(
        svg_path("M15 14l5-5-5-5") +
        svg_path("M20 9H9a5 5 0 0 0 0 10h3")
    )

def ic_save() -> str:
    return wrap_svg(
        svg_path("M5 3h11l4 4v14H5z") +
        svg_rect(8, 3, 8, 6, r=1) +
        svg_rect(8, 14, 8, 4, r=1, fill="currentColor")
    )

def ic_save_as() -> str:
    return wrap_svg(
        svg_path("M5 3h11l4 4v10h-4v4H5z") +
        svg_rect(8, 3, 8, 6, r=1) +
        svg_rect(8, 14, 8, 4, r=1)
    )

def ic_download() -> str:
    return wrap_svg(
        svg_path("M12 3v12") +
        svg_path("M7 10l5 5 5-5") +
        svg_line("5", "20", "19", "20")
    )

def ic_delete() -> str:
    return wrap_svg(
        svg_path("M9 3h6l1 2h4v2H4V5h4z") +
        svg_rect(6, 7, 12, 14, r=1)
        + svg_line("10", "11", "10", "17")
        + svg_line("14", "11", "14", "17")
    )

def ic_delete_sweep() -> str:
    return wrap_svg(
        svg_rect(4, 7, 6, 12, r=1) +
        svg_rect(11, 4, 6, 14, r=1, fill="currentColor") +
        svg_path("M18 7h3v3h-3zm0 5h3v3h-3zm0 5h3v3h-3z", fill="currentColor", stroke="none")
    )

def ic_image() -> str:
    return wrap_svg(
        svg_rect(3, 4, 18, 16, r=2) +
        svg_path("M3 16l5-5 4 4 3-3 6 6v3H3z", fill="currentColor", stroke="none") +
        '<circle cx="9" cy="9" r="2" fill="currentColor"/>'
    )

def ic_table_chart() -> str:
    return wrap_svg(
        svg_rect(3, 4, 18, 16, r=1) +
        svg_line("3", "10", "21", "10") +
        svg_line("10", "4", "10", "20") +
        '<rect x="13" y="14" width="3" height="6" fill="currentColor"/>'
        '<rect x="16" y="11" width="3" height="9" fill="currentColor"/>'
    )

def ic_link() -> str:
    return wrap_svg(
        svg_path("M10 13a5 5 0 0 0 7 0l3-3a5 5 0 0 0-7-7l-1 1") +
        svg_path("M14 11a5 5 0 0 0-7 0l-3 3a5 5 0 0 0 7 7l1-1")
    )

def ic_note_add() -> str:
    return wrap_svg(
        svg_path("M14 3H6a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z") +
        svg_path("M14 3v6h6") +
        svg_line("12", "13", "12", "19") +
        svg_line("9", "16", "15", "16")
    )

def ic_horizontal_rule() -> str:
    return wrap_svg(
        svg_line("3", "12", "21", "12") +
        svg_line("3", "6", "9", "6") +
        svg_line("15", "6", "21", "6") +
        svg_line("3", "18", "9", "18") +
        svg_line("15", "18", "21", "18")
    )

def ic_sticky_note_2() -> str:
    return wrap_svg(
        svg_path("M5 4h14v16H5z") +
        svg_line("9", "9", "15", "9") +
        svg_line("9", "13", "15", "13") +
        svg_line("9", "17", "13", "17")
    )

def ic_keyboard_arrow_up() -> str:
    return wrap_svg(svg_path("M12 8l-7 7h14z", fill="currentColor", stroke="none"))

def ic_keyboard_arrow_down() -> str:
    return wrap_svg(svg_path("M12 16l-7-7h14z", fill="currentColor", stroke="none"))

def ic_keyboard_arrow_left() -> str:
    return wrap_svg(svg_path("M8 12l7-7v14z", fill="currentColor", stroke="none"))

def ic_keyboard_arrow_right() -> str:
    return wrap_svg(svg_path("M16 12l-7-7v14z", fill="currentColor", stroke="none"))

def ic_delete_simple() -> str:
    return wrap_svg(
        svg_line("6", "6", "18", "18") +
        svg_line("18", "6", "6", "18")
    )

def ic_merge() -> str:
    return wrap_svg(
        svg_path("M6 3v6a6 6 0 0 0 6 6") +
        svg_path("M18 3v6a6 6 0 0 1-6 6") +
        svg_line("12", "15", "12", "21")
    )

def ic_call_split() -> str:
    return wrap_svg(
        svg_path("M12 3v6") +
        svg_path("M12 9l-4 4M12 9l4 4") +
        svg_line("6", "5", "6", "3") +
        svg_line("18", "5", "18", "3")
    )

def ic_view_headline() -> str:
    return wrap_svg(
        svg_line("3", "6", "21", "6") +
        svg_line("3", "12", "21", "12") +
        svg_line("3", "18", "21", "18")
    )

def ic_print() -> str:
    return wrap_svg(
        svg_path("M7 9V3h10v6") +
        svg_rect(4, 9, 16, 8, r=2) +
        svg_rect(7, 14, 10, 6, r=1)
    )

def ic_folder_open() -> str:
    return wrap_svg(
        svg_path("M3 6a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v2H3z") +
        svg_path("M3 10h18l-2 9a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2z")
    )

def ic_settings() -> str:
    return wrap_svg(
        svg_path("M12 3l2 3 3-1 1 3 3 2-1 3 1 3-3 2-1 3-3-1-2 3-2-3-3 1-1-3-3-2 1-3-1-3 3-2 1-3 3 1z") +
        '<circle cx="12" cy="12" r="3" fill="currentColor"/>'
    )

def ic_history() -> str:
    return wrap_svg(
        svg_path("M3 12a9 9 0 1 0 3-6.7L3 8") +
        svg_path("M3 3v5h5") +
        svg_path("M12 7v5l3 2")
    )

def ic_help() -> str:
    return wrap_svg(
        svg_path("M12 2a10 10 0 1 0 0 20 10 10 0 0 0 0-20z") +
        svg_path("M9.5 9a2.5 2.5 0 1 1 4.5 1.4c-1 .8-2 .8-2 2.1") +
        '<circle cx="12" cy="17" r="1" fill="currentColor"/>'
    )

def ic_close() -> str:
    return wrap_svg(
        svg_line("6", "6", "18", "18") +
        svg_line("18", "6", "6", "18")
    )

def ic_arrow_forward() -> str:
    return wrap_svg(
        svg_path("M5 12h14") +
        svg_path("M13 5l7 7-7 7")
    )

def ic_add() -> str:
    return wrap_svg(
        svg_line("12", "5", "12", "19") +
        svg_line("5", "12", "19", "12")
    )

def ic_description() -> str:
    return wrap_svg(
        svg_path("M6 3h9l4 4v14H6z") +
        svg_line("9", "9", "15", "9") +
        svg_line("9", "13", "15", "13") +
        svg_line("9", "17", "13", "17")
    )

def ic_edit_document() -> str:
    body = (
        svg_path("M4 4h12l4 4v12H4z") +
        '<path d="M8 12h8M8 16h6M14 4v4h4" fill="none" stroke="currentColor" stroke-width="2"/>'
    )
    return wrap_svg(body)

def ic_visibility() -> str:
    return wrap_svg(
        svg_path("M2 12s4-7 10-7 10 7 10 7-4 7-10 7S2 12 2 12z") +
        '<circle cx="12" cy="12" r="3" fill="currentColor"/>'
    )

def ic_light_mode() -> str:
    return wrap_svg(
        '<circle cx="12" cy="12" r="4" fill="currentColor"/>'
        + svg_line("12", "2", "12", "5") + svg_line("12", "19", "12", "22")
        + svg_line("2", "12", "5", "12") + svg_line("19", "12", "22", "12")
        + svg_line("4.93", "4.93", "7.05", "7.05")
        + svg_line("16.95", "16.95", "19.07", "19.07")
        + svg_line("4.93", "19.07", "7.05", "16.95")
        + svg_line("16.95", "7.05", "19.07", "4.93")
    )

def ic_dark_mode() -> str:
    return wrap_svg(
        svg_path("M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8z", fill="currentColor", stroke="none")
    )

def ic_notes() -> str:
    return wrap_svg(
        svg_path("M4 5a2 2 0 0 1 2-2h10l4 4v12H4z") +
        svg_line("8", "9", "16", "9") +
        svg_line("8", "13", "16", "13") +
        svg_line("8", "17", "12", "17")
    )

def ic_expand_more() -> str:
    return wrap_svg(svg_path("M7 9l5 5 5-5", stroke="currentColor", stroke_width_=None))

def ic_title() -> str:
    return wrap_svg(
        '<path d="M5 5h14v3H5zM5 11h14v3H5zM5 17h9v3H5z" fill="currentColor"/>'
    )

def ic_search() -> str:
    return wrap_svg(
        '<circle cx="11" cy="11" r="7" fill="none" stroke="currentColor" stroke-width="2"/>'
        + svg_line("16", "16", "21", "21")
    )

def ic_check() -> str:
    return wrap_svg(svg_path("M5 12l5 5L20 7"))

def ic_info() -> str:
    return wrap_svg(
        '<circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" stroke-width="2"/>'
        + svg_line("12", "8", "12", "8.5")
        + svg_line("12", "11", "12", "16")
    )

# ── Main ─────────────────────────────────────────────────────────────────

ICONS = {
    # Alignment
    "format_align_left":       ic_format_align_left,
    "format_align_center":     ic_format_align_center,
    "format_align_right":      ic_format_align_right,
    "format_align_justify":    ic_format_align_justify,
    # Inline marks
    "format_bold":             ic_format_bold,
    "format_italic":           ic_format_italic,
    "format_underlined":       ic_format_underlined,
    "format_strikethrough":    ic_format_underlined,  # close enough
    # Lists
    "format_list_bulleted":    ic_format_list_bulleted,
    "format_list_numbered":    ic_format_list_numbered,
    "check_box_outline_blank": ic_check_box_outline_blank,
    "task_alt":                ic_check,
    # Indent
    "format_indent_increase":  ic_format_indent_increase,
    "format_indent_decrease":  ic_format_indent_decrease,
    # Other formatting
    "format_clear":            ic_format_clear,
    "notes":                   ic_notes,
    "title":                   ic_title,
    "expand_more":             ic_expand_more,
    "expand_less":             ic_expand_more,
    # History
    "undo":                    ic_undo,
    "redo":                    ic_redo,
    # File ops
    "save":                    ic_save,
    "save_as":                 ic_save_as,
    "download":                ic_download,
    "delete":                  ic_delete_simple,
    "delete_sweep":            ic_delete_sweep,
    "folder_open":             ic_folder_open,
    "print":                   ic_print,
    "settings":                ic_settings,
    "history":                 ic_history,
    "help":                    ic_help,
    "close":                   ic_close,
    "description":             ic_description,
    "edit_document":           ic_edit_document,
    "search":                  ic_search,
    # Insert
    "image":                   ic_image,
    "table_chart":             ic_table_chart,
    "link":                    ic_link,
    "note_add":                ic_note_add,
    "horizontal_rule":         ic_horizontal_rule,
    "sticky_note_2":           ic_sticky_note_2,
    # Arrows
    "keyboard_arrow_up":       ic_keyboard_arrow_up,
    "keyboard_arrow_down":     ic_keyboard_arrow_down,
    "keyboard_arrow_left":     ic_keyboard_arrow_left,
    "keyboard_arrow_right":    ic_keyboard_arrow_right,
    "arrow_forward":           ic_arrow_forward,
    "arrow_drop_down":         ic_keyboard_arrow_down,
    # Table ops
    "merge":                   ic_merge,
    "call_split":              ic_call_split,
    "view_headline":           ic_view_headline,
    "add":                     ic_add,
    # Theme / visibility
    "light_mode":              ic_light_mode,
    "dark_mode":               ic_dark_mode,
    "visibility":              ic_visibility,
    # Misc
    "info":                    ic_info,
}

def main() -> int:
    written = 0
    for name, builder in ICONS.items():
        try:
            svg = builder()
        except TypeError as exc:
            # expand_more uses a keyword that doesn't exist on svg_path;
            # fix by using a plain stroke line.
            if "stroke_width_" in str(exc):
                svg = wrap_svg(
                    '<path d="M7 9l5 5 5-5" fill="none" stroke="currentColor" '
                    'stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>'
                )
            else:
                raise
        target = OUT_DIR / f"{name}.svg"
        target.write_text(svg + "\n", encoding="utf-8")
        written += 1

    print(f"[OK] Generated {written} SVG icons -> {OUT_DIR.relative_to(REPO_ROOT)}/")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
