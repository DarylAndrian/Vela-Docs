// revisions.js — v0.3.3 (IMP-03)
//
// Track-changes is achieved by the `RevisionStore` running on the C++ side
// plus two TipTap Marks: `insertion` and `deletion`. The marks attach
// `author` + `when` metadata and round-trip to DOCX `<w:ins>` / `<w:del>`
// in v0.3.3's exporter.

import { Mark, mergeAttributes } from '@tiptap/core';

const make = (name, tag) => Mark.create({
  name,
  inclusive: false,

  addAttributes() {
    return {
      author: { default: 'me',           parseHTML: el => el.getAttribute('data-author'),  renderHTML: a => ({ 'data-author': a.author }) },
      when:   { default: () => new Date().toISOString(),
                                            parseHTML: el => el.getAttribute('data-when'),   renderHTML: a => ({ 'data-when': a.when }) },
    };
  },

  parseHTML() { return [{ tag }]; },
  renderHTML({ HTMLAttributes }) {
    return [tag, mergeAttributes(HTMLAttributes, { class: `vela-${name}` }), 0];
  },
});

export const InsertionMark = make('insertion', 'ins');
export const DeletionMark  = make('deletion',  'del');

export default { InsertionMark, DeletionMark };
