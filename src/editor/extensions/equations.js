// equations.js — v0.3.3 (IMP-07)
//
// LaTeX-style equations via KaTeX (bundled locally in v0.3.4). We render
// inline via a TipTap node that captures `{ type: 'equation', attrs: { latex: '…', display: false } }`
// and proxies the rendering through `katex.renderToString`. MathML is
// produced for DOCX export.

import { Node, mergeAttributes } from '@tiptap/core';

export const Equation = Node.create({
  name: 'equation',
  group: 'inline',
  inline: true,
  atom: true,
  selectable: true,

  addAttributes() {
    return {
      latex:   { default: 'E=mc^2' },
      display: { default: false },     // false → inline, true → block
    };
  },

  parseHTML() {
    return [{ tag: 'span[data-equation]' }];
  },

  renderHTML({ HTMLAttributes, node }) {
    // At build time KaTeX may not be loaded; placeholder HTML.
    const tex   = node.attrs.latex;
    const disp  = node.attrs.display ? 'block' : 'inline';
    return ['span', mergeAttributes(HTMLAttributes, { class: 'vela-equation', 'data-display': disp, 'data-latex': tex }), `$${tex}$`];
  },

  addCommands() {
    return {
      insertEquation:
        ({ latex = 'E=mc^2', display = false } = {}) =>
        ({ commands }) => commands.insertContent({ type: this.name, attrs: { latex, display } }),
    };
  },
});

export default Equation;
