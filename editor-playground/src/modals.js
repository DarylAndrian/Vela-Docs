// ════════════════════════════════════════════════════════════════════════════
//  modals.js — v0.4.0
//
//  Modal dialogs for the playground. Existing dialogs (openInsertTable,
//  openInsertImage, promptLink) are kept from v0.3.x; v0.4.0 adds
//  openStyleEditor and openTabStopDialog.
//
//  All dialogs return a Promise. They mount into a #modal-root element in
//  the DOM (created on first use).
// ════════════════════════════════════════════════════════════════════════════

// ── Existing v0.3.x dialogs (kept unchanged) ──────────────────────────────

export function openInsertTable() {
    return openModal({
        title: 'Insert Table',
        render: (root, resolve) => {
            root.innerHTML = `
                <div class="modal-form">
                    <label>Rows <input type="number" id="modal-rows" value="3" min="1" max="50"/></label>
                    <label>Columns <input type="number" id="modal-cols" value="3" min="1" max="20"/></label>
                    <label><input type="checkbox" id="modal-header" checked/> Include header row</label>
                    <div class="modal-actions">
                        <button class="modal-btn" data-action="cancel">Cancel</button>
                        <button class="modal-btn primary" data-action="ok">Insert</button>
                    </div>
                </div>
            `;
            root.querySelector('[data-action="ok"]').addEventListener('click', () => {
                const rows = parseInt(root.querySelector('#modal-rows').value, 10);
                const cols = parseInt(root.querySelector('#modal-cols').value, 10);
                resolve({ rows, cols });
            });
            root.querySelector('[data-action="cancel"]').addEventListener('click', () => resolve(null));
        },
    });
}

export function openInsertImage() {
    return openModal({
        title: 'Insert Image',
        render: (root, resolve) => {
            root.innerHTML = `
                <div class="modal-form">
                    <label>Image URL <input type="text" id="modal-url" placeholder="https://… or data:…"/></label>
                    <label>Alt text <input type="text" id="modal-alt" placeholder="Description for accessibility"/></label>
                    <div class="modal-actions">
                        <button class="modal-btn" data-action="cancel">Cancel</button>
                        <button class="modal-btn primary" data-action="ok">Insert</button>
                    </div>
                </div>
            `;
            root.querySelector('[data-action="ok"]').addEventListener('click', () => {
                const url = root.querySelector('#modal-url').value.trim();
                if (!url) return resolve(null);
                resolve({ url, alt: root.querySelector('#modal-alt').value.trim() });
            });
            root.querySelector('[data-action="cancel"]').addEventListener('click', () => resolve(null));
        },
    });
}

export function promptLink() {
    const url = window.prompt('Enter URL (https://…)', 'https://');
    return url || null;
}

// ── NEW v0.4.0: openStyleEditor ──────────────────────────────────────────

const FONT_FAMILIES = [
    'Inter', 'Source Serif 4', 'Georgia', 'Times New Roman', 'Calibri',
    'Arial', 'JetBrains Mono', 'Cascadia Code',
];

const ALIGNMENT_OPTIONS = ['left', 'center', 'right', 'justify'];

/**
 * Opens the inline style editor. If `existingDef` is provided, the form is
 * pre-filled (used for "Modify 'X'…").
 */
