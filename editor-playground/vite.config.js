// ════════════════════════════════════════════════════════════════════════════
//  vite.config.js — Dev server for the Vela Docs TipTap editor (v0.3.1)
// ════════════════════════════════════════════════════════════════════════════
//  Run with:   npm run dev   →  opens http://localhost:5173
//  Production: npm run build:production
//               (uses vite.production.config.js → writes src/editor/dist/)
// ════════════════════════════════════════════════════════════════════════════
import { defineConfig } from 'vite';
import { resolve } from 'node:path';

export default defineConfig({
    server: {
        port: 5173,
        open: true,
        strictPort: false,
        // Allow Vite to read files outside `editor-playground/`, since the
        // editor bundle shares code with the production app in
        // ../src/editor/ (command-dispatcher.js, modals.js, extensions/).
        fs: {
            strict: false,
            allow: [
                resolve(__dirname),
                resolve(__dirname, '../src'),
                resolve(__dirname, '../resources'),
            ],
        },
    },
    build: {
        outDir: 'dist',
        sourcemap: true,
        target: 'es2020',
    },
    // Make these paths work when embedded in the Qt app at qrc:///
    base: './',

    // The shared editor pieces live in src/editor/extensions/, src/editor/
    // modals.js, and src/editor/command-dispatcher.js. Resolve them so we
    // never have to duplicate code between the playground and production.
    resolve: {
        alias: {
            '~dispatcher': resolve(__dirname, '../src/editor/command-dispatcher.js'),
            '~modals':     resolve(__dirname, '../src/editor/modals.js'),
        },
    },
});
