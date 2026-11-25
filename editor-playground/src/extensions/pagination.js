// ════════════════════════════════════════════════════════════════════════════
//  pagination.js — v0.5.0
//
//  True auto-flow pagination for the Vela Docs editor.
//
//  ARCHITECTURE
//  ────────────
//  ProseMirror's document model is one continuous tree; splitting paragraphs
//  across page-node boundaries breaks editing badly (every PM-based editor
//  avoids it). Instead, we keep the doc continuous and render pagination as a
//  VISUAL layer:
//
//    1. MEASURE — walk every top-level block in the editor DOM and read its
//       natural height (offsetHeight + computed top/bottom margins). Natural
//       height is independent of any pagination decorations already applied,
//       so the measurement is stable and needs no iteration.
//
//    2. ASSIGN — accumulate natural heights; whenever the running total
//       crosses `pageContentHeight` (page height − margins), the next block
//       starts a new page. Explicit `pageBreak` nodes always force a new page.
//       Blocks taller than a full page overflow gracefully (we don't split
//       tables/images in v1).
//
//    3. RENDER — insert a zero-content spacer widget between the last block of
//       one page and the first block of the next, sized to push the next
//       page's content down by exactly (remaining space on prev page) +
//       bottomMargin + pageGap + topMargin. Page backgrounds (white sheets
//       with shadow) are drawn by a separate overlay layer behind the editor.
//
//  The result behaves like Google Docs: content auto-reflows across A4 sheets
//  as you type, resize, or zoom. Recompute is debounced 50ms and only re-runs
//  when page assignments actually change (no infinite loops).
//
//  LIMITATIONS (v1)
//  ────────────────
//    • Blocks taller than one page overflow the sheet boundary rather than
//      being split across pages. Cross-page table row splitting is not
//      supported.
//    • Margins are uniform per document (no per-section margins yet).
//    • The ruler is hidden in paging mode (it's tied to the legacy 8.5in
//      single-page model).
// ════════════════════════════════════════════════════════════════════════════

import { Extension } from '@tiptap/core';
import { Plugin, PluginKey } from '@tiptap/pm/state';
import { Decoration, DecorationSet } from '@tiptap/pm/view';

const PX_PER_IN = 96;
const PX_PER_MM = PX_PER_IN / 25.4;

// Standard paper sizes in mm. Width/height are PORTRAIT dims; landscape swaps.
export const PAPER_SIZES_MM = {
    A4:     { w: 210,   h: 297   },
    Letter: { w: 215.9, h: 279.4 },
    Legal:  { w: 215.9, h: 355.6 },
    Tabloid:{ w: 279.4, h: 431.8 },
};

export const paginationKey = new PluginKey('velaPagination');

/**
 * Build the pagination ProseMirror plugin.
 * @param {object} settings — { size, orientation, marginTop/Right/Bottom/Left (inches), pageGap (px) }
 */
