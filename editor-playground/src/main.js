// ════════════════════════════════════════════════════════════════════════════
//  main.js — Vela Docs Editor v0.4.2
//
//  Loaded in two environments:
//   1. Dev: `npm run dev` (live, hot reload)
//   2. Production bundle: `npm run build:production` writes
//      src/editor/dist/editor.bundle.js (loaded by Qt via qrc:)
//
//  v0.4.2 — Polish + Table UX
//    • Hamburger button on the very left of the top bar (Google Docs style).
//      Replaces the colored Files tab from v0.4.1.
//    • Google Docs-style table grid picker — hover over the Insert Table
//      button to reveal a 5×5 grid; click to insert at that size. Default
//      size is 3×3.
//    • Tabbed Table Settings panel — replaces the v0.3.2 modal popup with
//      an inline right-side panel. Tabs: Borders / Cell / Table /
//      Row & column. Every field carries a Material Symbols icon.
//
//  v0.4.1 (carried over):
//    • Material Symbols font: local woff2 (no CDN, works fully offline)
//    • Ribbon redesigned — anchored Files tab, Word-style groups
//    • Welcome / empty-state screen on first launch (welcome.js)
//
//  v0.4.0 (carried over):
//    • Material Symbols icon font throughout the toolbar
//    • New color-picker module (HEX + RGB + Custom + Recent)
//    • New style-manager + Title / Subtitle / CustomStyle extensions
//    • New tab-stop plugin + ruler
//    • New Files-tab backstage overlay (Save / Save As / Open / Print /
//      Print Preview / Options / Help / Recent panels)
//    • "Update '[Style]' to match selection" wired to the style dropdown
// ════════════════════════════════════════════════════════════════════════════

import { dispatcher, registerCommand } from '~dispatcher';

import { Editor } from '@tiptap/core';
import StarterKit from '@tiptap/starter-kit';
import Underline from '@tiptap/extension-underline';
import TextAlign from '@tiptap/extension-text-align';
import Link from '@tiptap/extension-link';
import Placeholder from '@tiptap/extension-placeholder';
import Highlight from '@tiptap/extension-highlight';
import Image from '@tiptap/extension-image';
import TextStyle from '@tiptap/extension-text-style';
import Color from '@tiptap/extension-color';
import TaskList from '@tiptap/extension-task-list';
import TaskItem from '@tiptap/extension-task-item';
import CharacterCount from '@tiptap/extension-character-count';
import Table from '@tiptap/extension-table';
import TableRow from '@tiptap/extension-table-row';
import TableCell from '@tiptap/extension-table-cell';
import TableHeader from '@tiptap/extension-table-header';

import { openInsertImage, promptLink, openStyleEditor, openTabStopDialog } from './modals.js';
import { mountColorPicker } from './color-picker.js';
import { StyleManager } from './style-manager.js';
import { mountRuler } from './ruler.js';
import { TabStopPlugin } from './tab-stop.js';
import { mountTableGridPicker } from './table-picker.js';
import { mountDocumentTabs } from './document-tabs.js';
import { mountTableSettingsPanel } from './table-settings.js';

import { Title } from './extensions/title.js';
import { Subtitle } from './extensions/subtitle.js';
import { CustomStyle } from './extensions/custom-style.js';
import { PageBreak } from './extensions/page-break.js';
import FindReplace, { getSearchState } from './extensions/find-replace.js';
import { mountPagination } from './extensions/pagination.js';
import { loadSettings, applySettings, registerPageSettingsBridge } from './page-settings.js';

import { mountBackstage } from './backstage.js';
import { mountWelcome } from './welcome.js';
import { velaBridge } from './vela-bridge.js';

// Imported here (not only from index.html) so the production bundle carries the
// design system. Without this, `vite build` emits no CSS at all and the Qt
// app's qrc:/editor/dist/editor.bundle.css reference resolves to nothing.
import './styles.css';

const APP_VERSION = '0.4.2';

// ── Sample document ───────────────────────────────────────────────────────
const sampleDoc = {
    type: 'doc',
    content: [
        { type: 'title', content: [{ type: 'text', text: `Welcome to Vela Docs v${APP_VERSION}` }] },
        { type: 'subtitle', content: [{ type: 'text', text: 'Polish & table UX — the hamburger is back, and tables got their own panel.' }] },

        { type: 'paragraph', content: [
            { type: 'text', text: 'This is a ' },
            { type: 'text', marks: [{ type: 'bold' }], text: 'real' },
            { type: 'text', text: ' rich-text editor. Click the ' },
            { type: 'text', marks: [{ type: 'bold' }], text: '☰ menu' },
            { type: 'text', text: ' in the top-left corner (or press ' },
            { type: 'text', marks: [{ type: 'code' }], text: 'Alt+F' },
            { type: 'text', text: ') to open the backstage view. Press ' },
            { type: 'text', marks: [{ type: 'code' }], text: 'Esc' },
            { type: 'text', text: ' to close.' },
        ]},

        { type: 'heading', attrs: { level: 1 }, content: [{ type: 'text', text: `What's new in v${APP_VERSION}` }] },

        { type: 'heading', attrs: { level: 2 }, content: [{ type: 'text', text: '🍔 Hamburger menu (Google Docs style)' }] },
        { type: 'paragraph', content: [
            { type: 'text', marks: [{ type: 'bold' }], text: '1. Burger button on the far left. ' },
            { type: 'text', text: 'Replaces the colored Files tab. Click it to open the backstage overlay (Save / Save As / Open / Print / Print Preview / Options / Help / Recent).' },
        ]},

        { type: 'heading', attrs: { level: 2 }, content: [{ type: 'text', text: '📊 Table UX overhaul' }] },
        { type: 'paragraph', content: [
            { type: 'text', marks: [{ type: 'bold' }], text: '2. Grid picker. ' },
            { type: 'text', text: 'Hover the Insert Table button on the Insert ribbon tab to reveal a 5×5 grid; the highlighted region shows rows × columns. Click to insert a table with that size. Default size is 3×3.' },
        ]},
        { type: 'paragraph', content: [
            { type: 'text', marks: [{ type: 'bold' }], text: '3. Tabbed Table Settings panel. ' },
            { type: 'text', text: 'No more modal popup. Click "Table settings…" on the Table ribbon tab to slide in a side panel with four tabs (Borders / Cell / Table / Row & column). Every field carries a Material Symbols icon.' },
        ]},

        { type: 'heading', attrs: { level: 2 }, content: [{ type: 'text', text: 'Try it yourself' }] },
        { type: 'paragraph', content: [
            { type: 'text', text: 'Switch to the ' },
            { type: 'text', marks: [{ type: 'bold' }], text: 'Insert' },
            { type: 'text', text: ' ribbon tab and hover over the Table button. Drag through the grid to pick a size, then click to insert.' },
        ]},
        { type: 'paragraph', content: [
            { type: 'text', text: 'Then click in the new table, switch to the ' },
            { type: 'text', marks: [{ type: 'bold' }], text: 'Table' },
            { type: 'text', text: ' ribbon tab, and click ' },
            { type: 'text', marks: [{ type: 'bold' }], text: 'Table settings…' },
            { type: 'text', text: ' to open the new side panel.' },
        ]},
    ],
};

