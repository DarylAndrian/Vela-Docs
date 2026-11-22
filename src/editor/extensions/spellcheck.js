// spellcheck.js — v0.3.2 (MAJOR-07)
//
// Connects the C++ Hunspell checker (svc *SpellChecker*) to the TipTap
// editor. We use ProseMirror `Decoration` to add a wavy underline under
// misspelled words; suggestions come from a simple async round-trip
// through the EditorBridge.
//
// Grammar checking shares the same channel but renders with a blue
// underline (`vela-grammar-issue`); the C++ side decides which is which
// based on the dictionary's `*_grammar.aff` companion file (added in
// v0.3.4 when we ship more grammars).

import { Plugin, PluginKey } from '@tiptap/pm/state';
import { Decoration, DecorationSet } from '@tiptap/pm/view';

const key = new PluginKey('velaSpellcheck');

function collectWords(doc) {
  const words = [];
  doc.descendants((node, pos) => {
    if (!node.isText || !node.text) return;
    const re = /[A-Za-z][A-Za-z'-]*/g;
    let m;
    while ((m = re.exec(node.text))) {
      words.push({ word: m[0], from: pos + m.index, to: pos + m.index + m[0].length });
    }
  });
  return words;
}

async function askBridge(words) {
  if (!window.vela || typeof window.vela.invokeCommand !== 'function') return {};
  // Ask the bridge for mis-spelled indices in a single batch.
  return new Promise(resolve => {
    const id = window.vela.invokeCommand('spellCheck.batch', { words: words.map(w => w.word) });
    window.addEventListener('vela:commandResult', e => {
      if (e.detail.id !== id) return;
      resolve(e.detail.result || {});
    }, { once: true });
  });
}

export const SpellCheckPlugin = ({ locale = 'en-US' } = {}) => new Plugin({
  key,
  state: {
    init() { return { decorations: DecorationSet.empty, misspellings: {} }; },
    apply(tr, prev, _, newState) {
      const meta = tr.getMeta(key);
      if (meta && meta.misspellings) {
        const decos = [];
        for (const range of meta.misspellings) {
          decos.push(Decoration.inline(range.from, range.to, { class: 'vela-spell-issue' }));
        }
        return { ...prev, decorations: DecorationSet.create(newState.doc, decos), misspellings: meta.misspellings };
      }
      if (tr.docChanged)
        return { ...prev, decorations: DecorationSet.empty };
      return prev;
    },
  },
  props: {
    decorations(state) { return this.getState(state).decorations; },
  },

  view(editorView) {
    let scheduled = false;
    return {
      update(view) {
        if (scheduled) return;
        scheduled = true;
        setTimeout(async () => {
          scheduled = false;
          const words = collectWords(view.state.doc);
          if (words.length === 0) return;
          const result = await askBridge(words);
          const bad = new Set(result.misspelled || []);
          const ranges = words
            .filter(w => bad.has(w.word.toLowerCase()))
            .map(w => ({ from: w.from, to: w.to }));
          if (ranges.length)
            view.dispatch(view.state.tr.setMeta(key, { misspellings: ranges }));
        }, 350);
      },
    };
  },
});

export default SpellCheckPlugin;