export function openStyleEditor(existingDef) {
    const def = existingDef || {
        id: '',
        name: '',
        basedOn: 'Normal',
        font: 'Inter',
        size: 12,
        bold: false,
        italic: false,
        underline: false,
        color: '#222222',
        alignment: 'left',
        spacingBefore: 0,
        spacingAfter: 6,
    };

    return openModal({
        title: existingDef ? 'Modify "' + existingDef.name + '"' : 'Create New Style',
        width: 560,
        render: (root, resolve) => {
            root.innerHTML = `
                <div class="modal-form modal-style-editor">
                    <div class="modal-row">
                        <label>Name
                            <input type="text" id="se-name" value="${escapeAttr(def.name)}" placeholder="Heading 6, Caption, …"/>
                        </label>
                        <label>ID (auto)
                            <input type="text" id="se-id" value="${escapeAttr(def.id)}" placeholder="Heading6"/>
                        </label>
                    </div>
                    <div class="modal-row">
                        <label>Based on
                            <select id="se-basedOn">
                                ${['Normal', 'Heading1', 'Heading2', 'Heading3', 'Heading4', 'Heading5', 'Title', 'Subtitle', 'Quote', 'Code']
                                    .map((b) => `<option ${def.basedOn === b ? 'selected' : ''}>${b}</option>`)
                                    .join('')}
                            </select>
                        </label>
                        <label>Font family
                            <select id="se-font">
                                ${FONT_FAMILIES.map((f) => `<option ${def.font === f ? 'selected' : ''}>${f}</option>`).join('')}
                            </select>
                        </label>
                    </div>
                    <div class="modal-row">
                        <label>Font size
                            <input type="number" id="se-size" value="${def.size}" min="6" max="200"/> pt
                        </label>
                        <label>Text color
                            <input type="color" id="se-color" value="${def.color || '#222222'}"/>
                            <input type="text" id="se-color-hex" value="${def.color || '#222222'}" maxlength="7"/>
                        </label>
                    </div>
                    <div class="modal-row modal-row-flags">
                        <label><input type="checkbox" id="se-bold" ${def.bold ? 'checked' : ''}/> Bold</label>
                        <label><input type="checkbox" id="se-italic" ${def.italic ? 'checked' : ''}/> Italic</label>
                        <label><input type="checkbox" id="se-underline" ${def.underline ? 'checked' : ''}/> Underline</label>
                    </div>
                    <div class="modal-row">
                        <label>Alignment
                            <select id="se-align">
                                ${ALIGNMENT_OPTIONS.map((a) => `<option value="${a}" ${def.alignment === a ? 'selected' : ''}>${a[0].toUpperCase() + a.slice(1)}</option>`).join('')}
                            </select>
                        </label>
                        <label>Spacing before <input type="number" id="se-before" value="${def.spacingBefore || 0}" min="0" max="200"/> pt</label>
                        <label>Spacing after <input type="number" id="se-after" value="${def.spacingAfter || 0}" min="0" max="200"/> pt</label>
                    </div>
                    <div class="modal-preview" id="se-preview">Preview text</div>
                    <div class="modal-actions">
                        <button class="modal-btn" data-action="cancel">Cancel</button>
                        <button class="modal-btn primary" data-action="ok">${existingDef ? 'Save changes' : 'Create'}</button>
                    </div>
                </div>
            `;

            const updatePreview = () => {
                const preview = root.querySelector('#se-preview');
                preview.style.fontFamily = root.querySelector('#se-font').value;
                preview.style.fontSize = root.querySelector('#se-size').value + 'pt';
                preview.style.color = root.querySelector('#se-color-hex').value;
                preview.style.fontWeight = root.querySelector('#se-bold').checked ? 'bold' : 'normal';
                preview.style.fontStyle = root.querySelector('#se-italic').checked ? 'italic' : 'normal';
                preview.style.textDecoration = root.querySelector('#se-underline').checked ? 'underline' : 'none';
                preview.style.textAlign = root.querySelector('#se-align').value;
                preview.style.marginTop = root.querySelector('#se-before').value + 'pt';
                preview.style.marginBottom = root.querySelector('#se-after').value + 'pt';
            };
            root.querySelectorAll('input, select').forEach((el) => {
                el.addEventListener('input', updatePreview);
                el.addEventListener('change', updatePreview);
            });

            const colorInput = root.querySelector('#se-color');
            const hexInput = root.querySelector('#se-color-hex');
            colorInput.addEventListener('input', () => {
                hexInput.value = colorInput.value.toUpperCase();
                updatePreview();
            });
            hexInput.addEventListener('input', () => {
                if (/^#[0-9A-Fa-f]{6}$/.test(hexInput.value)) {
                    colorInput.value = hexInput.value;
                    updatePreview();
                }
            });

            const nameInput = root.querySelector('#se-name');
            const idInput = root.querySelector('#se-id');
            nameInput.addEventListener('input', () => {
                if (!idInput.value || idInput.dataset.auto !== 'false') {
                    const auto = nameInput.value
                        .replace(/\s+/g, '')
                        .replace(/[^A-Za-z0-9]/g, '');
                    idInput.value = auto;
                    idInput.dataset.auto = 'true';
                }
            });
            idInput.addEventListener('input', () => {
                idInput.dataset.auto = 'false';
            });

            updatePreview();

            root.querySelector('[data-action="ok"]').addEventListener('click', () => {
                const name = nameInput.value.trim();
                const id = idInput.value.trim();
                if (!name || !id) {
                    window.alert('Both Name and ID are required.');
                    return;
                }
                resolve({
                    id,
                    name,
                    basedOn: root.querySelector('#se-basedOn').value,
                    font: root.querySelector('#se-font').value,
                    size: parseInt(root.querySelector('#se-size').value, 10),
                    bold: root.querySelector('#se-bold').checked,
                    italic: root.querySelector('#se-italic').checked,
                    underline: root.querySelector('#se-underline').checked,
                    color: hexInput.value.toUpperCase(),
                    alignment: root.querySelector('#se-align').value,
                    spacingBefore: parseInt(root.querySelector('#se-before').value, 10),
                    spacingAfter: parseInt(root.querySelector('#se-after').value, 10),
                });
            });

            root.querySelector('[data-action="cancel"]').addEventListener('click', () => resolve(null));
        },
    });
}