// ── Editor instance ───────────────────────────────────────────────────────
const editor = new Editor({
    element: document.querySelector('.editor'),
    extensions: [
        StarterKit.configure({ heading: { levels: [1, 2, 3, 4, 5, 6] } }),
        Underline,
        TextStyle,
        Color,
        TextAlign.configure({ types: ['heading', 'paragraph', 'title', 'subtitle'] }),
        Link.configure({ openOnClick: false, autolink: true }),
        Highlight.configure({ multicolor: true }),
        Image.configure({ inline: false, allowBase64: true }),
        Table.configure({ resizable: true, allowTableNodeSelection: true }),
        TableRow, TableHeader, TableCell,
        TaskList, TaskItem.configure({ nested: true }),
        CharacterCount,
        Placeholder.configure({ placeholder: 'Start writing…' }),
        // ── v0.4.0 extensions ────────────────────────────────────
        Title,
        Subtitle,
        CustomStyle,
        // ── v0.5.0 extensions ────────────────────────────────────
        // PageBreak must come AFTER StarterKit so its Mod-Enter shortcut
        // wins over HardBreak's Mod-Enter (Shift-Enter still = HardBreak).
        PageBreak,
        FindReplace,
        // Pagination is registered as a raw ProseMirror plugin at runtime
        // via mountPagination().enable() (see below) — not as a TipTap
        // extension — so enabling/disabling paging mode doesn't trigger a
        // full extension rebuild or lose the editor selection.
        TabStopPlugin({ pageWidthInches: 8.5, marginLeftInches: 1.0, marginRightInches: 1.0 }),
    ],
    content: sampleDoc,
    onUpdate: () => { updateStats(); updateTableButtons(); syncRulerWithEditor(); },
    onSelectionUpdate: () => { updateActiveStates(); updateTableButtons(); updateCellCoords(); updateStyleDropdownLabel(); syncRulerWithEditor(); },
});

window.__editor = editor;
window.__docAsJSON = () => JSON.stringify(editor.getJSON(), null, 2);
window.__docAsHTML = () => editor.getHTML();

// ── Find & replace bar ────────────────────────────────────────────────────
// Ctrl+F / Ctrl+H / Ctrl+G. The extension swallows those keys inside the
// contenteditable and re-broadcasts them as `vela:findkey`, so both the
// in-editor and document-level paths land here.
window.__findBar = (() => {
    const css = document.createElement('style');
    css.textContent = `
        .vela-findbar{position:fixed;top:64px;right:24px;z-index:4000;display:flex;gap:6px;
            align-items:center;padding:8px 10px;border:1px solid var(--border,#d9d9d9);
            border-radius:10px;background:var(--surface,#fff);box-shadow:0 8px 24px rgba(0,0,0,.16);
            font:12px/1.4 Inter,system-ui,sans-serif;color:var(--text,#1f1f1f)}
        .vela-findbar[hidden]{display:none}
        .vela-findbar input{width:150px;padding:4px 7px;border:1px solid var(--border,#d9d9d9);
            border-radius:6px;background:transparent;color:inherit;font:inherit;outline:none}
        .vela-findbar input:focus{border-color:#4c8bf5}
        .vela-findbar button{padding:4px 8px;border:1px solid var(--border,#d9d9d9);border-radius:6px;
            background:transparent;color:inherit;font:inherit;cursor:pointer}
        .vela-findbar button:hover{background:rgba(0,0,0,.06)}
        .vela-findbar button.is-on{background:#4c8bf5;border-color:#4c8bf5;color:#fff}
        .vela-findbar .vela-findbar-count{min-width:56px;text-align:center;opacity:.7}
        .vela-findbar .vela-findbar-row2{display:flex;gap:6px;align-items:center}
        .vela-findbar-stack{display:flex;flex-direction:column;gap:6px}
        .vela-find-hit{background:#ffe08a;border-radius:2px}
        .vela-find-hit--active{background:#ff9632;outline:1px solid #e2711d}
    `;
    document.head.appendChild(css);

    const bar = document.createElement('div');
    bar.className = 'vela-findbar';
    bar.hidden = true;
    bar.innerHTML = `
        <div class="vela-findbar-stack">
            <div>
                <input data-find type="text" placeholder="Find" aria-label="Find">
                <button data-prev title="Previous (Ctrl+Shift+G)">&#8249;</button>
                <button data-next title="Next (Ctrl+G)">&#8250;</button>
                <span class="vela-findbar-count" data-count>No results</span>
                <button data-case title="Match case">Aa</button>
                <button data-word title="Whole word">&#8220;&#8221;</button>
                <button data-close title="Close (Esc)">&#10005;</button>
            </div>
            <div class="vela-findbar-row2" data-replacerow hidden>
                <input data-replace type="text" placeholder="Replace with" aria-label="Replace with">
                <button data-one>Replace</button>
                <button data-all>Replace all</button>
            </div>
        </div>`;
    document.body.appendChild(bar);

    const q = (s) => bar.querySelector(s);
    const findInput = q('[data-find]');
    const replaceInput = q('[data-replace]');
    const countEl = q('[data-count]');
    const caseBtn = q('[data-case]');
    const wordBtn = q('[data-word]');
    let opts = { caseSensitive: false, wholeWord: false };

    const refreshCount = () => {
        const s = getSearchState(editor);
        countEl.textContent = !s.term
            ? 'No results'
            : s.total === 0
                ? 'No results'
                : `${s.activeIndex + 1} of ${s.total}`;
    };

    const search = () => {
        editor.commands.setSearch(findInput.value, opts);
        refreshCount();
    };

    const open = (withReplace) => {
        bar.hidden = false;
        q('[data-replacerow]').hidden = !withReplace;
        findInput.focus();
        findInput.select();
        search();
    };
    const close = () => {
        bar.hidden = true;
        editor.commands.clearSearch();
        editor.commands.focus();
    };

    findInput.addEventListener('input', search);
    findInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') { e.preventDefault(); editor.commands.findStep(e.shiftKey ? -1 : 1); refreshCount(); }
        if (e.key === 'Escape') { e.preventDefault(); close(); }
    });
    replaceInput.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') { e.preventDefault(); close(); }
    });
    q('[data-next]').addEventListener('click', () => { editor.commands.findStep(1); refreshCount(); });
    q('[data-prev]').addEventListener('click', () => { editor.commands.findStep(-1); refreshCount(); });
    q('[data-one]').addEventListener('click', () => {
        editor.commands.replaceCurrent(replaceInput.value);
        editor.commands.findStep(1);
        refreshCount();
    });
    q('[data-all]').addEventListener('click', () => {
        editor.commands.replaceAll(replaceInput.value);
        refreshCount();
    });
    q('[data-close]').addEventListener('click', close);
    caseBtn.addEventListener('click', () => {
        opts.caseSensitive = !opts.caseSensitive;
        caseBtn.classList.toggle('is-on', opts.caseSensitive);
        search();
    });
    wordBtn.addEventListener('click', () => {
        opts.wholeWord = !opts.wholeWord;
        wordBtn.classList.toggle('is-on', opts.wholeWord);
        search();
    });

    // Keys pressed inside the editor arrive as this custom event.
    document.addEventListener('vela:findkey', (e) => {
        const { key, backwards } = e.detail || {};
        if (!bar.hidden && key === 'g') { editor.commands.findStep(backwards ? -1 : 1); refreshCount(); return; }
        open(key === 'h');
    });
    // Keys pressed anywhere else (ribbon, sidebar, body).
    document.addEventListener('keydown', (e) => {
        if (!(e.ctrlKey || e.metaKey)) {
            if (e.key === 'Escape' && !bar.hidden) close();
            return;
        }
        const k = e.key.toLowerCase();
        if (k === 'f' || k === 'h') { e.preventDefault(); open(k === 'h'); }
        else if (k === 'g' && !bar.hidden) { e.preventDefault(); editor.commands.findStep(e.shiftKey ? -1 : 1); refreshCount(); }
    });
    // Keep highlights honest while the user types.
    editor.on('update', () => { if (!bar.hidden) refreshCount(); });

    return { open, close, search, refreshCount, el: bar };
})();


