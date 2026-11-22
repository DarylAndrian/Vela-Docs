// ════════════════════════════════════════════════════════════════════════════
//  custom-style.js — v0.4.0
//
//  A TipTap Node extension for arbitrary user-defined paragraph styles
//  (Heading 6, Heading 7, "Caption", "Sidebar heading", …).
//
//  The actual visual style is applied via a `data-style-id` attribute on the
//  rendered paragraph; the StyleManager (style-manager.js) holds the style
//  definitions and renders matching CSS variables at startup.
//
//  Corresponds to DOCX `<w:pStyle w:val="UserStyle1" w:customStyle="1"/>`.
// ════════════════════════════════════════════════════════════════════════════

import { Node, mergeAttributes } from '@tiptap/core';

export const CustomStyle = Node.create({
    name: 'customStyle',
    group: 'block',
    content: 'inline*',
    defining: true,

    addAttributes() {
        return {
            styleId: {
                default: null,
                parseHTML: (el) => el.getAttribute('data-style-id'),
                renderHTML: (attrs) => {
                    if (!attrs.styleId) return {};
                    return { 'data-style-id': attrs.styleId };
                },
            },
        };
    },

    parseHTML() {
        return [{ tag: 'p[data-style-id]' }, { tag: 'div[data-style-id]' }];
    },

    renderHTML({ HTMLAttributes }) {
        return [
            'p',
            mergeAttributes(HTMLAttributes, { class: 'vela-custom-style' }),
            0,
        ];
    },

    addCommands() {
        return {
            setCustomStyle:
                (styleId) =>
                ({ commands }) => {
                    if (!styleId) return false;
                    return commands.setNode(this.name, { styleId });
                },
            unsetCustomStyle:
                () =>
                ({ commands }) => {
                    return commands.setParagraph();
                },
        };
    },
});

export default CustomStyle;