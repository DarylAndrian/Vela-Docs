// comments.js — v0.3.3 (IMP-02)
//
// Anchored comment marks. Each `commentId` mark wraps a text range; the
// `CommentStore` on the C++ side persists anchor + author + timestamp and
// serialises to DOCX `<w:comments>`.

import { Mark, mergeAttributes } from '@tiptap/core';

export const CommentMark = Mark.create({
  name: 'comment',
  inclusive: false,

  addAttributes() {
    return {
      commentId: { default: null, parseHTML: el => el.getAttribute('data-comment'), renderHTML: a => ({ 'data-comment': a.commentId }) },
      author:    { default: 'me',  parseHTML: el => el.getAttribute('data-author'), renderHTML: a => ({ 'data-author': a.author }) },
      resolved:  { default: false, parseHTML: el => el.getAttribute('data-resolved') === '1', renderHTML: a => a.resolved ? { 'data-resolved': '1' } : {} },
    };
  },

  parseHTML() { return [{ tag: 'span[data-comment]' }]; },
  renderHTML({ HTMLAttributes }) {
    return ['span', mergeAttributes(HTMLAttributes, { class: 'vela-comment' }), 0];
  },

  addCommands() {
    return {
      setComment:
        (id) =>
        ({ commands }) => commands.setMark(this.name, { commentId: id }),
      unsetComment:
        (id) =>
        ({ tr, state, dispatch }) => {
          // Remove the comment mark across any range(s) matching id.
          const { CommentMark } = this;
          const ranges = [];
          state.doc.descendants((node, pos) => {
            if (node.marks) {
              for (const m of node.marks) {
                if (m.type === CommentMark && m.attrs.commentId === id)
                  ranges.push({ from: pos, to: pos + node.nodeSize });
              }
            }
          });
          if (ranges.length === 0) return false;
          if (dispatch) {
            for (const r of ranges.reverse())
              tr.removeMark(r.from, r.to, CommentMark);
            dispatch(tr);
          }
          return true;
        },
    };
  },
});

export default CommentMark;