// ── Native bridge ─────────────────────────────────────────────────────────
// In the Qt shell this is what makes File > Save persist anything: the editor
// pushes its ProseMirror JSON into DocumentController, and pulls the document
// back on startup. In a plain browser every call resolves {ok:false} and the
// playground behaves exactly as before.
velaBridge.connect().then(async () => {
    if (!velaBridge.isNative) return;

    const loaded = await velaBridge.getBody();
    if (loaded.ok && loaded.body && Array.isArray(loaded.body.content)
        && loaded.body.content.length > 0) {
        editor.commands.setContent(loaded.body, false);
    }

    // Debounced push so a burst of typing is one bridge call, not hundreds.
    let pushTimer = 0;
    editor.on('update', () => {
        clearTimeout(pushTimer);
        pushTimer = setTimeout(() => {
            velaBridge.setBody(editor.getJSON()).then((r) => {
                if (!r.ok) console.warn('[vela-bridge] setBody failed:', r.error);
            });
        }, 400);
    });
});

// ── Pagination (v0.5.0) ───────────────────────────────────────────────────
// Mount the controller and apply persisted page settings. Default mode is
// 'pageless' (preserves the legacy infinite-scroll experience); switching to
// 'paging' enables true A4 auto-flow pagination.
const pagination = mountPagination(editor);
registerPageSettingsBridge(pagination, (s) => {
    // Refresh the Format tab's mode toggle if visible.
    document.querySelectorAll('[data-cmd="setPageMode"]').forEach((b) =>
        b.classList.toggle('is-active', b.dataset.arg === s.mode));
});
const initialSettings = loadSettings();
if (initialSettings.mode === 'paging') applySettings(pagination, initialSettings);
window.__velaPagination = pagination;

// ── Style manager (seed built-ins + load custom) ─────────────────────────
StyleManager.seedBuiltinStyles();

// ── Stats ─────────────────────────────────────────────────────────────────
function updateStats() {
    if (!editor) return;
    const text = editor.getText();
    const words = text.trim() ? text.trim().split(/\s+/).length : 0;
    const chars = text.length;
    // In paging mode the PaginationEngine tracks the real page count; fall
    // back to a word-count estimate in pageless mode (legacy behaviour).
    const inPaging = pagination?.isEnabled?.();
    const pages = inPaging
        ? (window.__velaPageCount || 1)
        : Math.max(1, Math.ceil(words / 250));
    const mins = Math.max(1, Math.round(words / 200));
    const wordsEl = document.getElementById('page-footer-words');
    const statsEl = document.getElementById('page-footer-stats');
    if (wordsEl) wordsEl.textContent =
        `${words.toLocaleString()} ${words === 1 ? 'word' : 'words'} · ${chars.toLocaleString()} chars`;
    if (statsEl) statsEl.textContent =
        `Page 1 of ${pages} · ${mins} min read`;
    // The DOM footer above is only visible inside the web view; this is what
    // feeds the native Qt status bar. No-op outside the Qt shell.
    velaBridge.reportStats(words, chars, pages);
}

// ── Active-state tracking ─────────────────────────────────────────────────
function updateActiveStates() {
    const buttons = document.querySelectorAll('button.tool[data-cmd], button.tool-pill[data-cmd]');
    buttons.forEach((btn) => {
        const cmd = btn.dataset.cmd;
        const arg = btn.dataset.arg;
        let active = false;
        if (!editor) return;
        try {
            switch (cmd) {
                case 'bold':      active = editor.isActive('bold'); break;
                case 'italic':    active = editor.isActive('italic'); break;
                case 'underline': active = editor.isActive('underline'); break;
                case 'strike':    active = editor.isActive('strike'); break;
                case 'heading':
                    if (arg === 'p') active = editor.isActive('paragraph');
                    else active = editor.isActive('heading', { level: parseInt(arg.slice(1)) });
                    break;
                case 'title':     active = editor.isActive('title'); break;
                case 'subtitle':  active = editor.isActive('subtitle'); break;
                case 'align':     active = editor.isActive({ textAlign: arg }); break;
                case 'bulletList':  active = editor.isActive('bulletList'); break;
                case 'orderedList': active = editor.isActive('orderedList'); break;
                case 'taskList':    active = editor.isActive('taskList'); break;
                case 'blockquote':  active = editor.isActive('blockquote'); break;
                case 'codeBlock':   active = editor.isActive('codeBlock'); break;
                case 'link':        active = editor.isActive('link'); break;
                case 'textColor': {
                    const c = editor.getAttributes('textStyle')?.color;
                    active = !!c;
                    if (c) updateSwatchBar(btn, c);
                    break;
                }
                case 'highlight': {
                    const c = editor.getAttributes('highlight')?.color;
                    active = !!c && c !== 'none';
                    if (c) updateSwatchBar(btn, c);
                    break;
                }
            }
        } catch (_) { /* not ready */ }
        btn.classList.toggle('is-active', active);
    });
}

