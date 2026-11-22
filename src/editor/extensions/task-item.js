// task-item.js — v0.3.1 (CRIT-01)
//
// Task item with our checkbox styling. Nesting is left to ProseMirror's
// list-item default behaviour.

import { TaskItem } from '@tiptap/extension-task-item';

export default TaskItem.extend({
  addOptions() {
    return { ...this.parent?.(), nested: true, HTMLAttributes: { class: 'vela-task-item' } };
  },
});