// ── NEW v0.4.0: openTabStopDialog ────────────────────────────────────────

const TAB_TYPES = ['left', 'center', 'right', 'decimal', 'bar'];
const TAB_LEADERS = ['none', 'dot', 'hyphen', 'underscore'];

/**
 * Opens the Tab Stop dialog. Lets the user add/remove/edit a list of stops.
 */
export function openTabStopDialog(currentStops) {
    let stops = currentStops.map((s) => ({ ...s }));

    return openModal({
        title: 'Tab Stops',
        width: 460,
        render: (root, resolve) => {
            const renderList = () => {
                const list = root.querySelector('#ts-list');
                list.innerHTML = stops.length === 0
                    ? '<li class="ts-empty">No custom tab stops. Default stops every 0.5\u2033.</li>'
                    : stops.map((s, i) => `
                        <li>
                            <input type="number" step="0.05" min="0.1" max="7" value="${s.pos}"
                                data-i="${i}" data-f="pos" class="ts-pos"/>
                            <select data-i="${i}" data-f="type" class="ts-type">
                                ${TAB_TYPES.map((t) => `<option ${s.type === t ? 'selected' : ''}>${t}</option>`).join('')}
                            </select>
                            <select data-i="${i}" data-f="leader" class="ts-leader">
                                ${TAB_LEADERS.map((l) => `<option ${s.leader === l ? 'selected' : ''}>${l}</option>`).join('')}
                            </select>
                            <button data-i="${i}" data-action="remove" class="ts-remove">\u2715</button>
                        </li>
                    `).join('');
            };

            root.innerHTML = `
                <div class="modal-form modal-tabstop">
                    <p class="ts-help">Default stops are at every 0.5\u2033 until you add a custom one.</p>
                    <ul class="ts-list" id="ts-list"></ul>
                    <button class="ts-add" data-action="add">+ Add tab stop at\u2026</button>
                    <div class="modal-actions">
                        <button class="modal-btn" data-action="cancel">Cancel</button>
                        <button class="modal-btn" data-action="clearAll">Clear all</button>
                        <button class="modal-btn primary" data-action="ok">OK</button>
                    </div>
                </div>
            `;

            renderList();

            root.querySelector('[data-action="add"]').addEventListener('click', () => {
                const pos = parseFloat(window.prompt('Tab stop position (inches):', '1.0') || '');
                if (isNaN(pos) || pos <= 0 || pos > 7) return;
                stops.push({ pos, type: 'left', leader: 'none' });
                stops.sort((a, b) => a.pos - b.pos);
                renderList();
                wireList();
            });

            const wireList = () => {
                root.querySelectorAll('input[data-i], select[data-i]').forEach((el) => {
                    el.addEventListener('input', () => {
                        const i = parseInt(el.dataset.i, 10);
                        const f = el.dataset.f;
                        if (f === 'pos') stops[i].pos = parseFloat(el.value) || 0;
                        else stops[i][f] = el.value;
                    });
                });
                root.querySelectorAll('[data-action="remove"]').forEach((b) => {
                    b.addEventListener('click', () => {
                        const i = parseInt(b.dataset.i, 10);
                        stops.splice(i, 1);
                        renderList();
                        wireList();
                    });
                });
            };
            wireList();

            root.querySelector('[data-action="clearAll"]').addEventListener('click', () => {
                stops = [];
                renderList();
                wireList();
            });
            root.querySelector('[data-action="ok"]').addEventListener('click', () => resolve(stops));
            root.querySelector('[data-action="cancel"]').addEventListener('click', () => resolve(null));
        },
    });
}