function updateSwatchBar(btn, color) {
    const bar = btn.querySelector('.swatch-bar');
    if (bar && color && color !== 'none') bar.style.background = color;
}

// ── Table button gating ───────────────────────────────────────────────────
const tableButtons = [
    'addRowBefore', 'addRowAfter', 'deleteRow',
    'addColBefore', 'addColAfter', 'deleteCol',
    'mergeCells', 'splitCell', 'toggleHeader', 'deleteTable',
    'openTableSettings',
];

function updateTableButtons() {
    const inTable = editor.isActive('table');
    tableButtons.forEach((cmd) => {
        document.querySelectorAll(`[data-cmd="${cmd}"]`).forEach((b) => {
            b.disabled = !inTable;
        });
    });
}

function updateCellCoords() {
    const el = document.getElementById('page-footer-cell-coords');
    if (!el) return;
    if (!editor.isActive('table')) { el.hidden = true; return; }
    const { from } = editor.state.selection;
    const dom = editor.view.nodeDOM(from);
    const cell = dom && dom.closest ? dom.closest('td, th') : null;
    let row = 1, col = 1;
    if (cell) {
        const tr = cell.parentElement;
        if (tr) row = Array.from(tr.parentElement.children).indexOf(tr) + 1;
        col = Array.from(cell.parentElement.children).indexOf(cell) + 1;
    }
    el.textContent = `R${row} C${col}`;
    el.hidden = false;
}

// ── Insert a table at the current cursor position (v0.4.2) ───────────────
function insertTableAtCursor(rows, cols) {
    // Move to end of current block so the table appears below, not inside
    const { $from } = editor.state.selection;
    const endPos = $from.end($from.depth);
    editor.chain().focus().insertContentAt(endPos, {
        type: 'paragraph',
    }).run();
    editor.chain().focus().insertTable({
        rows, cols, withHeaderRow: true,
    }).run();
    switchTab('table');
    flashStatus(`✓ Inserted ${rows} × ${cols} table`);
}

// ── Apply table settings (v0.4.2) ────────────────────────────────────────
function applyTableSettings(values) {
    if (!editor.isActive('table')) return;

    // Walk all rows and cells; apply attributes
    const { state } = editor;
    const tableType = state.schema.nodes.table;
    const rowType = state.schema.nodes.tableRow;
    const cellType = state.schema.nodes.tableCell;
    const headerType = state.schema.nodes.tableHeader;

    // Build CSS border style string
    const borderStyle = values.borderStyle || 'single';
    const borderColor = values.borderColor || '#888888';
    const borderWidth = values.borderWidth ?? 1;
    const borderCss = (borderStyle === 'none' || values.borderPreset === 'none')
        ? 'none'
        : `${borderWidth}px ${borderStyle} ${borderColor}`;

    // Determine which borders to apply based on preset
    const applyBorders = (cell, isFirstRow, isLastRow, isFirstCol, isLastCol) => {
        const style = cell.attrs.style || '';
        let newStyle = style;

        // Strip existing border declarations
        newStyle = newStyle.replace(/border[^:]*:[^;]*;?/gi, '').trim();

        const borders = [];
        if (values.borderPreset === 'all' || values.borderPreset === 'outside') {
            if (isFirstRow)    borders.push(`border-top: ${borderCss}`);
            if (isLastRow)     borders.push(`border-bottom: ${borderCss}`);
            if (isFirstCol)    borders.push(`border-left: ${borderCss}`);
            if (isLastCol)     borders.push(`border-right: ${borderCss}`);
        }
        if (values.borderPreset === 'all' || values.borderPreset === 'inside') {
            // Apply to all sides
            if (values.borderPreset === 'inside') {
                borders.length = 0;
                borders.push(`border-top: ${borderCss}`);
                borders.push(`border-bottom: ${borderCss}`);
                borders.push(`border-left: ${borderCss}`);
                borders.push(`border-right: ${borderCss}`);
            }
        }
        if (values.borderPreset === 'none') {
            borders.length = 0;
            borders.push('border: none');
        }

        // Padding & spacing
        borders.push(`padding: ${values.cellPadding}px`);
        if (values.cellSpacing > 0) {
            // Cell spacing is hard to express on individual cells in HTML;
            // we approximate via a transparent border on each cell.
            borders.push(`border-spacing: ${values.cellSpacing}px`);
        }

        // Vertical alignment
        borders.push(`vertical-align: ${values.verticalAlign}`);

        // Cell shading
        if (values.cellShading && values.cellShading !== '#ffffff' && values.cellShading !== '#fff') {
            borders.push(`background-color: ${values.cellShading}`);
        }

        newStyle = borders.join('; ');
        return chain().updateAttributes(cell.type.name, { style: newStyle });
    };

    // Apply via a transaction that walks the table node
    let tr = state.tr;
    let modified = false;

    state.doc.descendants((node, pos) => {
        if (node.type === tableType) {
            const tableNode = node;
            const rowNodes = [];
            tableNode.forEach((rowNode, rowOffset, rowIndex) => {
                if (rowNode.type === rowType) {
                    const cells = [];
                    rowNode.forEach((cellNode, cellOffset, cellIndex) => {
                        if (cellNode.type === cellType || cellNode.type === headerType) {
                            const isFirstRow = rowIndex === 0;
                            const isLastRow = rowIndex === tableNode.childCount - 1;
                            const isFirstCol = cellIndex === 0;
                            const isLastCol = cellIndex === rowNode.childCount - 1;
                            let newStyle = (cellNode.attrs.style || '').replace(/border[^:]*:[^;]*;?/gi, '').trim();
                            const borders = [];
                            if (values.borderPreset === 'none' || borderStyle === 'none') {
                                borders.push('border: none');
                            } else if (values.borderPreset === 'all') {
                                borders.push(`border: ${borderCss}`);
                            } else if (values.borderPreset === 'outside') {
                                if (isFirstRow) borders.push(`border-top: ${borderCss}`);
                                if (isLastRow)  borders.push(`border-bottom: ${borderCss}`);
                                if (isFirstCol) borders.push(`border-left: ${borderCss}`);
                                if (isLastCol)  borders.push(`border-right: ${borderCss}`);
                            } else if (values.borderPreset === 'inside') {
                                borders.push(`border-top: ${borderCss}`);
                                borders.push(`border-bottom: ${borderCss}`);
                                borders.push(`border-left: ${borderCss}`);
                                borders.push(`border-right: ${borderCss}`);
                            }
                            borders.push(`padding: ${values.cellPadding}px`);
                            borders.push(`vertical-align: ${values.verticalAlign}`);
                            if (values.cellShading && values.cellShading !== '#ffffff' && values.cellShading !== '#fff') {
                                borders.push(`background-color: ${values.cellShading}`);
                            }
                            const finalStyle = borders.join('; ');
                            tr = tr.setNodeMarkup(pos + 1 + rowOffset + 1 + cellOffset, undefined, {
                                ...cellNode.attrs,
                                style: finalStyle,
                            });
                            modified = true;
                        }
                    });
                }
            });
        }
    });

    if (modified) {
        editor.view.dispatch(tr);
    }
    flashStatus(`✓ Applied table settings`);
}

