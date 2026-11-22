// ════════════════════════════════════════════════════════════════════════════
//  page-settings.js — v0.5.0
//
//  Page settings model + persistence for the web editor. Mirrors the C++
//  PageStyle model (src/document/PageStyle.h) so DOCX round-trip and the web
//  view stay in sync.
//
//  Paper sizes are stored by name (A4 / Letter / Legal / Tabloid). Dimensions
//  in twips are exposed for C++ interop via window.vela.setPageSettings(json).
// ════════════════════════════════════════════════════════════════════════════

import { PAPER_SIZES_MM } from './extensions/pagination.js';

const STORAGE_KEY = 'vela.pageSettings';
const TWIPS_PER_MM = 1440 / 25.4;

// Twip dimensions for each paper size (portrait). Landscape swaps w/h.
export const PAPER_SIZES_TWIPS = {
    A4:     { w: 11906, h: 16838 },
    Letter: { w: 12240, h: 15840 },
    Legal:  { w: 12240, h: 20160 },
    Tabloid:{ w: 15840, h: 24480 },
};

export const DEFAULT_SETTINGS = Object.freeze({
    mode: 'pageless',       // 'pageless' | 'paging'
    size: 'A4',
    orientation: 'portrait',// 'portrait' | 'landscape'
    marginTop: 1,           // inches
    marginRight: 1,
    marginBottom: 1,
    marginLeft: 1,
    pageGap: 24,            // px (visual gap between sheets in paging mode)
    // Header / footer
    headerText: '',
    footerText: '',
    differentFirstPage: false,
    // Page numbering
    pageNumberFormat: 'decimal', // 'decimal' | 'upperRoman' | 'lowerRoman' | 'upperAlpha' | 'lowerAlpha'
    pageNumberStartAt: 1,
});

/**
 * Load persisted settings (merged with defaults). Returns a fresh object.
 */
export function loadSettings() {
    let stored = {};
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        if (raw) stored = JSON.parse(raw);
    } catch (_) { stored = {}; }
    return { ...DEFAULT_SETTINGS, ...stored };
}

/**
 * Persist settings to localStorage.
 */
export function saveSettings(settings) {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
    } catch (_) { /* private mode */ }
}

/**
 * Convert settings → twip dimensions (for C++ PageStyle round-trip).
 */
export function settingsToTwips(s) {
    const dims = PAPER_SIZES_TWIPS[s.size] || PAPER_SIZES_TWIPS.A4;
    const portrait = s.orientation !== 'landscape';
    return {
        widthTwips:  portrait ? dims.w : dims.h,
        heightTwips: portrait ? dims.h : dims.w,
        marginTopTwips:    Math.round((s.marginTop    ?? 1) * 1440),
        marginRightTwips:  Math.round((s.marginRight  ?? 1) * 1440),
        marginBottomTwips: Math.round((s.marginBottom ?? 1) * 1440),
        marginLeftTwips:   Math.round((s.marginLeft   ?? 1) * 1440),
    };
}

/**
 * Apply settings to the pagination controller + expose to native bridge.
 */
export function applySettings(pagination, settings) {
    if (!pagination) return;
    const { mode } = settings;
    if (mode === 'paging' && !pagination.isEnabled()) {
        pagination.enable(settings);
    } else if (mode === 'pageless' && pagination.isEnabled()) {
        pagination.disable();
    } else if (pagination.isEnabled()) {
        pagination.setSettings(settings);
    }
    saveSettings(settings);

    // Push to native (C++ PageSetupDialog / PageStyle model).
    if (window.vela?.setPageSettings) {
        try {
            window.vela.setPageSettings(JSON.stringify({ ...settings, ...settingsToTwips(settings) }));
        } catch (_) { /* bridge not present in playground */ }
    }
}

/**
 * Register the global handler that the C++ side calls to push settings into
 * the web editor.
 *
 *   window.vela.setPageSettings(JSON.parse(json))
 *
 * We instead expose window.__velaSetPageSettings(jsonStr) so the playground
 * (no QWebChannel) and the Qt host both work the same way.
 */
export function registerPageSettingsBridge(pagination, onApplied) {
    window.__velaSetPageSettings = (jsonStr) => {
        let incoming;
        try {
            incoming = typeof jsonStr === 'string' ? JSON.parse(jsonStr) : jsonStr;
        } catch (_) { return false; }
        if (!incoming || typeof incoming !== 'object') return false;
        const merged = { ...loadSettings(), ...incoming };
        applySettings(pagination, merged);
        onApplied?.(merged);
        return true;
    };
}
