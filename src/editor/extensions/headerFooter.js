// headerFooter.js — v0.3.2 (MAJOR-01)
//
// A TipTap Node that models the document header/footer region of the page.
// In production the page is rendered in a virtualized column; in
// `editor-playground/` the header/footer appears above/below the body.
//
// The Mark contains free text plus `{PAGE}` / `{NUMPAGES}` / `{roman}`
// tokens, all of which HeaderFooter.cpp renders into the visible string.

import { Node, mergeAttributes } from '@tiptap/core';

export const HEADER_FOOTER_TOKEN = /\{(PAGE|NUMPAGES|ROMAN|roman|NUMROMAN|numroman)\}/g;

export const HeaderFooter = Node.create({
  name: 'headerFooter',
  group: 'block',
  content: 'inline*',
  defining: true,

  addAttributes() {
    return {
      kind: {
        default: 'header',                  // 'header' | 'footer'
        parseHTML: el => el.getAttribute('data-kind') || 'header',
        renderHTML: a => ({ 'data-kind': a.kind }),
      },
      differentFirst: {
        default: false,
        parseHTML: el => el.getAttribute('data-different-first') === '1',
        renderHTML: a => a.differentFirst ? { 'data-different-first': '1' } : {},
      },
    };
  },

  parseHTML() { return [{ tag: 'header,footer' }]; },

  renderHTML({ HTMLAttributes }) {
    const tag = HTMLAttributes['data-kind'] === 'footer' ? 'footer' : 'header';
    return [tag, mergeAttributes(HTMLAttributes, { class: 'vela-section' }), 0];
  },

  addCommands() {
    return {
      insertHeader:
        (text = '') =>
        ({ commands }) =>
          commands.insertContent({ type: this.name, attrs: { kind: 'header' }, content: [{ type: 'text', text }] }),
      insertFooter:
        (text = '') =>
        ({ commands }) =>
          commands.insertContent({ type: this.name, attrs: { kind: 'footer' }, content: [{ type: 'text', text }] }),
    };
  },
});

export default HeaderFooter;
