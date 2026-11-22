// image.js — v0.3.1 (CRIT-01)
//
// Adds Image support with the lasso-set alt text + class hooks. The
// MAJOR-06 polish (resize handles / 5 wrap modes / paste from clipboard /
// drag-drop / compression) lands in v0.3.2.

import { Image } from '@tiptap/extension-image';

export const VELA_IMAGE_DEFAULTS = Object.freeze({
  inline:          false,
  allowBase64:     true,
  HTMLAttributes:  { class: 'vela-image' },
});

export default Image.extend({
  addOptions()    { return { ...VELA_IMAGE_DEFAULTS, ...this.parent?.() }; },

  addCommands() {
    return {
      ...this.parent?.(),
      insertImageFromUrl:
        (url, alt = '') =>
        ({ commands }) => commands.insertContent({
          type: 'image',
          attrs: { src: url, alt, title: alt },
        }),
      setImageAlt:
        (alt) =>
        ({ commands }) =>
          commands.updateAttributes('image', { alt, title: alt }),
    };
  },
});
