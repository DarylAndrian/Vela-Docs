// ════════════════════════════════════════════════════════════════════════════
//  ruler.js — v0.4.0
//
//  Horizontal ruler above the editor canvas. Renders:
//    • tick marks at every 0.05"  (minor) and 0.25" (major)
//    • page-margin triangles (drag to adjust left/right margins)
//    • tab-stop markers (click to add, drag to move, double-click to edit,
//      right-click for context menu)
//
//  The ruler is a passive view: it calls onChange callbacks whenever the user
//  adds, moves, removes, or edits a tab stop. The owning code (e.g. the
//  paragraph attribute system) is responsible for persisting the state.
//
//  Tab stop shape:
//      { pos: number /* inches */, type: 'left'|'center'|'right'|'decimal'|'bar',
//        leader: 'none'|'dot'|'hyphen'|'underscore' }
//
//  Usage:
//      import { mountRuler } from './ruler.js';
//      mountRuler(document.getElementById('ruler'), {
//          pageWidthInches: 8.5,
//          marginLeftInches: 1.0,
//          marginRightInches: 1.0,
//          tabStops: [],
//          onTabStopChange: (stops) => editor.commands.setTabStops(stops),
//          onMarginChange: (m) => editor.commands.setMargins(m),
//      });
// ════════════════════════════════════════════════════════════════════════════

const MINOR_TICK = 0.05;   // inches
const MAJOR_TICK = 0.25;
const NUMBERED_TICK = 0.5; // inches
const PX_PER_INCH = 96;    // CSS reference; scaled by actual page width

const TAB_GLYPHS = {
    left:    '▌',
    center:  '⊤',
    right:   '▐',
    decimal: '⊥',
    bar:     '│',
};