// ── Command dispatcher ────────────────────────────────────────────────────
async function execCommand(cmd, arg) {
    const chain = editor.chain().focus();
    switch (cmd) {
        // ── Inline marks ──
        case 'bold':       chain.toggleBold().run(); break;
        case 'italic':     chain.toggleItalic().run(); break;
        case 'underline':  chain.toggleUnderline().run(); break;
        case 'strike':     chain.toggleStrike().run(); break;

        // ── Heading / paragraph styles ──
        case 'heading':
            if (arg === 'p') chain.setParagraph().run();
            else chain.toggleHeading({ level: parseInt(arg.slice(1)) }).run();
            break;
        case 'title':
            chain.setNode('title').run();
            break;
        case 'subtitle':
            chain.setNode('subtitle').run();
            break;
        case 'applyStyle':
            StyleManager.apply(editor, arg);
            break;
        case 'updateStyleToMatch': {
            const { $from } = editor.state.selection;
            let para = null;
            for (let d = $from.depth; d >= 0; d--) {
                if ($from.node(d).isTextblock) { para = $from.node(d); break; }
            }
            if (para) {
                StyleManager.updateToMatch(arg, para);
                flashStatus(`✓ Updated '${arg}' to match selection`);
            }
            break;
        }
        case 'createCustomStyle': {
            const def = await openStyleEditor();
            if (def?.id) {
                StyleManager.register({ ...def, builtin: false });
                refreshStyleDropdown();
                StyleManager.apply(editor, def.id);
                flashStatus(`✓ Created style '${def.name}'`);
            }
            break;
        }
        case 'modifyStyle': {
            const existing = StyleManager.get(arg);
            const def = await openStyleEditor(existing);
            if (def?.id) {
                StyleManager.register({ ...def, id: arg });
                refreshStyleDropdown();
                flashStatus(`✓ Modified '${arg}'`);
            }
            break;
        }

        case 'align':      chain.setTextAlign(arg).run(); break;

        // ── Lists ──
        case 'bulletList':   chain.toggleBulletList().run(); break;
        case 'orderedList':  chain.toggleOrderedList().run(); break;
        case 'taskList':     chain.toggleTaskList().run(); break;

        // ── Block ──
        case 'blockquote':   chain.toggleBlockquote().run(); break;
        case 'codeBlock':    chain.toggleCodeBlock().run(); break;

        // ── Indent ──
        case 'indent':  chain.sinkListItem('listItem').run(); break;
        case 'outdent': chain.liftListItem('listItem').run(); break;

        // ── Link ──
        case 'insertLink': {
            if (editor.isActive('link')) chain.unsetLink().run();
            else {
                const url = promptLink();
                if (url) chain.setLink({ href: url }).run();
            }
            break;
        }

        // ── Colors ──
        case 'pickColor': {
            const target = arg && arg.target;
            const color = arg && arg.color;
            if (!color) break;
            if (target === 'textColor') {
                if (color === 'none' || color === 'transparent') {
                    chain.unsetColor().run();
                } else {
                    chain.setColor(color).run();
                }
            } else if (target === 'highlight') {
                if (color === 'none' || color === 'transparent') {
                    chain.unsetHighlight().run();
                } else {
                    chain.setMark('highlight', { color }).run();
                }
            }
            break;
        }

        // ── Tab stops ──
        case 'openTabStopDialog': {
            const { $from } = editor.state.selection;
            let para = null;
            for (let d = $from.depth; d >= 0; d--) {
                if ($from.node(d).isTextblock) { para = $from.node(d); break; }
            }
            const currentStops = para?.attrs?.tabStops || [];
            const updated = await openTabStopDialog(currentStops);
            if (updated) {
                chain.updateAttributes(para.type.name, { tabStops: updated }).run();
                if (window.__ruler) window.__ruler.setTabStops(updated);
                flashStatus(`✓ Set ${updated.length} tab stop(s)`);
            }
            break;
        }

        // ── Insert ──
        case 'openInsertTable': {
            // v0.4.2 — the grid picker handles this automatically; the command
            // is here for keyboard shortcuts (Ctrl+Shift+K inserts default 3×3).
            insertTableAtCursor(3, 3);
            break;
        }
        case 'openInsertImage': {
            const result = await openInsertImage();
            if (result && result.url) {
                editor.chain().focus().setImage({ src: result.url }).run();
            }
            break;
        }
        case 'pageBreak': editor.chain().focus().setPageBreak().run(); break;
        case 'hr': chain.setHorizontalRule().run(); break;
        case 'footnote': {
            editor.chain().focus().insertContent({
                type: 'text', text: '[*]',
                marks: [{ type: 'highlight' }],
            }).run();
            break;
        }
        case 'insertChar': {
            if (arg) editor.chain().focus().insertContent(arg).run();
            break;
        }

        // ── Table ops ──
        case 'addRowBefore': editor.chain().focus().addRowBefore().run(); break;
        case 'addRowAfter':  editor.chain().focus().addRowAfter().run(); break;
        case 'deleteRow':    editor.chain().focus().deleteRow().run(); break;
        case 'addColBefore': editor.chain().focus().addColumnBefore().run(); break;
        case 'addColAfter':  editor.chain().focus().addColumnAfter().run(); break;
        case 'deleteCol':    editor.chain().focus().deleteColumn().run(); break;
        case 'mergeCells':    editor.chain().focus().mergeCells().run(); break;
        case 'splitCell':     editor.chain().focus().splitCells().run(); break;
        case 'toggleHeader':  editor.chain().focus().toggleHeaderRow().run(); break;
        case 'deleteTable':   editor.chain().focus().deleteTable().run(); break;
        case 'openTableSettings': {
            const panel = document.getElementById('table-settings-panel');
            if (panel) {
                panel.hidden = false;
                editor.commands.focus();
            }
            break;
        }

        // ── History ──
        case 'undo': editor.chain().focus().undo().run(); break;
        case 'redo': editor.chain().focus().redo().run(); break;

        // ── Clear ──
        case 'clearFormatting':
            editor.chain().focus()
                .unsetBold().unsetItalic().unsetUnderline().unsetStrike()
                .unsetHighlight().unsetMark('textStyle')
                .run();
            break;

        // ── Files / Save / Export ──
        case 'save':
            flashStatus('✓ Saved · just now');
            window.vela?.saveDocument?.(JSON.stringify(editor.getJSON()));
            break;
        case 'export':
            downloadHtml();
            break;
        case 'clear':
            if (confirm('Clear the entire document?')) {
                editor.commands.clearContent();
                editor.commands.focus();
            }
            break;

        // ── Theme ──
        case 'toggleTheme':
            toggleTheme();
            break;

        // ── Page settings (v0.5.0) ────────────────────────────────────
        case 'setPageMode': {
            // arg: 'paging' | 'pageless'
            const s = { ...loadSettings(), mode: arg };
            applySettings(pagination, s);
            document.querySelectorAll('[data-cmd="setPageMode"]').forEach((b) =>
                b.classList.toggle('is-active', b.dataset.arg === arg));
            flashStatus(arg === 'paging' ? '✓ Paging view (A4)' : '✓ Pageless view');
            break;
        }
        case 'setOrientation': {
            // arg: 'portrait' | 'landscape'
            const s = { ...loadSettings(), orientation: arg };
            applySettings(pagination, s);
            document.querySelectorAll('[data-cmd="setOrientation"]').forEach((b) =>
                b.classList.toggle('is-active', b.dataset.arg === arg));
            flashStatus(`✓ ${arg === 'landscape' ? 'Landscape' : 'Portrait'}`);
            break;
        }
        case 'setPageSize': {
            // arg: 'A4' | 'Letter' | 'Legal' | 'Tabloid'
            const s = { ...loadSettings(), size: arg };
            applySettings(pagination, s);
            document.querySelectorAll('[data-cmd="setPageSize"]').forEach((b) =>
                b.classList.toggle('is-active', b.dataset.arg === arg));
            flashStatus(`✓ Paper size: ${arg}`);
            break;
        }
        case 'togglePageMode': {
            const cur = loadSettings().mode;
            execCommand('setPageMode', cur === 'paging' ? 'pageless' : 'paging');
            break;
        }
        case 'openPageSetup': {
            // Defer to native dialog when running under Qt; otherwise a future
            // web modal. For now, flip to Format tab.
            switchTab('format');
            break;
        }

        // ── Header & Footer (v0.5.0) ──────────────────────────────────
        // For v1 the header/footer text is stored in page settings and
        // rendered per-sheet by the PaginationEngine overlay. Rich header
        // editing (images, tables) is deferred.
        case 'editHeader': {
            const cur = loadSettings().headerText || '';
            const next = window.prompt('Header text (use {PAGE} for page number):', cur);
            if (next !== null) {
                applySettings(pagination, { ...loadSettings(), headerText: next });
                flashStatus('✓ Header updated');
            }
            break;
        }
        case 'editFooter': {
            const cur = loadSettings().footerText || '';
            const next = window.prompt('Footer text (use {PAGE} for page number):', cur);
            if (next !== null) {
                applySettings(pagination, { ...loadSettings(), footerText: next });
                flashStatus('✓ Footer updated');
            }
            break;
        }
        case 'insertPageNumber': {
            // Append "Page {PAGE} of {NUMPAGES}" to the footer if empty.
            const cur = loadSettings();
            const footer = cur.footerText || '';
            if (footer.includes('{PAGE}')) {
                flashStatus('Footer already has a page number');
            } else {
                const next = footer ? `${footer} · Page {PAGE} of {NUMPAGES}` : 'Page {PAGE} of {NUMPAGES}';
                applySettings(pagination, { ...cur, footerText: next });
                flashStatus('✓ Page number inserted in footer');
            }
            break;
        }

        // ── Ribbon tabs ──
        case 'switchTab':
            switchTab(arg);
            break;

        // ── Files tab / Backstage ──
        case 'toggleFiles':
            window.__backstage?.toggle();
            break;
        case 'openBackstagePanel':
            window.__backstage?.open(arg);
            break;
        case 'closeBackstage':
            window.__backstage?.close();
            break;
    }
    updateActiveStates();
    updateTableButtons();
    updateCellCoords();
}

