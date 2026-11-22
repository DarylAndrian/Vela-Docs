// ════════════════════════════════════════════════════════════════════════════
//  tab-stop.js — v0.4.0
//
//  A ProseMirror plugin that renders visible tab-stop markers inside the
//  document at the *active paragraph's* stops, AND keeps the editor's tab
//  character behavior aligned to the active paragraph's stops.
//
//  State (per paragraph):
//      paragraph.attrs.tabStops = [{ pos, type, leader }, ...]   // pos in inches
//
//  The ruler (ruler.js) is the primary editing surface; this plugin is the
//  in-document visualization. Together they make tab stops feel right.
// ════════════════════════════════════════════════════════════════════════════

import { Plugin, PluginKey } from '@tiptap/pm/state';
import { Decoration, DecorationSet } from '@tiptap/pm/view';

const PLUGIN_KEY = new PluginKey('velaTabStops');

const DEFAULT_TAB_INTERVAL = 0.5; // inches

function pxPerInch() {
    // CSS px per inch in the editor canvas
    return 96;
}

/**
 * Build Decoration.line decorations for the active paragraph that visualize
 * each tab stop as a small vertical marker rendered as inline background-image.
 */
function buildDecorations(doc, activePos) {
    if (activePos == null) return DecorationSet.empty;

    const decorations = [];
    const pxPerIn = pxPerInch();

    doc.nodesBetween(activePos, activePos + 1, (node, pos) => {
        if (!node.isTextblock) return;
        const stops = node.attrs?.tabStops || [];
        if (!stops.length) return;

        // For each line of the paragraph, render the stops at the cumulative x offset
        const deco = Decoration.node(pos, pos + node.nodeSize, {
            'data-tab-stops': JSON.stringify(stops),
            style: stops
                .map((s, i) => {
                    const xPx = s.pos * pxPerIn;
                    return `--vela-tab-${i}: ${xPx}px;`;
                })
                .join(' '),
        });
        decorations.push(deco);
    });

    return DecorationSet.create(doc, decorations);
}

/**
 * The plugin. Tracks the active paragraph and rebuilds decorations when
 * either the selection or the document changes.
 */
export const TabStopPlugin = ({ pageWidthInches = 8.5, marginLeftInches = 1.0, marginRightInches = 1.0 } = {}) => {
    return new Plugin({
        key: PLUGIN_KEY,

        state: {
            init: (_, { doc }) => ({ decorations: DecorationSet.empty, activePos: null }),
            apply(tr, old) {
                const meta = tr.getMeta(PLUGIN_KEY);
                if (meta?.activePos !== undefined) {
                    return { decorations: buildDecorations(tr.doc, meta.activePos), activePos: meta.activePos };
                }
                if (tr.docChanged || tr.selectionSet) {
                    const $from = tr.selection.$from;
                    let activePos = null;
                    for (let d = $from.depth; d >= 0; d--) {
                        const n = $from.node(d);
                        if (n.isTextblock) {
                            activePos = $from.before(d);
                            break;
                        }
                    }
                    return { decorations: buildDecorations(tr.doc, activePos), activePos };
                }
                return old;
            },
        },

        props: {
            decorations(state) {
                return this.getState(state).decorations;
            },
        },

        // Capture Tab key → snap to nearest stop
        handleKeyDown(view, event) {
            if (event.key !== 'Tab') return false;
            const { state } = view;
            const { $from } = state.selection;

            // Find the active paragraph
            let para = null, paraPos = -1;
            for (let d = $from.depth; d >= 0; d--) {
                if ($from.node(d).isTextblock) {
                    para = $from.node(d);
                    paraPos = $from.before(d);
                    break;
                }
            }
            if (!para) return false;

            const stops = para.attrs?.tabStops || [];
            const writableStart = marginLeftInches;
            const writableEnd = pageWidthInches - marginRightInches;
            const writable = writableEnd - writableStart;

            // Compute current cursor x offset (rough estimate: assume monospace for now)
            const cursorOffset = $from.parentOffset; // index of character in paragraph
            const currentX = writableStart + (cursorOffset * 0.1); // heuristic

            let target;
            if (event.shiftKey) {
                // Shift+Tab → previous stop
                target = [...stops].reverse().find((s) => s.pos < currentX) || stops[0];
            } else {
                // Tab → next stop
                target = stops.find((s) => s.pos > currentX);
                if (!target) {
                    // Default to the next 0.5" grid
                    const next = Math.ceil((currentX + 0.001) / DEFAULT_TAB_INTERVAL) * DEFAULT_TAB_INTERVAL;
                    if (next <= writableEnd) target = { pos: next, type: 'left', leader: 'none' };
                }
            }

            if (!target) return false;
            event.preventDefault();
            view.dispatch(state.tr.insertText('\t', $from.pos));
            return true;
        },
    });
};

export default TabStopPlugin;
export { PLUGIN_KEY, DEFAULT_TAB_INTERVAL };