function buildPaginationPlugin(settings) {
    // Resolve current geometry in px.
    function geometry() {
        const dims = PAPER_SIZES_MM[settings.size] || PAPER_SIZES_MM.A4;
        const portrait = settings.orientation !== 'landscape';
        const pageWmm = portrait ? dims.w : dims.h;
        const pageHmm = portrait ? dims.h : dims.w;
        const pageWidthPx  = Math.round(pageWmm * PX_PER_MM);
        const pageHeightPx = Math.round(pageHmm * PX_PER_MM);
        const m = {
            top:    Math.round((settings.marginTop    ?? 1) * PX_PER_IN),
            right:  Math.round((settings.marginRight  ?? 1) * PX_PER_IN),
            bottom: Math.round((settings.marginBottom ?? 1) * PX_PER_IN),
            left:   Math.round((settings.marginLeft   ?? 1) * PX_PER_IN),
        };
        return {
            pageWidthPx, pageHeightPx, m,
            contentWidthPx:  pageWidthPx - m.left - m.right,
            contentHeightPx: pageHeightPx - m.top - m.bottom,
            pageGapPx: settings.pageGap ?? 24,
            sheetOuterHeight: pageHeightPx + (settings.pageGap ?? 24),
        };
    }

    // ── Decoration build ────────────────────────────────────────────────
    // Map of doc pos (block start) → page index. Computed by measure().
    let lastAssignments = null; // [{ pos, page }, ...] sentinel for "unchanged"

    function buildDecorations(state, geom) {
        const decos = [];
        const assignments = []; // { pos, page } per top-level block, in order

        // Walk top-level block nodes, accumulating natural height per page.
        let page = 0;
        let used = 0; // px used on current page's content area
        const doc = state.doc;

        doc.forEach((node, offset) => {
            const pos = offset; // position BEFORE the node (0-based at doc start)
            // Force new page on explicit pageBreak nodes.
            if (node.type.name === 'pageBreak') {
                if (used > 0) { page += 1; used = 0; }
                assignments.push({ pos, page, isBreak: true });
                return;
            }
            // Height lookup happens in view.update() — at decoration build
            // time we don't have live DOM heights, so this function consumes
            // the assignments computed there via plugin state. This buildDecorations
            // is a fallback that produces no spacers; real spacers come from
            // the cached assignment in plugin state.
            assignments.push({ pos, page, isBreak: false });
        });

        return { decos, assignments };
    }

    return new Plugin({
        key: paginationKey,
        state: {
            init() {
                return { assignments: null, geom: geometry(), version: 0 };
            },
            apply(tr, value, oldState, newState) {
                const meta = tr.getMeta(paginationKey);
                if (meta && meta.assignments) {
                    return { assignments: meta.assignments, geom: meta.geom || value.geom, version: value.version + 1 };
                }
                if (tr.docChanged) {
                    // Doc changed — assignments need recompute; mark stale.
                    return { assignments: null, geom: value.geom, version: value.version + 1 };
                }
                return value;
            },
        },
        props: {
            decorations(state) {
                const value = paginationKey.getState(state);
                if (!value || !value.assignments) return DecorationSet.empty;
                // Build spacer widgets from the cached assignments.
                const { assignments, geom } = value;
                const widgets = [];
                // The first block that starts each page after page 0 gets a
                // preceding spacer widget sized to advance to the next sheet.
                let prevPage = 0;
                // usedThisPage tracks how much of the content area was filled
                // on the previous page so the spacer fills the remainder.
                // We recompute by walking assignments in order with their
                // natural heights (cached alongside assignments).
                let usedOnPrevPage = 0;
                for (let i = 0; i < assignments.length; i++) {
                    const a = assignments[i];
                    if (a.page > prevPage || a.isBreak) {
                        // Insert spacer before this block at `a.pos`.
                        const remaining = Math.max(0, geom.contentHeightPx - usedOnPrevPage);
                        const spacerHeight = geom.m.bottom + geom.pageGapPx + geom.m.top
                            + (a.isBreak ? 0 : remaining);
                        widgets.push(Decoration.widget(a.pos, () => {
                            const el = document.createElement('div');
                            el.className = 'vela-page-spacer';
                            el.style.height = `${spacerHeight}px`;
                            el.setAttribute('data-page-spacer', String(a.page));
                            el.setAttribute('contenteditable', 'false');
                            return el;
                        }, { side: -1, key: `pg-${a.pos}` }));
                        prevPage = a.page;
                        usedOnPrevPage = 0;
                    }
                    if (!a.isBreak) {
                        usedOnPrevPage += a.naturalHeight || 0;
                    }
                }
                return DecorationSet.create(state.doc, widgets);
            },
        },
    });
}

// ── Measurement & recompute (runs in DOM, outside PM render) ──────────────
/**
 * Walk the editor DOM, measure each top-level block's natural height, compute
 * page assignments, and if they changed, dispatch a transaction to refresh
 * decorations.
 *
 * @param {EditorView} view
 * @param {object} settings
 */