window.__exec = execCommand;

// ── Wire up all command buttons ───────────────────────────────────────────
document.querySelectorAll('[data-cmd]').forEach((btn) => {
    btn.addEventListener('mousedown', (e) => {
        if (btn.dataset.cmd === 'textColor' || btn.dataset.cmd === 'highlight') return;
        e.preventDefault();
    });
});

document.addEventListener('click', (e) => {
    const btn = e.target.closest('[data-cmd]');
    if (!btn) return;
    e.preventDefault();
    execCommand(btn.dataset.cmd, btn.dataset.arg);
});

// ── Color pickers (v0.4.0) ───────────────────────────────────────────────
mountColorPicker('textColor', document.getElementById('btn-text-color'), {
    onApply: (color) => execCommand('pickColor', { target: 'textColor', color }),
});
mountColorPicker('highlight', document.getElementById('btn-highlight'), {
    onApply: (color) => execCommand('pickColor', { target: 'highlight', color }),
});

// ── Style dropdown (v0.4.0) ───────────────────────────────────────────────
function buildStyleDropdown() {
    const menu = document.getElementById('style-menu');
    if (!menu) return;

    const list = StyleManager.list();
    const builtIn = list.filter((s) => s.builtin);
    const custom = list.filter((s) => !s.builtin);
    const currentId = StyleManager.currentStyleId(editor);

    const items = [];

    if (currentId) {
        items.push(`<button class="style-item is-current" data-action="update" data-id="${currentId}">
            Update '${currentId}' to match selection
        </button>`);
        items.push(`<button class="style-item" data-action="modify" data-id="${currentId}">
            Modify '${currentId}'…
        </button>`);
        items.push(`<hr/>`);
    }

    items.push(`<div class="style-section-label">Built-in Styles</div>`);
    builtIn.forEach((s) => {
        items.push(`<button class="style-item" data-action="apply" data-id="${s.id}">${s.name}</button>`);
    });

    if (custom.length) {
        items.push(`<hr/>`);
        items.push(`<div class="style-section-label">Custom Styles</div>`);
        custom.forEach((s) => {
            items.push(`<button class="style-item" data-action="apply" data-id="${s.id}">${s.name}</button>`);
        });
    }

    items.push(`<hr/>`);
    items.push(`<button class="style-item is-create" data-action="create">+ New Style…</button>`);

    menu.innerHTML = items.join('');

    menu.addEventListener('click', (e) => {
        const btn = e.target.closest('[data-action]');
        if (!btn) return;
        e.preventDefault();
        const action = btn.dataset.action;
        const id = btn.dataset.id;
        if (action === 'apply') {
            execCommand('applyStyle', id);
            closeStyleDropdown();
        } else if (action === 'update') {
            execCommand('updateStyleToMatch', id);
            closeStyleDropdown();
        } else if (action === 'modify') {
            execCommand('modifyStyle', id);
            closeStyleDropdown();
        } else if (action === 'create') {
            execCommand('createCustomStyle');
            closeStyleDropdown();
        }
    });
}

