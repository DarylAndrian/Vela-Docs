// ════════════════════════════════════════════════════════════════════════════
//  backstage.js — v0.4.0
//
//  Playground-side controller for the Microsoft Word-style Files-tab
//  backstage overlay. The DOM is in index.html (.backstage + .backstage-rail
//  + .backstage-content). This module wires up the rail-item buttons,
//  mounts each panel into the content area, handles open/close/animation,
//  and routes actions back to the caller via the `handlers` option.
//
//  Usage:
//      import { mountBackstage } from './backstage.js';
//      mountBackstage(document.getElementById('backstage'), {
//          onSave, onSaveAs, onOpen, onPrint, onPrintPreview, ...
//      });
// ════════════════════════════════════════════════════════════════════════════

const PANEL_DEFINITIONS = [
    {
        id: 'save',
        icon: 'save',
        title: 'Save',
        build: () => buildSavePanel(),
    },
    {
        id: 'saveAs',
        icon: 'save_as',
        title: 'Save As',
        build: () => buildSaveAsPanel(),
    },
    {
        id: 'open',
        icon: 'folder_open',
        title: 'Open',
        build: () => buildOpenPanel(),
    },
    {
        id: 'print',
        icon: 'print',
        title: 'Print',
        build: () => buildPrintPanel(),
    },
    {
        id: 'printPreview',
        icon: 'visibility',
        title: 'Print Preview',
        build: () => buildPrintPreviewPanel(),
    },
    {
        id: 'options',
        icon: 'settings',
        title: 'Options',
        build: () => buildOptionsPanel(),
    },
    {
        id: 'help',
        icon: 'help',
        title: 'Help',
        build: () => buildHelpPanel(),
    },
    {
        id: 'recent',
        icon: 'history',
        title: 'Recent',
        build: () => buildRecentPanel(),
    },
];

export function mountBackstage(rootEl, handlers = {}) {
    if (!rootEl) return null;

    const rail = rootEl.querySelector('.backstage-rail');
    const content = rootEl.querySelector('.backstage-content');

    // Wire rail items
    rail.querySelectorAll('.backstage-item').forEach((btn) => {
        const id = btn.dataset.panel;
        btn.addEventListener('click', (e) => {
            e.preventDefault();
            open(id);
        });
    });

    // Wire close button
    const closeBtn = rootEl.querySelector('.backstage-close');
    closeBtn?.addEventListener('click', (e) => {
        e.preventDefault();
        close();
    });

    // Default first panel
    let currentPanelId = null;
    const panelsById = new Map();

    function open(panelId = 'save') {
        if (!PANEL_DEFINITIONS.find((p) => p.id === panelId)) return;
        // Highlight rail
        rail.querySelectorAll('.backstage-item').forEach((b) =>
            b.classList.toggle('is-active', b.dataset.panel === panelId));

        // Mount panel content
        if (!panelsById.has(panelId)) {
            const def = PANEL_DEFINITIONS.find((p) => p.id === panelId);
            const node = def.build(handlers);
            panelsById.set(panelId, node);
            content.appendChild(node);
        }
        // Hide all others
        panelsById.forEach((n, id) => {
            n.hidden = id !== panelId;
        });
        currentPanelId = panelId;
        rootEl.hidden = false;
        rootEl.classList.add('is-visible');
    }

    function close() {
        rootEl.hidden = true;
        rootEl.classList.remove('is-visible');
    }

    function toggle(panelId) {
        if (rootEl.hidden || rootEl.classList.contains('is-visible') === false) {
            open(panelId || currentPanelId || 'save');
        } else {
            close();
        }
    }

    // Backdrop click closes (but only if not clicking inside content)
    rootEl.addEventListener('click', (e) => {
        if (e.target === rootEl) close();
    });

    return { open, close, toggle };
}

// ── Panel builders ────────────────────────────────────────────────────────

function buildSavePanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Save</h2>
        <p class="bs-summary">Your document is up to date.</p>
        <div class="bs-actions">
            <button class="bs-btn primary" data-action="save">
                <span class="ms">save</span> Save
            </button>
        </div>
        <div class="bs-formats">
            <h3>Save as another format</h3>
            <button class="bs-btn" data-action="saveAsDocx">
                <span class="ms">description</span> Word Document (.docx)
            </button>
            <button class="bs-btn" data-action="saveAsPdf">
                <span class="ms">picture_as_pdf</span> PDF (.pdf)
            </button>
            <button class="bs-btn" data-action="saveAsMd">
                <span class="ms">code</span> Markdown (.md)
            </button>
        </div>
    `;
    return el;
}

function buildSaveAsPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Save As</h2>
        <p class="bs-summary">Choose a location and file format.</p>
        <div class="bs-formats">
            <button class="bs-btn" data-action="saveAsDocx">
                <span class="ms">description</span> Word Document (.docx)
            </button>
            <button class="bs-btn" data-action="saveAsOdt">
                <span class="ms">article</span> OpenDocument Text (.odt)
            </button>
            <button class="bs-btn" data-action="saveAsRtf">
                <span class="ms">text_fields</span> Rich Text Format (.rtf)
            </button>
            <button class="bs-btn" data-action="saveAsPdf">
                <span class="ms">picture_as_pdf</span> PDF (.pdf)
            </button>
            <button class="bs-btn" data-action="saveAsMd">
                <span class="ms">code</span> Markdown (.md)
            </button>
            <button class="bs-btn" data-action="saveAsHtml">
                <span class="ms">html</span> HTML (.html)
            </button>
            <button class="bs-btn" data-action="saveAsTxt">
                <span class="ms">description</span> Plain Text (.txt)
            </button>
        </div>
    `;
    return el;
}

function buildOpenPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Open</h2>
        <div class="bs-actions">
            <button class="bs-btn primary" data-action="openBrowse">
                <span class="ms">folder_open</span> Browse…
            </button>
        </div>
        <h3>Recent documents</h3>
        <div class="bs-recent-grid" id="bs-open-recent"></div>
    `;
    populateRecent(el.querySelector('#bs-open-recent'));
    return el;
}

function buildPrintPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Print</h2>
        <div class="bs-print-grid">
            <label>Printer <select id="bs-printer">
                <option>Default printer</option>
            </select></label>
            <label>Copies <input type="number" id="bs-copies" value="1" min="1"/></label>
            <label>Pages
                <select id="bs-pages">
                    <option value="all">All</option>
                    <option value="current">Current page</option>
                    <option value="range">Range…</option>
                </select>
            </label>
        </div>
        <div class="bs-actions">
            <button class="bs-btn primary" data-action="print">
                <span class="ms">print</span> Print
            </button>
        </div>
    `;
    return el;
}

function buildPrintPreviewPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel bs-print-preview';
    el.innerHTML = `
        <h2>Print Preview</h2>
        <p class="bs-summary">The print preview renders the document exactly as it will appear on paper.</p>
        <div class="bs-preview-controls">
            <button class="bs-btn" data-action="zoomOut"><span class="ms">zoom_out</span></button>
            <span class="bs-zoom-label">100%</span>
            <button class="bs-btn" data-action="zoomIn"><span class="ms">zoom_in</span></button>
            <button class="bs-btn primary" data-action="print">
                <span class="ms">print</span> Print
            </button>
        </div>
        <div class="bs-preview-frame">
            <iframe id="bs-preview-iframe" title="Print preview"></iframe>
        </div>
    `;
    const iframe = el.querySelector('#bs-preview-iframe');
    setTimeout(() => {
        const doc = window.__editor?.getHTML?.() || '<p>No document</p>';
        const html = `<!doctype html><html><head><style>
            body { font-family: Georgia, serif; max-width: 7.5in; margin: 1in auto; line-height: 1.5; color: #222; }
            h1, h2 { color: #3525CD; }
        </style></head><body>${doc}</body></html>`;
        iframe.srcdoc = html;
    }, 50);
    return el;
}

function buildOptionsPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Options</h2>
        <p class="bs-summary">Vela Docs preferences — Documents, Display, Editing, Privacy, Keybindings, Language, Templates, Mail Merge.</p>
        <div class="bs-formats">
            <button class="bs-btn" data-action="openPreferences">
                <span class="ms">tune</span> Open Preferences…
            </button>
        </div>
    `;
    return el;
}

function buildHelpPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Help</h2>
        <div class="bs-formats">
            <button class="bs-btn" data-action="openAbout">
                <span class="ms">info</span> About Vela Docs
            </button>
            <button class="bs-btn" data-action="openUserGuide">
                <span class="ms">menu_book</span> User guide
            </button>
            <button class="bs-btn" data-action="openChangelog">
                <span class="ms">history</span> Changelog
            </button>
            <button class="bs-btn" data-action="openShortcuts">
                <span class="ms">keyboard</span> Keyboard shortcuts
            </button>
        </div>
        <div class="bs-shortcut-grid">
            <h3>Common shortcuts</h3>
            <table>
                <tr><th>Action</th><th>Shortcut</th></tr>
                <tr><td>Bold</td><td><kbd>Ctrl</kbd>+<kbd>B</kbd></td></tr>
                <tr><td>Italic</td><td><kbd>Ctrl</kbd>+<kbd>I</kbd></td></tr>
                <tr><td>Underline</td><td><kbd>Ctrl</kbd>+<kbd>U</kbd></td></tr>
                <tr><td>Save</td><td><kbd>Ctrl</kbd>+<kbd>S</kbd></td></tr>
                <tr><td>Open Files</td><td><kbd>Alt</kbd>+<kbd>F</kbd></td></tr>
                <tr><td>Heading 1</td><td><kbd>Ctrl</kbd>+<kbd>1</kbd></td></tr>
                <tr><td>Heading 2</td><td><kbd>Ctrl</kbd>+<kbd>2</kbd></td></tr>
                <tr><td>Bullet list</td><td><kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>L</kbd></td></tr>
                <tr><td>Numbered list</td><td><kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>O</kbd></td></tr>
                <tr><td>Insert link</td><td><kbd>Ctrl</kbd>+<kbd>K</kbd></td></tr>
                <tr><td>Find</td><td><kbd>Ctrl</kbd>+<kbd>F</kbd></td></tr>
                <tr><td>Replace</td><td><kbd>Ctrl</kbd>+<kbd>H</kbd></td></tr>
                <tr><td>Print</td><td><kbd>Ctrl</kbd>+<kbd>P</kbd></td></tr>
            </table>
        </div>
    `;
    return el;
}

function buildRecentPanel() {
    const el = document.createElement('section');
    el.className = 'bs-panel';
    el.innerHTML = `
        <h2>Recent</h2>
        <div class="bs-recent-grid" id="bs-recent-grid"></div>
    `;
    populateRecent(el.querySelector('#bs-recent-grid'));
    return el;
}

function populateRecent(container) {
    if (!container) return;
    // Mock recent files for the playground
    const samples = [
        'Resume.docx', 'Project-proposal.docx', 'Meeting-notes.docx',
        'Quarterly-report.docx', 'Cover-letter.docx',
    ];
    container.innerHTML = samples
        .map(
            (name) => `
            <button class="bs-recent-card">
                <div class="bs-recent-thumb"><span class="ms">description</span></div>
                <div class="bs-recent-name">${name}</div>
            </button>`,
        )
        .join('');
}