export function mountRuler(container, opts = {}) {
    const config = {
        pageWidthInches:  8.5,
        marginLeftInches: 1.0,
        marginRightInches: 1.0,
        tabStops:         [],
        units:            'inches', // 'inches' | 'cm'
        onTabStopChange:  () => {},
        onMarginChange:   () => {},
        onTabStopEdit:    () => {},  // fires when user double-clicks a stop
        ...opts,
    };

    let active = true;
    let drag = null; // { kind: 'tab'|'margin', index?, side?, startX, originalPos }

    const state = {
        pageWidthInches:  config.pageWidthInches,
        marginLeftInches: config.marginLeftInches,
        marginRightInches: config.marginRightInches,
        tabStops:         config.tabStops.slice(),
    };

    function render() {
        if (!container || !active) return;
        const writableWidth = state.pageWidthInches - state.marginLeftInches - state.marginRightInches;
        if (writableWidth <= 0) return;
        const unitPx = writableWidth * PX_PER_INCH;
        const pagePx = state.pageWidthInches * PX_PER_INCH;
        const marginLeftPx = state.marginLeftInches * PX_PER_INCH;
        const marginRightPx = state.marginRightInches * PX_PER_INCH;

        container.innerHTML = `
            <svg class="ruler-svg" viewBox="0 0 ${pagePx} 24" preserveAspectRatio="none"
                 xmlns="http://www.w3.org/2000/svg">
                <!-- Page outline -->
                <rect x="0" y="0" width="${pagePx}" height="24" class="ruler-page"/>

                <!-- Tick marks (only inside the writable area) -->
                <g class="ruler-ticks" transform="translate(${marginLeftPx}, 0)">
                    ${renderTicks(writableWidth)}
                </g>

                <!-- Margin handles -->
                <polygon class="ruler-margin ruler-margin-left"
                         points="0,4 ${marginLeftPx - 6},4 ${marginLeftPx - 6},14 ${marginLeftPx},18 ${marginLeftPx - 6},22 ${marginLeftPx - 6},14 0,14"/>
                <polygon class="ruler-margin ruler-margin-right"
                         points="${pagePx},4 ${pagePx - marginRightPx + 6},4 ${pagePx - marginRightPx + 6},14 ${pagePx - marginRightPx},18 ${pagePx - marginRightPx + 6},22 ${pagePx - marginRightPx + 6},14 ${pagePx},14"/>
            </svg>

            <div class="ruler-tabs" style="position:absolute; left:${marginLeftPx}px; right:${marginRightPx}px; top:0; bottom:0;">
                ${state.tabStops.map((s, i) => renderTabStop(s, i)).join('')}
            </div>
        `;
        attachHandlers();
    }

    function renderTicks(widthInches) {
        const parts = [];
        const tickCount = Math.round(widthInches / MINOR_TICK);
        for (let i = 0; i <= tickCount; i++) {
            const x = i * MINOR_TICK * PX_PER_INCH;
            const inch = i * MINOR_TICK;
            const isMajor = Math.abs(inch % MAJOR_TICK) < 0.001;
            const isNumbered = Math.abs(inch % NUMBERED_TICK) < 0.001;
            const tickHeight = isMajor ? 10 : (isNumbered ? 8 : 5);
            parts.push(
                `<line x1="${x}" y1="${24 - tickHeight}" x2="${x}" y2="24" class="ruler-tick ${isMajor ? 'major' : 'minor'}"/>`,
            );
            if (isNumbered && inch > 0) {
                parts.push(
                    `<text x="${x + 2}" y="9" class="ruler-label">${inch.toFixed(inch % 1 === 0 ? 0 : 1)}"</text>`,
                );
            }
        }
        return parts.join('');
    }

    function renderTabStop(stop, index) {
        const x = stop.pos * PX_PER_INCH;
        const glyph = TAB_GLYPHS[stop.type] || TAB_GLYPHS.left;
        return `
            <div class="ruler-tab ${stop.type}${stop.leader !== 'none' ? ' has-leader leader-' + stop.leader : ''}"
                 data-index="${index}"
                 style="position:absolute; left:${x - 6}px; top:0; width:12px; height:24px;">
                <span class="ruler-tab-glyph">${glyph}</span>
                ${stop.leader !== 'none' ? `<span class="ruler-tab-leader">${leaderGlyph(stop.leader)}</span>` : ''}
            </div>
        `;
    }

    function leaderGlyph(leader) {
        switch (leader) {
            case 'dot':       return '· · ·';
            case 'hyphen':    return '- - -';
            case 'underscore': return '_ _ _';
            default: return '';
        }
    }

    // ── Interaction ────────────────────────────────────────────

    function attachHandlers() {
        const svg = container.querySelector('.ruler-svg');
        const tabs = container.querySelector('.ruler-tabs');

        // Click on writable area → add a left tab stop
        tabs.addEventListener('mousedown', (e) => {
            if (e.target.closest('.ruler-tab')) return; // handled below
            if (e.button !== 0) return;
            const rect = tabs.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const inch = clamp(x / PX_PER_INCH, 0.05, state.pageWidthInches - state.marginLeftInches - state.marginRightInches - 0.05);
            state.tabStops.push({ pos: snap(inch), type: 'left', leader: 'none' });
            sortStops();
            config.onTabStopChange(state.tabStops.slice());
            render();
        });

        // Margin handles
        container.querySelectorAll('.ruler-margin').forEach((el) => {
            el.addEventListener('mousedown', (e) => {
                e.preventDefault();
                const side = el.classList.contains('ruler-margin-left') ? 'left' : 'right';
                drag = {
                    kind: 'margin',
                    side,
                    startX: e.clientX,
                    originalLeft:  state.marginLeftInches,
                    originalRight: state.marginRightInches,
                };
                window.addEventListener('mousemove', onMarginDrag);
                window.addEventListener('mouseup', onDragEnd);
            });
        });

        // Tab stops
        container.querySelectorAll('.ruler-tab').forEach((el) => {
            const index = parseInt(el.dataset.index, 10);

            el.addEventListener('mousedown', (e) => {
                if (e.button !== 2) { // left button only — right button handled by contextmenu
                    e.preventDefault();
                    e.stopPropagation();
                    drag = {
                        kind: 'tab',
                        index,
                        startX: e.clientX,
                        originalPos: state.tabStops[index].pos,
                    };
                    window.addEventListener('mousemove', onTabDrag);
                    window.addEventListener('mouseup', onDragEnd);
                }
            });

            el.addEventListener('dblclick', () => {
                config.onTabStopEdit(state.tabStops[index], (updated) => {
                    if (updated) {
                        state.tabStops[index] = updated;
                        sortStops();
                        config.onTabStopChange(state.tabStops.slice());
                        render();
                    }
                });
            });

            el.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                showContextMenu(e.clientX, e.clientY, index);
            });
        });
    }

    function onTabDrag(e) {
        if (!drag) return;
        const dx = e.clientX - drag.startX;
        const dxInches = dx / PX_PER_INCH;
        const newPos = snap(drag.originalPos + dxInches);
        const writable = state.pageWidthInches - state.marginLeftInches - state.marginRightInches;
        state.tabStops[drag.index] = {
            ...state.tabStops[drag.index],
            pos: clamp(newPos, 0.05, writable - 0.05),
        };
        render();
    }

    function onMarginDrag(e) {
        if (!drag) return;
        const dx = e.clientX - drag.startX;
        const dxInches = dx / PX_PER_INCH;
        const max = state.pageWidthInches - 0.5; // minimum 0.5" writable
        if (drag.side === 'left') {
            state.marginLeftInches = clamp(drag.originalLeft + dxInches, 0.25, max - 0.25);
        } else {
            state.marginRightInches = clamp(drag.originalRight - dxInches, 0.25, max - 0.25);
        }
        render();
    }

    function onDragEnd() {
        if (!drag) return;
        if (drag.kind === 'tab') {
            sortStops();
            config.onTabStopChange(state.tabStops.slice());
        } else if (drag.kind === 'margin') {
            config.onMarginChange({
                left:  state.marginLeftInches,
                right: state.marginRightInches,
            });
        }
        drag = null;
        window.removeEventListener('mousemove', onTabDrag);
        window.removeEventListener('mousemove', onMarginDrag);
        window.removeEventListener('mouseup', onDragEnd);
        render();
    }

    function showContextMenu(x, y, index) {
        // Remove any existing menu
        document.querySelectorAll('.ruler-context-menu').forEach((m) => m.remove());

        const stop = state.tabStops[index];
        const menu = document.createElement('div');
        menu.className = 'ruler-context-menu';
        menu.style.left = `${x}px`;
        menu.style.top = `${y}px`;
        menu.innerHTML = `
            <button data-action="left">Left <kbd>L</kbd></button>
            <button data-action="center">Center <kbd>C</kbd></button>
            <button data-action="right">Right <kbd>R</kbd></button>
            <button data-action="decimal">Decimal <kbd>D</kbd></button>
            <button data-action="bar">Bar</button>
            <hr/>
            <button data-action="leader-none">No leader</button>
            <button data-action="leader-dot">Dot leader</button>
            <button data-action="leader-hyphen">Dash leader</button>
            <button data-action="leader-underscore">Underscore leader</button>
            <hr/>
            <button data-action="clear">Clear this stop</button>
            <button data-action="clear-all">Clear all</button>
        `;
        document.body.appendChild(menu);

        menu.addEventListener('click', (e) => {
            const action = e.target.closest('[data-action]')?.dataset.action;
            if (!action) return;
            menu.remove();
            if (action === 'clear') {
                state.tabStops.splice(index, 1);
            } else if (action === 'clear-all') {
                state.tabStops.length = 0;
            } else if (action.startsWith('leader-')) {
                state.tabStops[index].leader = action.slice(7);
            } else {
                state.tabStops[index].type = action;
            }
            sortStops();
            config.onTabStopChange(state.tabStops.slice());
            render();
        });

        // Close on outside click
        setTimeout(() => {
            const close = (ev) => {
                if (!menu.contains(ev.target)) {
                    menu.remove();
                    document.removeEventListener('click', close);
                }
            };
            document.addEventListener('click', close);
        }, 0);
    }

    // ── Public API ─────────────────────────────────────────────

    return {
        render,
        setTabStops(stops) {
            state.tabStops = (stops || []).slice();
            render();
        },
        getTabStops() {
            return state.tabStops.slice();
        },
        setMargins({ left, right }) {
            if (typeof left === 'number')  state.marginLeftInches  = left;
            if (typeof right === 'number') state.marginRightInches = right;
            render();
        },
        setPageWidth(inches) {
            state.pageWidthInches = inches;
            render();
        },
        destroy() {
            active = false;
            container.innerHTML = '';
        },
    };
}

// ── helpers ───────────────────────────────────────────────────────────────

function clamp(v, min, max) {
    return Math.max(min, Math.min(max, v));
}

function snap(inches) {
    // Snap to 0.05" grid
    return Math.round(inches / 0.05) * 0.05;
}

function sortStops() {
    state.tabStops.sort((a, b) => a.pos - b.pos);
}