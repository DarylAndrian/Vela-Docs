// ════════════════════════════════════════════════════════════════════════════
//  page-break.js — v0.5.0
//
//  A TipTap Node representing an explicit page break.
//
//  Renders as `<div data-page-break class="vela-page-break">`. Atom node — it
//  has no editable content; the user selects it as a single unit. Provides:
//    • setPageBreak() — insert a page break at the cursor
//    • unsetPageBreak() — remove a page break at the cursor
//
//  Keyboard: Ctrl/Cmd+Enter inserts a page break (Word / Google Docs convention).
//  This intentionally shadows HardBreak's Mod-Enter binding; Shift-Enter still
//  inserts a HardBreak (line break within a paragraph). TipTap keymaps are
//  registered such that later-registered extensions take precedence, so this
//  extension must be registered AFTER StarterKit.
// ════════════════════════════════════════════════════════════════════════════

import { Node, mergeAttributes } from '@tiptap/core';

export const PageBreak = Node.create({
    name: 'pageBreak',
    group: 'block',
    atom: true,
    selectable: true,
    draggable: false,

    parseHTML() {
        return [
            { tag: 'div[data-page-break]' },
            { tag: 'hr[data-page-break]' },
            // Legacy: stale v0.3 bundles serialised page breaks as an <hr> with
            // a special class — accept it but normalise on parse.
            { tag: 'hr.vela-page-break' },
        ];
    },

    renderHTML({ HTMLAttributes }) {
        return [
            'div',
            mergeAttributes(HTMLAttributes, {
                'data-page-break': '',
                class: 'vela-page-break',
                contenteditable: 'false',
            }),
        ];
    },

    renderText() {
        return '';
    },

    addCommands() {
        return {
            setPageBreak:
                () =>
                ({ chain }) => {
                    return chain()
                        .insertContent({ type: this.name })
                        // Place the cursor in a fresh paragraph after the break
                        // so the user can keep typing on the next page.
                        .insertContent({ type: 'paragraph' })
                        .run();
                },
            unsetPageBreak:
                () =>
                ({ state, dispatch }) => {
                    const { selection } = state;
                    const node = selection.node;
                    if (node && node.type.name === this.name) {
                        if (dispatch) dispatch(state.tr.deleteSelection());
                        return true;
                    }
                    return false;
                },
        };
    },

    addKeyboardShortcuts() {
        return {
            'Mod-Enter': () => this.editor.commands.setPageBreak(),
        };
    },

    addInputRules() {
        // No input rules — page breaks are inserted explicitly via Ctrl+Enter
        // or the Insert toolbar. Typing "---" still inserts an <hr>.
        return [];
    },
});

export default PageBreak;
