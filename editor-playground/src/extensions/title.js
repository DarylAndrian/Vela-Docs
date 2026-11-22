// ════════════════════════════════════════════════════════════════════════════
//  title.js — v0.4.0
//
//  A TipTap Node for the "Title" paragraph style — large, bold, centered.
//  Corresponds to DOCX `<w:pStyle w:val="Title"/>`.
//
//  Registers as a `title` node in the schema. Renders as a `<h1 class="vela-title">`.
//  Provides `setTitle()` / `unsetTitle()` commands.
// ════════════════════════════════════════════════════════════════════════════

import { Node, mergeAttributes } from '@tiptap/core';

export const Title = Node.create({
    name: 'title',
    group: 'block',
    content: 'inline*',
    defining: true,

    parseHTML() {
        return [{ tag: 'h1.vela-title' }, { tag: 'p[data-style="title"]' }];
    },

    renderHTML({ HTMLAttributes }) {
        return [
            'h1',
            mergeAttributes(HTMLAttributes, { class: 'vela-title', 'data-style': 'title' }),
            0,
        ];
    },

    addCommands() {
        return {
            setTitle:
                () =>
                ({ commands }) => {
                    return commands.setNode(this.name);
                },
            unsetTitle:
                () =>
                ({ commands }) => {
                    return commands.setParagraph();
                },
        };
    },

    addKeyboardShortcuts() {
        return {
            'Mod-shift-1': () => this.editor.commands.setTitle(),
        };
    },
});

export default Title;