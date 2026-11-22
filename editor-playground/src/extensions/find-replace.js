// ════════════════════════════════════════════════════════════════════════════
//  find-replace.js — Search & replace across the whole document
//
//  Matches are found on a flattened view of the document text so a hit that
//  spans a formatting boundary ("bold|normal") is still found — Word does this
//  and per-text-node search would miss it. Block boundaries become "\n" so a
//  match never runs across paragraphs.
//
//  Each match is stored as a list of {from,to} segments, one per text node it
//  covers, because decorations and replacements have to be applied per node.
// ════════════════════════════════════════════════════════════════════════════
import { Extension } from '@tiptap/core';
import { Plugin, PluginKey, TextSelection } from '@tiptap/pm/state';
import { Decoration, DecorationSet } from '@tiptap/pm/view';

export const findReplaceKey = new PluginKey('vela-find-replace');

const EMPTY = { term: '', options: { caseSensitive: false, wholeWord: false }, matches: [], activeIndex: -1 };

function escapeRegExp(s) {
    return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

/// Flatten the doc into one string plus the text-node spans that produced it.
function flatten(doc) {
    let full = '';
    const spans = [];
    doc.descendants((node, pos) => {
        if (node.isText) {
            spans.push({ from: pos, start: full.length, length: node.text.length });
            full += node.text;
        } else if (node.isBlock) {
            full += '\n';
        }
        return true;
    });
    return { full, spans };
}

/// Map a [start,end) range in the flattened string back to doc segments.
function segmentsFor(spans, start, end) {
    const segments = [];
    for (const span of spans) {
        const spanEnd = span.start + span.length;
        if (spanEnd <= start) continue;
        if (span.start >= end) break;
        const from = span.from + Math.max(0, start - span.start);
        const to   = span.from + Math.min(span.length, end - span.start);
        if (to > from) segments.push({ from, to });
    }
    return segments;
}

function findAll(doc, term, options) {
    if (!term) return [];
    const { full, spans } = flatten(doc);
    const flags = options.caseSensitive ? 'g' : 'gi';
    const pattern = options.wholeWord
        ? new RegExp(`(?<![\\p{L}\\p{N}_])${escapeRegExp(term)}(?![\\p{L}\\p{N}_])`, flags + 'u')
        : new RegExp(escapeRegExp(term), flags);

    const matches = [];
    let m;
    // A pathological term could otherwise loop forever on a zero-length match.
    let guard = 0;
    while ((m = pattern.exec(full)) !== null && guard++ < 100000) {
        if (m[0].length === 0) { pattern.lastIndex++; continue; }
        matches.push(segmentsFor(spans, m.index, m.index + m[0].length));
    }
    return matches;
}

function decorationsFor(state) {
    const { matches, activeIndex } = findReplaceKey.getState(state) || EMPTY;
    if (!matches.length) return DecorationSet.empty;

    const decos = [];
    matches.forEach((segments, i) => {
        const isActive = i === activeIndex;
        for (const seg of segments) {
            decos.push(Decoration.inline(seg.from, seg.to, {
                class: isActive ? 'vela-find-hit vela-find-hit--active' : 'vela-find-hit',
            }));
        }
    });
    return DecorationSet.create(state.doc, decos);
}

/// Index of the first match at or after the cursor, so Ctrl+F from mid-document
/// continues forward instead of jumping back to the top.
function nearestIndex(matches, pos) {
    if (!matches.length) return -1;
    for (let i = 0; i < matches.length; ++i) {
        if (matches[i][0].from >= pos) return i;
    }
    return 0;
}

const FindReplace = Extension.create({
    name: 'velaFindReplace',

    addOptions() {
        return { caseSensitive: false, wholeWord: false };
    },

    addCommands() {
        return {
            setSearch: (term, options = {}) => ({ tr, state, dispatch }) => {
                const merged = { ...this.options, ...options };
                const matches = findAll(state.doc, term, merged);
                const activeIndex = term ? nearestIndex(matches, tr.selection.from) : -1;
                if (dispatch) {
                    tr.setMeta(findReplaceKey, { term, options: merged, matches, activeIndex });
                }
                return true;
            },

            clearSearch: () => ({ tr, dispatch }) => {
                if (dispatch) tr.setMeta(findReplaceKey, { ...EMPTY });
                return true;
            },

            /// direction: 1 = next, -1 = previous. Wraps around.
            findStep: (direction = 1) => ({ tr, state, dispatch }) => {
                const cur = findReplaceKey.getState(state) || EMPTY;
                if (!cur.matches.length) return false;
                const n = cur.matches.length;
                const activeIndex = cur.activeIndex < 0
                    ? nearestIndex(cur.matches, tr.selection.from)
                    : (((cur.activeIndex + direction) % n) + n) % n;
                if (dispatch) {
                    tr.setMeta(findReplaceKey, { ...cur, activeIndex });
                    const seg = cur.matches[activeIndex][0];
                    // Move the caret to the hit so the view scrolls it into view.
                    tr.setSelection(TextSelection.near(tr.doc.resolve(seg.from)));
                }
                return true;
            },

            /// Replace the active match, then step to the next one.
            replaceCurrent: (replacement = '') => ({ tr, state, dispatch }) => {
                const cur = findReplaceKey.getState(state) || EMPTY;
                if (cur.activeIndex < 0 || !cur.matches[cur.activeIndex]) return false;
                const segments = cur.matches[cur.activeIndex];

                if (dispatch) {
                    // Descending order keeps the earlier offsets valid.
                    for (let i = segments.length - 1; i >= 0; --i) {
                        const seg = segments[i];
                        tr.insertText(i === 0 ? replacement : '', seg.from, seg.to);
                    }
                    tr.setMeta(findReplaceKey, {
                        ...cur,
                        term: cur.term,
                        matches: findAll(tr.doc, cur.term, cur.options),
                        activeIndex: -1,
                    });
                }
                return true;
            },

            replaceAll: (replacement = '') => ({ tr, state, dispatch }) => {
                const cur = findReplaceKey.getState(state) || EMPTY;
                if (!cur.matches.length) return false;

                if (dispatch) {
                    for (let m = cur.matches.length - 1; m >= 0; --m) {
                        const segments = cur.matches[m];
                        for (let i = segments.length - 1; i >= 0; --i) {
                            const seg = segments[i];
                            tr.insertText(i === 0 ? replacement : '', seg.from, seg.to);
                        }
                    }
                    tr.setMeta(findReplaceKey, { ...cur, matches: [], activeIndex: -1 });
                }
                return true;
            },
        };
    },

    addProseMirrorPlugins() {
        return [
            new Plugin({
                key: findReplaceKey,
                state: {
                    init: () => ({ ...EMPTY }),
                    apply(tr, value) {
                        const meta = tr.getMeta(findReplaceKey);
                        if (meta) return meta;
                        // Keep decorations glued to their text when the doc changes.
                        if (tr.docChanged && value.matches.length) {
                            return {
                                ...value,
                                matches: findAll(tr.doc, value.term, value.options),
                                activeIndex: -1,
                            };
                        }
                        return value;
                    },
                },
                props: {
                    decorations: (state) => decorationsFor(state),
                    handleKeyDown: (view, event) => {
                        const mod = event.ctrlKey || event.metaKey;
                        if (!mod) return false;
                        const k = event.key.toLowerCase();
                        // The editor swallows these before main.js's document-level
                        // handler ever sees them, so they are re-broadcast as events
                        // the find bar listens for.
                        if (k === 'f' || k === 'h' || k === 'g') {
                            event.preventDefault();
                            // Without this the same keypress also reaches the
                            // document-level listener in main.js and every step
                            // would advance twice.
                            event.stopPropagation();
                            view.dom.dispatchEvent(new CustomEvent('vela:findkey', {
                                bubbles: true,
                                detail: { key: k, backwards: event.shiftKey },
                            }));
                            return true;
                        }
                        return false;
                    },
                },
            }),
        ];
    },
});

export default FindReplace;

/// Read-only view of the current search, for the UI's "3 of 17" counter.
export function getSearchState(editor) {
    const state = findReplaceKey.getState(editor.state) || EMPTY;
    return {
        term: state.term,
        total: state.matches.length,
        activeIndex: state.activeIndex,
    };
}
