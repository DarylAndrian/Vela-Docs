// character-count.js — v0.3.1 (CRIT-01)
//
// We re-export the upstream CharacterCount extension and add a custom
// counter plugin that emits `{ words, characters, pages, readingTime }` to
// status.js whenever the document changes. Pages assume 250 wpm and 350
// words/page (Word-ish default).

import { CharacterCount } from '@tiptap/extension-character-count';

const WPM = 200;    // reading speed
const WORDS_PER_PAGE = 350;

function estimate(doc) {
  let words = 0;
  let characters = 0;
  doc.descendants(node => {
    if (node.isText) {
      const text = node.text || '';
      characters += text.length;
      words += (text.trim().match(/\S+/g) || []).length;
    }
    return true;
  });
  const pages    = Math.max(1, Math.ceil(words / WORDS_PER_PAGE));
  const reading  = Math.max(1, Math.ceil(words / WPM)); // minutes
  return { words, characters, pages, readingTime: reading };
}

export const VELA_COUNTER_NAMESPACE = 'vela';

export default CharacterCount.extend({
  name: 'characterCountVela',

  addProseMirrorPlugins() {
    return [{
      key: new this.editor.storage.characterCountVela?.key ?? undefined,
      // Forward stats to window.__vela_stats_cb if any caller subscribes.
      view: editorView => ({
        update: (view) => {
          if (typeof window !== 'undefined' && typeof window.__vela_stats_cb === 'function') {
            window.__vela_stats_cb(estimate(view.state.doc));
          }
        },
      }),
    }];
  },
});

export function count(doc) { return estimate(doc); }
