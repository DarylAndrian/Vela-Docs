// table-row.js — v0.3.1 (CRIT-01)
// Re-export of the upstream TableRow extension with the additional commands
// the ribbon exposes (add row above/below, delete row).

import { TableRow } from '@tiptap/extension-table-row';

export default TableRow.extend({
  addCommands() {
    return {
      ...this.parent?.(),
      addRowAbove: () => ({ commands }) => commands.addRowBefore(),
      addRowBelow: () => ({ commands }) => commands.addRowAfter(),
      deleteRow:   () => ({ commands }) => commands.deleteRow(),
    };
  },
});
