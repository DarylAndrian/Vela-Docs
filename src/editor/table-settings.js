// ════════════════════════════════════════════════════════════════════════════
//  table-settings.js — Vela Docs Editor v0.4.2
//
//  Tabbed table settings panel (Borders / Cell / Table / Row & column).
//  Replaces the v0.3.2 TablePropertiesDialog modal popup with an inline
//  right-side panel that doesn't disrupt the editing flow.
//
//  All fields carry Material Symbols icons in their labels. The border
//  presets show a 4-icon quick-picker (All / Outside / Inside / None)
//  instead of a dropdown.
//
//  Lives in editor-playground/src/ (mirrored in src/editor/ for production).
// ════════════════════════════════════════════════════════════════════════════

export function mountTableSettingsPanel({ panel, tabs, panes, closeBtn, footer, onApply, onReset, editor }) {
    // Tab switching
    tabs.forEach((tab) => {
        tab.addEventListener('click', () => {
            const target = tab.dataset.tab;
            tabs.forEach((t) => {
                const active = t.dataset.tab === target;
                t.classList.toggle('is-active', active);
                t.setAttribute('aria-selected', active ? 'true' : 'false');
            });
            panes.forEach((p) => {
                p.hidden = p.dataset.pane !== target;
                p.classList.toggle('is-active', p.dataset.pane === target);
            });
        });
    });

    // Border preset selection (mutually exclusive)
    const presets = panel.querySelectorAll('.tsp-preset');
    presets.forEach((p) => {
        p.addEventListener('click', () => {
            presets.forEach((q) => q.classList.toggle('is-active', q === p));
        });
    });

    // Vertical-align icon-row (mutually exclusive)
    const valignBtns = panel.querySelectorAll('[data-valign]');
    valignBtns.forEach((b) => {
        b.addEventListener('click', () => {
            valignBtns.forEach((q) => q.classList.toggle('is-active', q === b));
        });
    });

    // Close
    const close = () => { panel.hidden = true; };
    closeBtn?.addEventListener('click', close);

    // Collect all form values
    function collectValues() {
        const activePreset = panel.querySelector('.tsp-preset.is-active');
        const activeValign = panel.querySelector('[data-valign].is-active');
        return {
            borderPreset: activePreset?.dataset.preset || 'all',
            borderStyle: panel.querySelector('#tsp-border-style')?.value || 'single',
            borderColor: panel.querySelector('#tsp-border-color')?.value || '#888888',
            borderWidth: parseInt(panel.querySelector('#tsp-border-width')?.value, 10) || 1,
            cellPadding: parseInt(panel.querySelector('#tsp-cell-padding')?.value, 10) || 6,
            cellSpacing: parseInt(panel.querySelector('#tsp-cell-spacing')?.value, 10) || 0,
            cellShading: panel.querySelector('#tsp-cell-shading')?.value || '#ffffff',
            verticalAlign: activeValign?.dataset.valign || 'middle',
            tableWidth: parseInt(panel.querySelector('#tsp-table-width')?.value, 10) || 100,
            tableAlign: panel.querySelector('#tsp-table-align')?.value || 'left',
            tableAutofit: panel.querySelector('#tsp-table-autofit')?.value || 'window',
            tableHeader: panel.querySelector('#tsp-table-header')?.checked || false,
            rowHeight: parseInt(panel.querySelector('#tsp-row-height')?.value, 10) || 20,
            colWidth: parseInt(panel.querySelector('#tsp-col-width')?.value, 10) || 120,
        };
    }

    // Footer buttons
    footer.querySelector('[data-action="apply"]')?.addEventListener('click', () => {
        const values = collectValues();
        onApply?.(values);
        close();
    });
    footer.querySelector('[data-action="reset"]')?.addEventListener('click', () => {
        onReset?.();
        // Reset to defaults
        panel.querySelectorAll('.tsp-preset').forEach((p, i) => p.classList.toggle('is-active', i === 0));
        panel.querySelectorAll('[data-valign]').forEach((v, i) => v.classList.toggle('is-active', i === 1));
        const defaults = {
            'tsp-border-style': 'single',
            'tsp-border-color': '#888888',
            'tsp-border-width': 1,
            'tsp-cell-padding': 6,
            'tsp-cell-spacing': 0,
            'tsp-cell-shading': '#ffffff',
            'tsp-table-width': 100,
            'tsp-table-align': 'left',
            'tsp-table-autofit': 'window',
            'tsp-table-header': true,
            'tsp-row-height': 20,
            'tsp-col-width': 120,
        };
        for (const [id, v] of Object.entries(defaults)) {
            const el = panel.querySelector('#' + id);
            if (el) {
                if (el.type === 'checkbox') el.checked = v;
                else el.value = v;
            }
        }
    });

    return { close, collectValues };
}