export function recomputePagination(view, settings) {
    if (!view || !view.dom) return;
    const dims = PAPER_SIZES_MM[settings.size] || PAPER_SIZES_MM.A4;
    const portrait = settings.orientation !== 'landscape';
    const pageWmm = portrait ? dims.w : dims.h;
    const pageHmm = portrait ? dims.h : dims.w;
    const pageHeightPx = Math.round(pageHmm * PX_PER_MM);
    const m = {
        top:    Math.round((settings.marginTop    ?? 1) * PX_PER_IN),
        bottom: Math.round((settings.marginBottom ?? 1) * PX_PER_IN),
    };
    const contentHeightPx = pageHeightPx - m.top - m.bottom;
    const pageGapPx = settings.pageGap ?? 24;

    // Top-level block DOM nodes — ProseMirror renders these as direct children
    // of view.dom (with possible spacer widgets between, which we skip).
    const dom = view.dom;
    const childNodes = Array.from(dom.childNodes);
    const blockNodes = childNodes.filter((n) =>
        n.nodeType === 1 && !n.classList?.contains('vela-page-spacer')
        && !n.classList?.contains('ProseMirror-widget'));

    const doc = view.state.doc;
    const topBlocks = [];
    doc.forEach((node, offset) => topBlocks.push({ node, pos: offset }));

    // Measure each block's natural height. offsetHeight is independent of
    // pagination spacers, so this is stable.
    const assignments = [];
    let page = 0;
    let used = 0;

    for (let i = 0; i < topBlocks.length; i++) {
        const { node, pos } = topBlocks[i];
        const domEl = blockNodes[i];

        if (node.type.name === 'pageBreak') {
            if (used > 0) { page += 1; used = 0; }
            assignments.push({ pos, page, isBreak: true, naturalHeight: 0 });
            continue;
        }

        let naturalHeight = 0;
        if (domEl) {
            const cs = getComputedStyle(domEl);
            const mt = parseFloat(cs.marginTop) || 0;
            const mb = parseFloat(cs.marginBottom) || 0;
            naturalHeight = mt + (domEl.offsetHeight || 0) + mb;
        }

        // Block-too-tall guard: if a single block is taller than a full page,
        // let it occupy from the current position and force a new page next
        // time (it visually overflows the sheet — documented v1 limitation).
        if (used > 0 && used + naturalHeight > contentHeightPx) {
            page += 1;
            used = 0;
        }
        assignments.push({ pos, page, isBreak: false, naturalHeight });
        used += naturalHeight;
        if (naturalHeight > contentHeightPx) {
            // Oversized block — force the NEXT block onto a new page.
            page += 1;
            used = 0;
        }
    }

    // Compare with cached assignments; only dispatch if changed.
    const prev = paginationKey.getState(view.state)?.assignments;
    if (assignmentsEqual(prev, assignments)) return;

    const geom = {
        pageHeightPx, contentHeightPx, m, pageGapPx,
        m_bottom: m.bottom, m_top: m.top,
    };
    const tr = view.state.tr.setMeta(paginationKey, { assignments, geom });
    tr.setMeta('addToHistory', false);
    view.dispatch(tr);

    // Page count → status bar + overlay.
    const pageCount = assignments.length ? (assignments[assignments.length - 1].page + 1) : 1;
    updatePageCount(pageCount);
    renderPageSheets(view.dom, pageCount, settings, geom);
}

function assignmentsEqual(a, b) {
    if (!a || !b || a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) {
        if (a[i].pos !== b[i].pos || a[i].page !== b[i].page || !!a[i].isBreak !== !!b[i].isBreak) {
            return false;
        }
    }
    return true;
}

// ── Page count → status bar ───────────────────────────────────────────────
let lastPageCount = 1;
function updatePageCount(n) {
    if (n === lastPageCount) return;
    lastPageCount = n;
    const el = document.getElementById('page-footer-stats');
    if (el) {
        const text = el.textContent;
        // Preserve any trailing "· X min read" suffix.
        const suffix = text.includes('·') ? text.slice(text.indexOf('·')) : '';
        el.textContent = `Page 1 of ${n}${suffix ? ' ' + suffix : ''}`;
    }
    window.__velaPageCount = n;
}

// ── Page-sheet background overlay ─────────────────────────────────────────
/**
 * Render N white A4 sheet backgrounds behind the editor. The overlay is a
 * sibling of `.editor` inside `.page`, positioned absolutely with negative
 * z-index so content paints on top. Header/footer text (with {PAGE} tokens
 * resolved) is rendered per sheet.
 */
