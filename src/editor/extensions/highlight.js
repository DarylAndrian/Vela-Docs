// highlight.js — v0.3.1 (CRIT-01)
// TipTap Highlight extension with the 8-colour Vela Precision palette.
// Re-exports the official extension with our colour swatch table attached so
// the ribbon can read it.

import { Highlight } from '@tiptap/extension-highlight';

/** @type {string[]} The 8 highlight swatches. */
export const VELA_HIGHLIGHTS = [
  '#fff59d', // yellow
  '#c8e6a0', // green
  '#a0d8ef', // blue
  '#f4a8a8', // red
  '#dab4f5', // purple
  '#ffd8a8', // orange
  '#bfd4c2', // mint
  '#bfbfbf', // grey
];

export default Highlight.extend({
  addOptions() {
    return {
      ...this.parent?.(),
      multicolor: true,
      HTMLAttributes: { class: 'vela-highlight' },
    };
  },
});
