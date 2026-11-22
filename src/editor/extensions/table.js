// table.js — v0.3.1 (CRIT-01) + v0.3.2 polish (MAJOR-05)
//
// Wraps the upstream TipTap table extension with the cell-merge / split /
// header-row toggle / etc. commands that the playground already exposes.
// The visual resize handles and the Properties dialog wire up in v0.3.2.

import { Table } from '@tiptap/extension-table';

export default Table.extend({
  allowNested() { return true; }, // MAJOR-05 nested tables

  addCommands() {
    return {
      ...this.parent?.(),
      insertTableEx:
        ({ rows = 3, cols = 3, withHeaderRow = true } = {}) =>
        ({ commands }) =>
          commands.insertTable({ rows, cols, withHeaderRow }),
      deleteCurrentTable:
        () => ({ tr, editor, commands }) => {
          const { $from } = editor.state.selection;
          for (let depth = $from.depth; depth > 0; depth--) {
            const node = $from.node(depth);
            if (node.type.name === 'table') {
              return commands.deleteTable();
            }
          }
          return false;
        },
    };
  },
});