function refreshStyleDropdown() {
    const menu = document.getElementById('style-menu');
    if (menu && !menu.hidden) buildStyleDropdown();
}

function openStyleDropdown() {
    buildStyleDropdown();
    const menu = document.getElementById('style-menu');
    const trigger = document.getElementById('style-trigger');
    if (!menu || !trigger) return;

    // Portal the menu to <body> and position it fixed so it escapes the
    // toolbar's overflow:hidden clip (.tab-panel has overflow-y:hidden).
    if (menu.dataset.portaled !== '1') {
        document.body.appendChild(menu);
        menu.dataset.portaled = '1';
    }

    const rect = trigger.getBoundingClientRect();
    menu.style.left = `${rect.left}px`;
    menu.style.top = `${rect.bottom + 4}px`;
    menu.hidden = false;
    document.addEventListener('click', closeStyleDropdownOnOutsideClick);
}
function closeStyleDropdown() {
    const menu = document.getElementById('style-menu');
    if (menu) menu.hidden = true;
    document.removeEventListener('click', closeStyleDropdownOnOutsideClick);
}
function closeStyleDropdownOnOutsideClick(e) {
    if (e.target.closest('#style-dropdown')) return;
    if (e.target.closest('#style-menu')) return;
    closeStyleDropdown();
}

function updateStyleDropdownLabel() {
    const label = document.querySelector('#style-dropdown .style-name');
    if (!label) return;
    const id = StyleManager.currentStyleId(editor);
    const def = StyleManager.get(id);
    label.textContent = def?.name || id || 'Normal';
}

document.getElementById('style-trigger')?.addEventListener('click', (e) => {
    e.preventDefault();
    e.stopPropagation();
    const menu = document.getElementById('style-menu');
    if (menu?.hidden) openStyleDropdown();
    else closeStyleDropdown();
});

// ── Ruler (v0.4.0) ────────────────────────────────────────────────────────
window.__ruler = mountRuler(document.getElementById('ruler'), {
    pageWidthInches: 8.5,
    marginLeftInches: 1.0,
    marginRightInches: 1.0,
    tabStops: [],
    onTabStopChange: (stops) => {
        const { $from } = editor.state.selection;
        for (let d = $from.depth; d >= 0; d--) {
            const n = $from.node(d);
            if (n.isTextblock) {
                editor.chain().focus().updateAttributes(n.type.name, { tabStops: stops }).run();
                return;
            }
        }
    },
    onMarginChange: (margins) => {
        flashStatus(`✓ Margin: left ${margins.left.toFixed(2)}″, right ${margins.right.toFixed(2)}″`);
    },
    onTabStopEdit: (currentStop, onResult) => {
        const result = window.confirm(
            `Tab stop at ${currentStop.pos.toFixed(2)}″\nType: ${currentStop.type}\nLeader: ${currentStop.leader}`,
        );
        onResult(result ? currentStop : null);
    },
});

function syncRulerWithEditor() {
    const { $from } = editor.state.selection;
    for (let d = $from.depth; d >= 0; d--) {
        const n = $from.node(d);
        if (n.isTextblock) {
            const stops = n.attrs?.tabStops || [];
            window.__ruler?.setTabStops(stops);
            return;
        }
    }
    window.__ruler?.setTabStops([]);
}

// ── Backstage (v0.4.0) ────────────────────────────────────────────────────
window.__backstage = mountBackstage(document.getElementById('backstage'), {
    onSave: () => execCommand('save'),
    onSaveAs: () => alert('Save As — choose a file format and location'),
    onOpen:  () => alert('Open — pick a file'),
    onPrint: () => window.print(),
    onPrintPreview: () => window.print(),
});

// ── Table grid picker (v0.4.2 — Google Docs style) ──────────────────────
mountTableGridPicker({
    button: document.getElementById('btn-insert-table'),
    popover: document.getElementById('table-grid-popover'),
    grid:    document.getElementById('table-grid'),
    label:   document.getElementById('table-grid-label'),
    onInsert: ({ rows, cols }) => insertTableAtCursor(rows, cols),
});

// ── Table settings panel (v0.4.2 — tabbed, inline, not modal) ───────────
mountTableSettingsPanel({
    panel:    document.getElementById('table-settings-panel'),
    tabs:     Array.from(document.querySelectorAll('.tsp-tab')),
    panes:    Array.from(document.querySelectorAll('.tsp-pane')),
    closeBtn: document.getElementById('tsp-close'),
    footer:   document.querySelector('.tsp-footer'),
    editor,
    onApply:  (values) => applyTableSettings(values),
    onReset:  () => flashStatus('✓ Table settings reset'),
});

window.__docTabs = mountDocumentTabs({
    sidebar: document.getElementById('document-tabs-sidebar'),
    list:    document.getElementById('dts-list'),
    addBtn:  document.getElementById('dts-add'),
    onSelect: (id) => flashStatus(`✓ Switched to ${id}`),
    onCreate:  () => flashStatus('✓ New document created'),
    onClose:   () => flashStatus('✓ Document closed'),
});
document.getElementById('btn-document-tabs')?.addEventListener('click', () => {
    window.__docTabs?.toggle();
});
document.getElementById('dts-close')?.addEventListener('click', () => {
    window.__docTabs?.close();
});


// Close table settings panel on Escape
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        const panel = document.getElementById('table-settings-panel');
        if (panel && !panel.hidden) {
            panel.hidden = true;
            e.stopPropagation();
        }
    }
});

// ── Ribbon tab switching ──────────────────────────────────────────────────
function switchTab(name) {
    document.querySelectorAll('.ribbon-tab').forEach((t) =>
        t.classList.toggle('is-active', t.dataset.tab === name));
    document.querySelectorAll('.tab-panel').forEach((p) => {
        const match = p.dataset.panel === name;
        p.hidden = !match;
    });
}
document.querySelectorAll('.ribbon-tab').forEach((tab) => {
    tab.addEventListener('click', () => switchTab(tab.dataset.tab));
});

// ── Font selectors ────────────────────────────────────────────────────────
const fontSelect = document.getElementById('font-family');
const sizeSelect = document.getElementById('font-size');

