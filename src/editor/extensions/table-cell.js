// table-cell.js — v0.3.1 (CRIT-01)
import { TableCell } from '@tiptap/extension-table-cell';

export default TableCell.extend({
  addAttributes() {
    return {
      ...this.parent?.(),
      colspan: { default: 1, parseHTML: el => parseInt(el.getAttribute('colspan') || '1', 10), renderHTML: a => a.colspan > 1 ? { colspan: String(a.colspan) } : {} },
      rowspan: { default: 1, parseHTML: el => parseInt(el.getAttribute('rowspan') || '1', 10), renderHTML: a => a.rowspan > 1 ? { rowspan: String(a.rowspan) } : {} },
      background: { default: null, parseHTML: el => el.getAttribute('data-bg'), renderHTML: a => a.background ? { 'data-bg': a.background, style: `background:${a.background}` } : {} },
    };
  },

  addCommands() {
    return {
      ...this.parent?.(),
      mergeSelectedCells: () => ({ commands }) => commands.mergeCells(),
      splitCell:          () => ({ commands }) => commands.splitCell(),
    };
  },
});
