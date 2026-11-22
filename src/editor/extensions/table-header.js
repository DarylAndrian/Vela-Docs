// table-header.js — v0.3.1 (CRIT-01)
import { TableHeader } from '@tiptap/extension-table-header';

export default TableHeader.extend({
  addAttributes() {
    return {
      ...this.parent?.(),
      background: { default: null, parseHTML: el => el.getAttribute('data-bg'), renderHTML: a => a.background ? { 'data-bg': a.background, style: `background:${a.background}` } : {} },
    };
  },

  addCommands() {
    return {
      ...this.parent?.(),
      toggleTableHeader: () => ({ commands }) => commands.toggleHeader(),
    };
  },
});
