// ════════════════════════════════════════════════════════════════════════════
//  color-picker.js — v0.4.0
//
//  A popover color picker that mounts onto an anchor button.
//  Replaces the old fixed 8-swatch picker in main.js (v0.3.x).
//
//  Structure (built dynamically inside .color-picker[data-target]):
//
//      ┌─ Section: Standard  (8 swatches)
//      ├─ Section: Tints      (8 shades of last-used color)
//      ├─ Section: Custom     (color picker + R/G/B inputs + HEX field)
//      └─ Section: Recent     (last 8 custom colors, persisted)
//
//  Persistence:
//      web     →  localStorage["vela.recentColors.<target>"]
//      prod    →  window.vela.recentColors sync via EditorBridge
//
//  Usage:
//      import { mountColorPicker } from './color-picker.js';
//      mountColorPicker('textColor', document.getElementById('btn-text-color'), {
//          onApply: (color) => editor.chain().focus().setMark('textStyle', { color }).run(),
//          onClose: () => {},
//      });
// ════════════════════════════════════════════════════════════════════════════

const STANDARD_PALETTE = [
    '#000000', '#464555', '#BA1A1A', '#7E3000',
    '#3525CD', '#2F9E44', '#C92A2A', '#FFFFFF',
];

const HIGHLIGHT_PALETTE = [
    '#FFF3A0', '#FFD8A8', '#C3FAE8', '#D0BFFF',
    '#FFADAD', '#B5F4A5', '#A5D8FF', 'transparent',
];

const STORAGE_PREFIX = 'vela.recentColors.';

/**
 * Mount a color picker popover onto an anchor button.
 * @param {'textColor'|'highlight'} target
 * @param {HTMLElement} anchor
 * @param {{ onApply: (color: string) => void, onClose?: () => void }} callbacks
 */
export function mountColorPicker(target, anchor, { onApply, onClose }) {
    const container = document.querySelector(`.color-picker[data-target="${target}"]`);
    if (!container) {
        console.warn(`[color-picker] no container found for target="${target}"`);
        return;
    }

    // Build the popover DOM once
    if (container.childElementCount === 0) {
        buildPickerDom(container, target);
    }

    // Toggle on anchor click
    anchor.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        const wasHidden = container.hidden;
        document.querySelectorAll('.color-picker').forEach((p) => (p.hidden = true));
        container.hidden = !wasHidden;
        if (!container.hidden) {
            positionPicker(container, anchor);
            refreshRecentSection(container, target);
        }
    });

    // Swatch click → apply + close
    container.addEventListener('click', (e) => {
        const swatch = e.target.closest('[data-color]');
        if (swatch) {
            const color = swatch.dataset.color;
            onApply(color);
            pushRecentColor(target, color);
            container.hidden = true;
            onClose?.();
            return;
        }

        // Custom picker apply button
        if (e.target.closest('[data-role="apply"]')) {
            const hex = container.querySelector('[data-role="hex"]').value;
            if (isValidHex(hex)) {
                const normalized = normalizeHex(hex);
                onApply(normalized);
                pushRecentColor(target, normalized);
                container.hidden = true;
                onClose?.();
            }
            return;
        }

        // Cancel button
        if (e.target.closest('[data-role="cancel"]')) {
            container.hidden = true;
            onClose?.();
            return;
        }

        // "Clear color" (textColor) or "None" (highlight)
        if (e.target.closest('[data-role="clear"]')) {
            onApply('none');
            container.hidden = true;
            onClose?.();
            return;
        }
    });

    // HEX ↔ RGB sync inside the custom section
    syncCustomInputs(container);

    // Close on outside click
    document.addEventListener('click', (e) => {
        if (
            container.hidden &&
            !e.target.closest('[data-cmd="textColor"]') &&
            !e.target.closest('[data-cmd="highlight"]')
        ) {
            container.hidden = true;
        }
    });

    // Keyboard: Esc closes, Enter applies
    container.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            container.hidden = true;
            onClose?.();
        } else if (e.key === 'Enter') {
            const hex = container.querySelector('[data-role="hex"]')?.value;
            if (hex && isValidHex(hex)) {
                const normalized = normalizeHex(hex);
                onApply(normalized);
                pushRecentColor(target, normalized);
                container.hidden = true;
                onClose?.();
            }
        }
    });
}

// ── Internal helpers ────────────────────────────────────────────────────────

