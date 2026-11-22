// outline.js — v0.3.3 (IMP-01)
//
// TipTap node "outlineHeading" that exposes its bounds via the
// `editor.storage.outline` field, populated by a small ProseMirror plugin.
// The sidebar binds to that data and re-renders on every update.

import { Plugin, PluginKey } from '@tiptap/pm/state';

const key = new PluginKey('velaOutline');

function readOutline(doc) {
  const out = [];
  doc.descendants((node, pos) => {
    if (node.type.name === 'heading') {
      out.push({
        level:  node.attrs.level,
        text:   node.textContent,
        anchor: pos,
      });
    }
  });
  return out;
}

export const OutlinePlugin = new Plugin({
  key,
  state: {
    init(_, state) { return readOutline(state.doc); },
    apply(tr, prev, _, newState) {
      if (tr.docChanged) return readOutline(newState.doc);
      return prev;
    },
  },
});

export function getOutline(view) { return OutlinePlugin.getState(view.state) || []; }

if (typeof window !== 'undefined') window.__vela_outline = { getOutline };
