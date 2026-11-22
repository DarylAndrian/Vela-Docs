// find-replace.js — v0.3.2 (MAJOR-03)
//
// Find & Replace over the document, using ProseMirror `Decoration`. Every
// match becomes an inline `widget` decoration; the panel calls into this
// module through `window.__vela_find`.

import { Decoration, DecorationSet } from '@tiptap/pm/view';
import { Plugin, PluginKey } from '@tiptap/pm/state';

const key = new PluginKey('velaFindReplace');

class FindState {
  constructor(query, replaceWith, opts) {
    this.query      = query;
    this.replaceWith= replaceWith;
    this.opts       = opts || {}; // {regex, caseSensitive, wholeWord, inSelection}
    this.decorations= DecorationSet.empty;
    this.matches    = [];
  }
}

function buildDecorations(doc, state) {
  if (!state.query) return DecorationSet.empty;
  const decorator = [];
  const re = buildRegex(state.query, state.opts);
  if (!re) return DecorationSet.empty;

  doc.descendants((node, pos) => {
    if (!node.isText || !node.text) return;
    const text = node.text;
    let m;
    while ((m = re.exec(text))) {
      const from = pos + m.index;
      const to   = from + m[0].length;
      decorator.push(Decoration.inline(from, to, { class: 'vela-find-match' }));
      state.matches.push({ from, to });
      if (m[0].length === 0) re.lastIndex++;
    }
  });
  return DecorationSet.create(doc, decorator);
}

function buildRegex(query, opts) {
  if (!query) return null;
  try {
    if (opts.regex) return new RegExp(query, opts.caseSensitive ? 'g' : 'gi');
    const escaped = query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    const body = opts.wholeWord ? `\\b${escaped}\\b` : escaped;
    return new RegExp(body, opts.caseSensitive ? 'g' : 'gi');
  } catch (_) { return null; }
}

export const FindReplacePlugin = new Plugin({
  key,
  state: {
    init() { return new FindState('', '', {}); },
    apply(tr, prev, oldState, newState) {
      const meta = tr.getMeta(key);
      if (meta) {
        const next = new FindState(meta.query, meta.replaceWith || '', meta.opts || {});
        next.decorations = buildDecorations(newState.doc, next);
        return next;
      }
      return prev;
    },
  },
  props: {
    decorations(state) { return this.getState(state).decorations; },
  },
});

export function dispatchFind(view, query, opts, replaceWith) {
  view.dispatch(view.state.tr.setMeta(key, { query, replaceWith, opts: opts || {} }));
}

export function replaceNext(view) {
  const plugin = FindReplacePlugin.getState(view.state);
  if (!plugin || !plugin.matches.length) return false;
  const { from, to } = plugin.matches[0];
  const replaceWith = plugin.replaceWith ?? '';
  view.dispatch(view.state.tr.insertText(replaceWith, from, to));
  return true;
}

export function replaceAll(view) {
  let count = 0;
  while (replaceNext(view)) count++;
  return count;
}

if (typeof window !== 'undefined') {
  window.__vela_find = { dispatchFind, replaceNext, replaceAll };
}
