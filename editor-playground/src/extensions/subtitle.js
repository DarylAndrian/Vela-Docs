// ════════════════════════════════════════════════════════════════════════════
//  subtitle.js — v0.4.0
//
//  A TipTap Node for the "Subtitle" paragraph style — slightly smaller than
//  Title, italic, gray. Corresponds to DOCX `<w:pStyle w:val="Subtitle"/>`.
// ════════════════════════════════════════════════════════════════════════════

import { Node, mergeAttributes } from '@tiptap/core';

export const Subtitle = Node.create({
    name: 'subtitle',
    group: 'block',
    content: 'inline*',
    defining: true,

    parseHTML() {
        return [{ tag: 'h2.vela-subtitle' }, { tag: 'p[data-style="subtitle"]' }];
    },

    renderHTML({ HTMLAttributes }) {
        return [
            'h2',
            mergeAttributes(HTMLAttributes, { class: 'vela-subtitle', 'data-style': 'subtitle' }),
            0,
        ];
    },

    addCommands() {
        return {
            setSubtitle:
                () =>
                ({ commands }) => {
                    return commands.setNode(this.name);
                },
            unsetSubtitle:
                () =>
                ({ commands }) => {
                    return commands.setParagraph();
                },
        };
    },

    addKeyboardShortcuts() {
        return {
            'Mod-shift-2': () => this.editor.commands.setSubtitle(),
        };
    },
});

export default Subtitle;