fontSelect?.addEventListener('change', () => {
    editor.chain().focus().setMark('textStyle', { fontFamily: fontSelect.value }).run();
});
sizeSelect?.addEventListener('change', () => {
    const px = parseInt(sizeSelect.value, 10);
    if (!isNaN(px)) editor.chain().focus().setMark('textStyle', { fontSize: `${px}px` }).run();
});

// ── Page size selector (Format tab, v0.5.0) ───────────────────────────────
const pageSizeSelect = document.getElementById('page-size-select');
pageSizeSelect?.addEventListener('change', () => {
    execCommand('setPageSize', pageSizeSelect.value);
});

// ── Theme toggle ──────────────────────────────────────────────────────────
const themeBtns = [
    document.getElementById('toggle-theme'),
    document.getElementById('toggle-theme-2'),
];
const themeName = document.getElementById('theme-name');
let dark = false;

function toggleTheme() {
    dark = !dark;
    document.documentElement.dataset.theme = dark ? 'dark' : 'light';
    if (themeName) themeName.textContent = dark ? 'Dark' : 'Light';
    themeBtns.forEach((b) => {
        if (b) b.querySelector('.ms').textContent = dark ? 'dark_mode' : 'light_mode';
    });
}
themeBtns.forEach((b) => b && b.addEventListener('click', toggleTheme));

// ── Status flash ──────────────────────────────────────────────────────────
const statusBar = document.getElementById('status');
function flashStatus(text, ms = 1200) {
    if (!statusBar) return;
    const original = statusBar.innerHTML;
    statusBar.style.background = 'var(--accent-soft)';
    statusBar.textContent = text;
    setTimeout(() => {
        statusBar.style.background = '';
        statusBar.innerHTML = original;
    }, ms);
}

// ── Download as HTML ──────────────────────────────────────────────────────
function downloadHtml() {
    const html = `<!doctype html><html><head><meta charset="utf-8"><title>Document</title>
<style>body{font-family:Georgia,serif;max-width:800px;margin:2em auto;line-height:1.6;color:#222;padding:0 2em}</style>
</head><body>${editor.getHTML()}</body></html>`;
    const blob = new Blob([html], { type: 'text/html' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url; a.download = 'vela-document.html'; a.click();
    URL.revokeObjectURL(url);
    flashStatus('✓ Exported as HTML');
}

// ── Save / Export / Clear buttons ─────────────────────────────────────────
document.getElementById('btn-save')?.addEventListener('click', () => execCommand('save'));
document.getElementById('btn-export')?.addEventListener('click', () => execCommand('export'));
document.getElementById('btn-clear')?.addEventListener('click', () => execCommand('clear'));

// ── Welcome / empty-state overlay (v0.4.1) ──────────────────────────────
window.__welcome = mountWelcome(document.body, {
    version: APP_VERSION,
    onPickTemplate: (id) => {
        editor.commands.clearContent();
        editor.commands.focus();
        flashStatus(`New ${id} document ready`);
    },
    onPickRecent: (item) => {
        flashStatus(`Opening "${item.name}" (recent file plumbing is a v0.4.2 target)`);
        editor.commands.focus();
    },
    recent: [
        { name: 'Q3 Marketing Strategy Overview', subtitle: 'Opened 2 hours ago' },
        { name: 'Project Phoenix Specs',          subtitle: 'Opened yesterday' },
        { name: 'Invoice #4029 - ACME Corp',      subtitle: 'Opened Oct 24, 2026' },
        { name: 'Weekly Meeting Notes',           subtitle: 'Opened Oct 20, 2026' },
    ],
});

// Status bar version chip
(function mountVersionChip() {
    const statusEl = document.getElementById('status');
    if (!statusEl || statusEl.querySelector('.version-chip')) return;
    const chip = document.createElement('span');
    chip.className = 'version-chip';
    chip.textContent = `v${APP_VERSION}`;
    statusEl.insertBefore(chip, statusEl.firstChild);
})();

// ── Hamburger button (v0.4.2 — Google Docs style) ───────────────────────
const hamburger = document.getElementById('btn-files');
hamburger?.addEventListener('click', () => execCommand('toggleFiles'));
// Highlight hamburger when backstage is open
const observer = new MutationObserver(() => {
    if (!hamburger) return;
    const backstage = document.getElementById('backstage');
    hamburger.classList.toggle('is-active', !!(backstage && !backstage.hidden));
});
const backstageEl = document.getElementById('backstage');
if (backstageEl) observer.observe(backstageEl, { attributes: true, attributeFilter: ['hidden'] });

// ── Keyboard shortcuts ────────────────────────────────────────────────────
document.addEventListener('keydown', (e) => {
    const mod = e.ctrlKey || e.metaKey;
    if (!mod) {
        if (e.altKey && e.key.toLowerCase() === 'f') {
            e.preventDefault();
            execCommand('toggleFiles');
        }
        if (e.key === 'Escape') {
            window.__backstage?.close();
            closeStyleDropdown();
        }
        return;
    }
    if (e.target.closest('input, textarea, [contenteditable]')) return;

    const k = e.key.toLowerCase();
    const shift = e.shiftKey;
    const alt = e.altKey;

    if (!shift && !alt) {
        switch (k) {
            case 'b': execCommand('bold'); e.preventDefault(); return;
            case 'i': execCommand('italic'); e.preventDefault(); return;
            case 'u': execCommand('underline'); e.preventDefault(); return;
            case 's': execCommand('save'); e.preventDefault(); return;
            case 'k': execCommand('insertLink'); e.preventDefault(); return;
            case '0': execCommand('heading', 'p'); e.preventDefault(); return;
        }
        if (k >= '1' && k <= '4') {
            execCommand('heading', 'h' + k);
            e.preventDefault(); return;
        }
    }

    if (shift && !alt) {
        switch (k) {
            case 's': execCommand('strike'); e.preventDefault(); return;
            case 'l': execCommand('bulletList'); e.preventDefault(); return;
            case 'o': execCommand('orderedList'); e.preventDefault(); return;
            case '7': execCommand('taskList'); e.preventDefault(); return;
            case 'k': execCommand('openInsertTable'); e.preventDefault(); return;
            case 'z': execCommand('redo'); e.preventDefault(); return;
        }
    }

    if (alt && !shift && k === 'i') {
        execCommand('openInsertImage');
        e.preventDefault(); return;
    }

    if (!shift && !alt && k === ']') { execCommand('indent'); e.preventDefault(); return; }
    if (!shift && !alt && k === '[') { execCommand('outdent'); e.preventDefault(); return; }
});

// ── Init ──────────────────────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', () => {
    updateStats();
    updateActiveStates();
    updateTableButtons();
    updateCellCoords();
    updateStyleDropdownLabel();
});