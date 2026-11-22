// command-dispatcher.js — v0.3.1
//
// Central command bus for the editor. Maps every ribbon / keyboard / menu
// action to a TipTap command + the EditorBridge C++ call.
//
// Async commands (file IO) get a request id, await `vela.commandResult(id)`,
// and resolve a promise. Sync commands just execute and return.
//
// Usage (from main.js):
//   import { dispatcher, registerCommand } from './command-dispatcher.js';
//   registerCommand('toggleBold', (args, editor) => editor.chain().focus().toggleBold().run());
//   await dispatcher.run('toggleBold', {}, editor);

const handlers = new Map();
let nextId = 1;

export function registerCommand(name, fn) {
  handlers.set(name, fn);
}

export const dispatcher = {
  run(name, args, editor) {
    const fn = handlers.get(name);
    if (!fn)
      return { ok: false, error: `unknown command: ${name}` };

    // File / save / dirty commands traverse the bridge.
    const FILE_OPS = new Set(['openDocument','saveDocument','saveDocumentAs','newDocument','closeDocument']);

    if (FILE_OPS.has(name)) {
      if (window.vela && typeof window.vela.invokeCommand === 'function') {
        const id = window.vela.invokeCommand(name, args || {});
        return new Promise(resolve => {
          const onResult = (...args) => {
            const [rid, ok, payload] = args;
            if (rid !== id) return;
            window.removeEventListener?.('vela:commandResult', onResult);
            resolve({ ok, ...payload });
          };
          // QWebChannel dispatches via the property setters on window.vela;
          // for safety we also expose a fallback CustomEvent if available.
          const slot = (payload) => onResult(...[id, payload.ok, payload]);
          if (window.vela && typeof window.vela.commandResult === 'object') {
            // no-op; QWebChannel will set window.vela.commandResult directly.
          }
          // Listen on a global event as a fallback for tests / dev tools.
          window.addEventListener('vela:commandResult', e => slot(e.detail), { once: true });
          // Timeout safety net.
          setTimeout(() => resolve({ ok: false, error: 'timeout' }), 5500);
        });
      }
    }

    // Sync / local-only command.
    try {
      const out = fn(args || {}, editor);
      return { ok: true, value: out };
    } catch (err) {
      if (window.vela && typeof window.vela.errorReported === 'function') {
        window.vela.errorReported(name, String(err));
      }
      return { ok: false, error: String(err) };
    }
  },
};

// Pre-register every command the playground/ribbon expects (v0.3.1 baseline).
registerCommand('toggleBold',        (_, ed) => ed.chain().focus().toggleBold().run());
registerCommand('toggleItalic',      (_, ed) => ed.chain().focus().toggleItalic().run());
registerCommand('toggleUnderline',   (_, ed) => ed.chain().focus().toggleUnderline().run());
registerCommand('toggleStrike',      (_, ed) => ed.chain().focus().toggleStrike().run());
registerCommand('toggleCode',        (_, ed) => ed.chain().focus().toggleCode().run());
registerCommand('toggleBlockquote',  (_, ed) => ed.chain().focus().toggleBlockquote().run());
registerCommand('toggleCodeBlock',   (_, ed) => ed.chain().focus().toggleCodeBlock().run());
registerCommand('toggleTaskList',    (_, ed) => ed.chain().focus().toggleTaskList().run());
registerCommand('toggleBulletList',  (_, ed) => ed.chain().focus().toggleBulletList().run());
registerCommand('toggleOrderedList', (_, ed) => ed.chain().focus().toggleOrderedList().run());
registerCommand('indent',            (_, ed) => ed.chain().focus().sinkListItem('listItem').run());
registerCommand('outdent',           (_, ed) => ed.chain().focus().liftListItem('listItem').run());
registerCommand('setHeading',        (args, ed) => ed.chain().focus().toggleHeading({ level: args.level || 1 }).run());
registerCommand('setTextAlign',      (args, ed) => ed.chain().focus().setTextAlign(args.align).run());
registerCommand('setFontFamily',     (args, ed) => ed.chain().focus().setFontFamily(args.family).run());
registerCommand('setFontSize',       (args, ed) => ed.chain().focus().setFontSize(`${args.size}pt`).run());
registerCommand('setTextColor',      (args, ed) => ed.chain().focus().setColor(args.color).run());
registerCommand('setHighlight',      (args, ed) => ed.chain().focus().toggleHighlight({ color: args.color }).run());
registerCommand('clearFormatting',   (_, ed) => ed.chain().focus().unsetAllMarks().clearNodes().run());
registerCommand('insertLink',        (args, ed) => ed.chain().focus().setLink({ href: args.href, target: '_blank' }).run());
registerCommand('insertPageBreak',   (_, ed) => ed.chain().focus().setPageBreak().run());
registerCommand('insertHR',          (_, ed) => ed.chain().focus().setHorizontalRule().run());
registerCommand('insertFootnote',    (_, ed) => ed.chain().focus().insertContent({ type: 'footnote', attrs: { id: String(Date.now()) } }).run());
registerCommand('insertSpecialChar', (args, ed) => ed.chain().focus().insertContent(args.char).run());
registerCommand('insertTableEx',     (args, ed) => ed.chain().focus().insertTableEx(args).run());
registerCommand('insertImageFromUrl',(args, ed) => ed.chain().focus().insertImageFromUrl(args.url, args.alt).run());
registerCommand('undo',              (_, ed) => ed.chain().focus().undo().run());
registerCommand('redo',              (_, ed) => ed.chain().focus().redo().run());
