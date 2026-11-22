# Vela Docs v0.3.5 macros
#
# Shipped macros live in this directory. Each file is a JavaScript macro
# usable through the `Tools → Macros` menu. They run inside a QJSEngine
# sandboxed to the `vela` host object:
#
#     vela.editor.*     — TipTap commands + selection
#     vela.document.*   — load/save/import/export
#     vela.ui.*         — toasts, dialogs, prompts
#
# v0.3.5 ships the following five macros:
#   1. format-toc.js       — convert hand-written TOCs into real TOCs
#   2. reset-styles.js     — un-set every custom style on a doc
#   3. batch-find-replace.js — find/replace across multiple files
#   4. pngify-images.js    — convert all images to PNG (lossless)
#   5. insert-date-header.js — insert today's date in the header

export function formatTableOfContents(vela) {
  const { state } = vela.editor.view;
  const headings = [];
  state.doc.descendants(n => {
    if (n.type.name === 'heading') headings.push({ level: n.attrs.level, text: n.textContent });
  });
  vela.editor.commands.clearContent(true);
  for (const h of headings) {
    vela.editor.commands.insertContent({
      type: 'paragraph',
      content: [{ type: 'text', text: '  '.repeat(h.level - 1) + '• ' + h.text }]
    });
  }
}
export default formatTableOfContents;