function renderPageSheets(editorDom, pageCount, settings, geom) {
    const page = editorDom.closest('.page');
    if (!page) return;
    let overlay = page.querySelector('.vela-page-sheets');
    if (!overlay) {
        overlay = document.createElement('div');
        overlay.className = 'vela-page-sheets';
        overlay.setAttribute('aria-hidden', 'true');
        page.insertBefore(overlay, page.firstChild);
    }
    const dims = PAPER_SIZES_MM[settings.size] || PAPER_SIZES_MM.A4;
    const portrait = settings.orientation !== 'landscape';
    const pageWmm = portrait ? dims.w : dims.h;
    const pageWidthPx = Math.round(pageWmm * PX_PER_MM);
    const pageHeightPx = geom.pageHeightPx;
    const pageGapPx = geom.pageGapPx;
    const mLeftPx = Math.round((settings.marginLeft ?? 1) * PX_PER_IN);
    const mRightPx = Math.round((settings.marginRight ?? 1) * PX_PER_IN);
    const mTopPx = geom.m.top;
    const mBottomPx = geom.m.bottom;

    let html = '';
    for (let i = 0; i < pageCount; i++) {
        const pageNum = i + (settings.pageNumberStartAt ?? 1);
        const top = i * (pageHeightPx + pageGapPx);
        const headerResolved = resolveTokens(settings.headerText || '', pageNum, pageCount, settings.pageNumberFormat);
        const footerResolved = resolveTokens(settings.footerText || '', pageNum, pageCount, settings.pageNumberFormat);
        // Skip header on first page if differentFirstPage is set and there's no
        // first-page-specific text (v1: just hide it).
        const showHeader = !(i === 0 && settings.differentFirstPage);
        const headerHtml = (showHeader && headerResolved)
            ? `<div class="vela-page-header" style="top:${top + mTopPx * 0.3}px;left:${mLeftPx}px;right:${mRightPx}px">${escapeHtml(headerResolved)}</div>`
            : '';
        const footerHtml = (showHeader && footerResolved)
            ? `<div class="vela-page-footer" style="top:${top + pageHeightPx - mBottomPx * 0.7}px;left:${mLeftPx}px;right:${mRightPx}px">${escapeHtml(footerResolved)}</div>`
            : '';
        html += `<div class="vela-page-sheet" style="top:${top}px;width:${pageWidthPx}px;height:${pageHeightPx}px"></div>${headerHtml}${footerHtml}`;
    }
    overlay.innerHTML = html;
    // Tell CSS how tall the overlay should be (drives .page min-height).
    overlay.style.height = `${pageCount * pageHeightPx + (pageCount - 1) * pageGapPx}px`;
}

// ── Token resolution ({PAGE}, {NUMPAGES}, Roman/alpha formats) ─────────────
function resolveTokens(text, pageNum, pageCount, format) {
    return text
        .replace(/\{NUMPAGES\}/g, String(pageCount))
        .replace(/\{PAGE\}/g, formatPageNumber(pageNum, format));
}

function formatPageNumber(n, format) {
    if (!n || n < 1) return String(n || 0);
    switch (format) {
        case 'upperRoman':  return toRoman(n);
        case 'lowerRoman':  return toRoman(n).toLowerCase();
        case 'upperAlpha':  return toAlpha(n);
        case 'lowerAlpha':  return toAlpha(n).toLowerCase();
        case 'decimal':
        default:            return String(n);
    }
}

function toRoman(num) {
    const map = [
        ['M', 1000], ['CM', 900], ['D', 500], ['CD', 400],
        ['C', 100], ['XC', 90], ['L', 50], ['XL', 40],
        ['X', 10], ['IX', 9], ['V', 5], ['IV', 4], ['I', 1],
    ];
    let out = '';
    for (const [sym, val] of map) {
        while (num >= val) { out += sym; num -= val; }
    }
    return out;
}

function toAlpha(n) {
    let s = '';
    while (n > 0) {
        const rem = (n - 1) % 26;
        s = String.fromCharCode(65 + rem) + s;
        n = Math.floor((n - 1) / 26);
    }
    return s;
}