function buildPickerDom(container, target) {
    const palette = target === 'highlight' ? HIGHLIGHT_PALETTE : STANDARD_PALETTE;
    const allowNone = target === 'highlight';
    const removeLabel = target === 'highlight' ? 'Remove highlight' : 'Remove color';

    container.innerHTML = `
        <button class="cp-remove" data-role="clear" title="${removeLabel}">
            <span class="cp-remove-icon" aria-hidden="true">✕</span>
            <span>${removeLabel}</span>
        </button>

        <section class="cp-section">
            <header class="cp-section-title">Standard</header>
            <div class="cp-swatches">
                ${palette.map((c) => {
                    const isTransparent = c === 'transparent';
                    const cls = isTransparent ? 'cp-swatch cp-swatch-none' : 'cp-swatch';
                    const style = isTransparent
                        ? ''
                        : `background:${c}`;
                    const title = isTransparent
                        ? (target === 'highlight' ? 'No highlight' : 'No color')
                        : c;
                    return `<button class="${cls}"
                            data-color="${c}"
                            ${style ? `style="${style}"` : ''}
                            title="${title}"></button>`;
                }).join('')}
            </div>
        </section>

        <section class="cp-section">
            <header class="cp-section-title">Tints</header>
            <div class="cp-swatches cp-tints" data-role="tints"></div>
        </section>

        <section class="cp-section cp-custom" data-role="custom-section">
            <header class="cp-section-title">
                <span class="cp-caret">▸</span> Custom color
            </header>
            <div class="cp-custom-body" hidden>
                <div class="cp-custom-grid">
                    <input type="color" class="cp-color-input" data-role="color-input" value="#3525CD"/>
                    <div class="cp-custom-fields">
                        <label>R <input type="number" min="0" max="255" data-role="r" value="53"/></label>
                        <label>G <input type="number" min="0" max="255" data-role="g" value="37"/></label>
                        <label>B <input type="number" min="0" max="255" data-role="b" value="205"/></label>
                        <label class="cp-hex-label">HEX
                            <input type="text" data-role="hex" value="#3525CD" maxlength="7"/>
                        </label>
                    </div>
                </div>
                <div class="cp-custom-actions">
                    <button data-role="cancel">Cancel</button>
                    <button data-role="apply" class="primary">Apply</button>
                </div>
            </div>
        </section>

        <section class="cp-section">
            <header class="cp-section-title">Recent</header>
            <div class="cp-swatches cp-recent" data-role="recent"></div>
        </section>
    `;

    // Toggle custom section
    container.querySelector('.cp-caret').parentElement.addEventListener('click', () => {
        const body = container.querySelector('.cp-custom-body');
        const caret = container.querySelector('.cp-caret');
        body.hidden = !body.hidden;
        caret.textContent = body.hidden ? '▸' : '▾';
    });

    // Initial tints (from default color #3525CD)
    const tintsContainer = container.querySelector('[data-role="tints"]');
    renderTints(tintsContainer, '#3525CD', allowNone);
}

function syncCustomInputs(container) {
    const colorInput = container.querySelector('[data-role="color-input"]');
    const rInput = container.querySelector('[data-role="r"]');
    const gInput = container.querySelector('[data-role="g"]');
    const bInput = container.querySelector('[data-role="b"]');
    const hexInput = container.querySelector('[data-role="hex"]');

    function applyFromHex(hex) {
        if (!isValidHex(hex)) return;
        const normalized = normalizeHex(hex);
        const { r, g, b } = hexToRgb(normalized);
        rInput.value = r;
        gInput.value = g;
        bInput.value = b;
        colorInput.value = normalized;
    }

    function applyFromRgb() {
        const r = clamp255(rInput.value);
        const g = clamp255(gInput.value);
        const b = clamp255(bInput.value);
        const hex = rgbToHex(r, g, b);
        hexInput.value = hex;
        colorInput.value = hex;
    }

    colorInput.addEventListener('input', () => applyFromHex(colorInput.value));
    hexInput.addEventListener('input', () => applyFromHex(hexInput.value));
    [rInput, gInput, bInput].forEach((input) => {
        input.addEventListener('input', applyFromRgb);
    });
}

