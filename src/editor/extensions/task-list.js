// task-list.js — v0.3.1 (CRIT-01)
//
// Task list with the `toggleTaskList` command the ribbon uses. Each item
// nests via `task-item` (sibling file).

import { TaskList } from '@tiptap/extension-task-list';

export default TaskList.extend({
  addCommands() {
    return {
      ...this.parent?.(),
      toggleTaskList:
        () => ({ commands }) => commands.toggleList('taskList', 'taskItem'),
    };
  },
});