function escapeHtml(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

// ── TipTap Extension wrapper ──────────────────────────────────────────────
export const Pagination = Extension.create({
    name: 'pagination',
    addOptions() {
        return {
            enabled: false,
            size: 'A4',
            orientation: 'portrait',
            marginTop: 1, marginRight: 1, marginBottom: 1, marginLeft: 1,
            pageGap: 24,
        };
    },
    addProseMirrorPlugins() {
        if (!this.options.enabled) return [];
        return [buildPaginationPlugin(this.options)];
    },
});

// ── Controller: mount + lifecycle ─────────────────────────────────────────
/**
 * Mount the pagination controller on an editor. Returns an API to toggle,
 * update settings, and force a recompute.
 *
 * @param {Editor} editor — TipTap editor instance
 * @returns {{
 *   enable: (settings?: object) => void,
 *   disable: () => void,
 *   setSettings: (settings: object) => void,
 *   recompute: () => void,
 *   isEnabled: () => boolean,
 * }}
 */
export function mountPagination(editor) {
    let enabled = false;
    let settings = {
        size: 'A4',
        orientation: 'portrait',
        marginTop: 1, marginRight: 1, marginBottom: 1, marginLeft: 1,
        pageGap: 24,
    };
    let debounceTimer = null;

    function scheduleRecompute() {
        if (!enabled) return;
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            if (editor?.view) recomputePagination(editor.view, settings);
        }, 50);
    }

    function applyContainerClass(on) {
        const canvas = document.getElementById('canvas');
        const page = document.getElementById('page');
        const ruler = document.getElementById('ruler');
        if (canvas) canvas.classList.toggle('is-paging', on);
        if (page) page.classList.toggle('is-paging', on);
        // The legacy ruler is tied to the single-page 8.5in model; hide it
        // in paging mode to avoid a misleading width bar.
        if (ruler) ruler.classList.toggle('is-paging-hidden', on);
    }

    function applyGeometryCss() {
        const dims = PAPER_SIZES_MM[settings.size] || PAPER_SIZES_MM.A4;
        const portrait = settings.orientation !== 'landscape';
        const pageWmm = portrait ? dims.w : dims.h;
        const pageHmm = portrait ? dims.h : dims.w;
        const root = document.documentElement;
        root.style.setProperty('--vela-page-w', `${pageWmm}mm`);
        root.style.setProperty('--vela-page-h', `${pageHmm}mm`);
        root.style.setProperty('--vela-page-mt', `${settings.marginTop ?? 1}in`);
        root.style.setProperty('--vela-page-mr', `${settings.marginRight ?? 1}in`);
        root.style.setProperty('--vela-page-mb', `${settings.marginBottom ?? 1}in`);
        root.style.setProperty('--vela-page-ml', `${settings.marginLeft ?? 1}in`);
        root.style.setProperty('--vela-page-gap', `${settings.pageGap ?? 24}px`);
    }

    function enable(overrides) {
        if (overrides) settings = { ...settings, ...overrides };
        if (enabled) {
            // Already on — just refresh geometry & recompute.
            applyGeometryCss();
            scheduleRecompute();
            return;
        }
        enabled = true;
        applyGeometryCss();
        applyContainerClass(true);
        // Register the PM plugin directly (no extension rebuild → no focus loss).
        // We pass the live `settings` object so the plugin always sees current
        // geometry without re-registration.
        if (editor?.view) {
            const existing = paginationKey.getState(editor.view.state);
            if (!existing) {
                editor.registerPlugin(buildPaginationPlugin(settings));
            }
        }
        scheduleRecompute();
    }

    function disable() {
        if (!enabled) return;
        enabled = false;
        applyContainerClass(false);
        // Tear down decorations by removing the plugin.
        if (editor?.unregisterPlugin) {
            editor.unregisterPlugin(paginationKey);
        }
        // Remove overlay + reset page count display.
        const page = document.getElementById('page');
        page?.querySelector('.vela-page-sheets')?.remove();
        const statsEl = document.getElementById('page-footer-stats');
        if (statsEl) statsEl.textContent = 'Page 1 of 1';
        lastPageCount = 1;
    }

    function setSettings(overrides) {
        settings = { ...settings, ...overrides };
        if (enabled) {
            applyGeometryCss();
            scheduleRecompute();
        }
    }

    // Recompute triggers.
    if (editor) {
        editor.on('update', scheduleRecompute);
        editor.on('selectionUpdate', scheduleRecompute);
    }
    window.addEventListener('resize', scheduleRecompute);
    if (document.fonts?.ready) document.fonts.ready.then(scheduleRecompute);

    return {
        enable,
        disable,
        setSettings,
        recompute: scheduleRecompute,
        isEnabled: () => enabled,
        getSettings: () => settings,
    };
}

export default Pagination;
