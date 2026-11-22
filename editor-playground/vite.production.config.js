// vite.production.config.js — v0.4.0
// Build a single offline-bundle for the Qt-embedded editor.
//
// Output: src/editor/dist/editor.bundle.js
// Plus fonts and icons copied to src/editor/dist/assets/
//
// Usage (from repo root):
//   cd editor-playground
//   npm install
//   npm run build:production
//
// The output is then registered in resources/resources.qrc and loaded from
// src/editor/index.html via `qrc:/editor/dist/editor.bundle.js`.

import { defineConfig } from 'vite';
import { resolve } from 'node:path';

export default defineConfig({
    root: '.',
    publicDir: false,
    base: '',
    resolve: {
        alias: {
            // The shared editor pieces live in src/editor/extensions/, src/editor/
            // modals.js, and src/editor/command-dispatcher.js. Resolve them so we
            // never have to duplicate code between the playground and production.
            '~dispatcher': resolve(__dirname, '../src/editor/command-dispatcher.js'),
            '~modals':     resolve(__dirname, '../src/editor/modals.js'),
            // TiTapp dependencies (verbatim from dev config for parity)
            '@tiptap/core':                  '@tiptap/core',
            '@tiptap/starter-kit':           '@tiptap/starter-kit',
            '@tiptap/extension-underline':   '@tiptap/extension-underline',
            '@tiptap/extension-text-align':  '@tiptap/extension-text-align',
            '@tiptap/extension-link':        '@tiptap/extension-link',
            '@tiptap/extension-placeholder':  '@tiptap/extension-placeholder',
            '@tiptap/extension-highlight':   '@tiptap/extension-highlight',
            '@tiptap/extension-image':       '@tiptap/extension-image',
            '@tiptap/extension-table':       '@tiptap/extension-table',
            '@tiptap/extension-table-row':   '@tiptap/extension-table-row',
            '@tiptap/extension-table-header': '@tiptap/extension-table-header',
            '@tiptap/extension-table-cell':   '@tiptap/extension-table-cell',
            '@tiptap/extension-task-list':   '@tiptap/extension-task-list',
            '@tiptap/extension-task-item':   '@tiptap/extension-task-item',
            '@tiptap/extension-character-count':'@tiptap/extension-character-count',
            '@tiptap/extension-text-style': '@tiptap/extension-text-style',
            '@tiptap/extension-color':      '@tiptap/extension-color',
        },
    },
    build: {
        outDir: resolve(__dirname, '../src/editor/dist'),
        emptyOutDir: true,
        target: 'es2020',
        minify: 'esbuild',
        sourcemap: false,
        cssCodeSplit: false,
        // The bundle is loaded from qrc:/, which has no filesystem to resolve
        // relative asset URLs against. Inline the icon font (the only binary
        // asset) so dist/ contains exactly the two files index.html references.
        assetsInlineLimit: 1024 * 1024,
        rollupOptions: {
            // One entry, one classic script. A second `modals` entry used to
            // force code-splitting, which made editor.bundle.js an ES module
            // (`import … from "./modals.bundle.js"`). index.html loads these as
            // classic scripts, so the bundle threw
            // "Cannot use import statement outside a module" and the editor
            // never started. IIFE also sidesteps module CORS over the qrc: scheme.
            input: {
                editor: resolve(__dirname, 'src/main.js'),
            },
            output: {
                format: 'iife',
                inlineDynamicImports: true,
                manualChunks: undefined,
                entryFileNames: '[name].bundle.js',
                // cssCodeSplit:false yields one stylesheet. Name it exactly what
                // src/editor/index.html links: editor.bundle.css.
                assetFileNames: (assetInfo) => {
                    const name = assetInfo.name
                        ?? (Array.isArray(assetInfo.names) ? assetInfo.names[0] : '');
                    if (name && name.endsWith('.css')) return 'editor.bundle.css';
                    return 'assets/[name][extname]';
                },
            },
        },
    },
    optimizeDeps: {
        include: [
            '@tiptap/core',
            '@tiptap/starter-kit',
            '@tiptap/extension-underline',
            '@tiptap/extension-text-align',
            '@tiptap/extension-link',
            '@tiptap/extension-placeholder',
            '@tiptap/extension-highlight',
            '@tiptap/extension-image',
            '@tiptap/extension-table',
            '@tiptap/extension-table-row',
            '@tiptap/extension-table-header',
            '@tiptap/extension-table-cell',
            '@tiptap/extension-task-list',
            '@tiptap/extension-task-item',
            '@tiptap/extension-character-count',
            '@tiptap/extension-text-style',
            '@tiptap/extension-color',
        ],
    },
});