function renderTints(container, baseHex, includeNone) {
    const { h, s, l } = hexToHsl(baseHex);
    const tints = [];
    for (let i = 0; i < 8; i++) {
        const newL = 0.92 - (i / 7) * 0.7;     // 0.92 → 0.22
        tints.push(hslToHex(h, s, newL));
    }
    container.innerHTML = tints
        .map(
            (c) =>
                `<button class="cp-swatch" data-color="${c}" style="background:${c}" title="${c}"></button>`,
        )
        .join('');
}

function refreshRecentSection(container, target) {
    const recentEl = container.querySelector('[data-role="recent"]');
    if (!recentEl) return;
    const recent = getRecentColors(target);
    recentEl.innerHTML = recent.length
        ? recent
              .map(
                  (c) =>
                      `<button class="cp-swatch" data-color="${c}" style="background:${c}" title="${c}"></button>`,
              )
              .join('')
        : '<span class="cp-empty">No recent colors yet</span>';
}

function pushRecentColor(target, color) {
    if (color === 'none' || color === 'transparent') return;
    const key = STORAGE_PREFIX + target;
    let recent = [];
    try {
        recent = JSON.parse(localStorage.getItem(key) || '[]');
    } catch (_) {
        recent = [];
    }
    recent = [color, ...recent.filter((c) => c !== color)].slice(0, 8);
    try {
        localStorage.setItem(key, JSON.stringify(recent));
    } catch (_) {
        /* private mode */
    }

    // Sync to native bridge if present
    if (window.vela?.setRecentColor) {
        try {
            window.vela.setRecentColor(target, color);
        } catch (_) {
            /* ignore */
        }
    }

    // Refresh tints to be based on the new color
    const container = document.querySelector(`.color-picker[data-target="${target}"]`);
    if (container) {
        const tints = container.querySelector('[data-role="tints"]');
        const allowNone = target === 'highlight';
        renderTints(tints, color, allowNone);
    }
}

function getRecentColors(target) {
    try {
        return JSON.parse(localStorage.getItem(STORAGE_PREFIX + target) || '[]');
    } catch (_) {
        return [];
    }
}

function positionPicker(container, anchor) {
    const rect = anchor.getBoundingClientRect();
    container.style.left = `${rect.left}px`;
    container.style.top = `${rect.bottom + 4}px`;
}

// ── Color math ─────────────────────────────────────────────────────────────

export function isValidHex(s) {
    if (typeof s !== 'string') return false;
    return /^#?([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$/.test(s.trim());
}

export function normalizeHex(s) {
    let h = s.trim();
    if (!h.startsWith('#')) h = '#' + h;
    if (h.length === 4) {
        // #RGB → #RRGGBB
        h = '#' + h[1] + h[1] + h[2] + h[2] + h[3] + h[3];
    }
    return h.toUpperCase();
}

export function hexToRgb(hex) {
    const h = normalizeHex(hex).slice(1);
    return {
        r: parseInt(h.slice(0, 2), 16),
        g: parseInt(h.slice(2, 4), 16),
        b: parseInt(h.slice(4, 6), 16),
    };
}

export function rgbToHex(r, g, b) {
    return (
        '#' +
        [r, g, b]
            .map((v) => Math.max(0, Math.min(255, parseInt(v, 10) || 0)).toString(16).padStart(2, '0').toUpperCase())
            .join('')
    );
}

function clamp255(v) {
    const n = parseInt(v, 10);
    if (Number.isNaN(n)) return 0;
    return Math.max(0, Math.min(255, n));
}

export function hexToHsl(hex) {
    const { r, g, b } = hexToRgb(hex);
    const rn = r / 255, gn = g / 255, bn = b / 255;
    const max = Math.max(rn, gn, bn), min = Math.min(rn, gn, bn);
    let h = 0, s = 0;
    const l = (max + min) / 2;
    if (max !== min) {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        switch (max) {
            case rn: h = (gn - bn) / d + (gn < bn ? 6 : 0); break;
            case gn: h = (bn - rn) / d + 2; break;
            case bn: h = (rn - gn) / d + 4; break;
        }
        h /= 6;
    }
    return { h, s, l };
}

export function hslToHex(h, s, l) {
    let r, g, b;
    if (s === 0) {
        r = g = b = l;
    } else {
        const hue2rgb = (p, q, t) => {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1 / 6) return p + (q - p) * 6 * t;
            if (t < 1 / 2) return q;
            if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
            return p;
        };
        const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        const p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }
    return rgbToHex(Math.round(r * 255), Math.round(g * 255), Math.round(b * 255));
}