// ── Modal infrastructure ──────────────────────────────────────────────────

function openModal({ title, render, width = 460 }) {
    return new Promise((resolve) => {
        const root = ensureModalRoot();
        const previousFocus = document.activeElement;
        const overlay = document.createElement('div');
        overlay.className = 'modal-overlay';

        const dialog = document.createElement('div');
        dialog.className = 'modal-dialog';
        dialog.setAttribute('role', 'dialog');
        dialog.setAttribute('aria-modal', 'true');
        dialog.setAttribute('aria-label', title);
        dialog.style.width = width + 'px';
        dialog.innerHTML = `
            <header class="modal-header">
                <h2>${escapeHtml(title)}</h2>
                <button class="modal-close" data-action="close" aria-label="Close">\u2715</button>
            </header>
            <div class="modal-body"></div>
        `;
        overlay.appendChild(dialog);

        const body = dialog.querySelector('.modal-body');
        const closeWith = (value) => {
            overlay.remove();
            document.removeEventListener('keydown', onKey, true);
            previousFocus?.focus();
            resolve(value);
        };
        const onKey = (e) => {
            if (e.key === 'Escape') {
                e.preventDefault();
                e.stopPropagation();
                closeWith(null);
            }
            if (e.key !== 'Tab') return;
            const fields = [...dialog.querySelectorAll('button, input, select, textarea, a[href]')]
                .filter((field) => !field.disabled && field.getClientRects().length);
            const first = fields[0];
            const last = fields.at(-1);
            if (!dialog.contains(document.activeElement) || (e.shiftKey ? document.activeElement === first : document.activeElement === last)) {
                e.preventDefault();
                (e.shiftKey ? last : first)?.focus();
            }
        };
        render(body, closeWith);
        overlay.addEventListener('click', (e) => {
            if (e.target === overlay) closeWith(null);
        });
        dialog.querySelector('[data-action="close"]').addEventListener('click', () => closeWith(null));
        document.addEventListener('keydown', onKey, true);

        root.appendChild(overlay);

        body.querySelector('input, select, textarea, button')?.focus();
    });
}

function ensureModalRoot() {
    let root = document.getElementById('modal-root');
    if (!root) {
        root = document.createElement('div');
        root.id = 'modal-root';
        document.body.appendChild(root);
    }
    return root;
}

// HTML escape functions — built with String.fromCharCode() so the
// '&' character never appears as a literal in the source.
function escapeHtml(s) {
    var AMP = String.fromCharCode(38);
    return String(s)
        .split(AMP).join(AMP + 'amp;')
        .split('<').join(AMP + 'lt;')
        .split('>').join(AMP + 'gt;');
}

function escapeAttr(s) {
    var AMP = String.fromCharCode(38);
    return escapeHtml(s).split('"').join(AMP + 'quot;');
}
