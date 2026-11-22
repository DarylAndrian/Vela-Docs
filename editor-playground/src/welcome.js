// ════════════════════════════════════════════════════════════════════════════
//  welcome.js — Vela Docs first-run overlay (v0.4.1)
//
//  Pattern reference: stitch_vela_docs_ui_system/welcome_to_vela_docs
//
//  Behavior:
//    • Mounts a full-page welcome overlay when launched for the first time.
//    • Has a "Skip" button (right side of the topbar) to dismiss.
//    • Lists five starter templates (Blank, Resume, Letter, Report, Invoice)
//      drawn as styled HTML/CSS mocks — click handler lets the caller react.
//    • Lists recent documents (placeholder 4-card grid; the caller can pass
//      actual recent files via options.recent).
//    • Reopens if every document is closed and the user opens Files again —
//      (the editor exposes `welcome.open()` from main.js when needed).
//
//  Usage:
//      import { mountWelcome } from './welcome.js';
//      const welcome = mountWelcome(document.body, {
//          version: '0.4.1',
//          onPickTemplate: (id) => { ... },
//          onPickRecent:   (recentItem) => { ... },
//          recent: [...],   // optional; if absent, the empty state is shown
//      });
// ════════════════════════════════════════════════════════════════════════════

const STORAGE_KEY = 'vela.welcome.dismissedAt';

/**
 * @typedef {Object} WelcomeOptions
 * @property {string} [version]      App version shown in the topbar badge.
 * @property {WelcomeTemplateId[]} [templates] Override the default list.
 * @property {Function} [onPickTemplate] Called with the template id when a user clicks a template card.
 * @property {Function} [onPickRecent]   Called with the recent file when a user clicks a recent card.
 * @property {WelcomeRecentItem[]} [recent] Recent files. Empty list shows the empty state.
 * @property {boolean} [autoShow]  If false, does not show until welcome.open() is called.
 */

/**
 * @typedef {'blank'|'resume'|'letter'|'report'|'invoice'} WelcomeTemplateId
 */

/**
 * @typedef {Object} WelcomeRecentItem
 * @property {string} name
 * @property {string} [subtitle]
 * @property {'docx'|'odt'|'rtf'|'md'|'pdf'|'txt'} [kind]
 */

/**
 * Mount the welcome overlay into the host element.
 * @param {HTMLElement} host
 * @param {WelcomeOptions} [options]
 * @returns {{ open: Function, close: Function, isOpen: Function }}
 */
export function mountWelcome(host, options = {}) {
    const version = options.version || '0.4.1';
    const templates = options.templates || DEFAULT_TEMPLATES;
    const dismissThresholdMs = 1000 * 60 * 60 * 24 * 7; // 7 days

    let open = false;
    let lastDismissedAt = Number(localStorage.getItem(STORAGE_KEY) || 0);
    const fresh = !lastDismissedAt || (Date.now() - lastDismissedAt) > dismissThresholdMs;

    // ── DOM ────────────────────────────────────────────────────────────────
    const root = document.createElement('div');
    root.className = 'welcome-root';
    if (!fresh && options.autoShow !== false) {
        // Wait — only hide by default when caller asked us to auto-show. We
        // leave the controls on screen until the caller decides. First run:
        // open by default unless explicitly disabled.
        root.classList.add('is-hidden');
    }
    root.innerHTML = buildShell(version, templates, options.recent || []);
    host.appendChild(root);

    // ── Events ────────────────────────────────────────────────────────────
    // Skip button
    root.querySelector('[data-action="skip"]')?.addEventListener('click', () => close(true));

    // Theme toggle inside welcome
    root.querySelector('[data-action="theme"]')?.addEventListener('click', () => {
        const cur = document.documentElement.dataset.theme;
        const next = cur === 'dark' ? 'light' : 'dark';
        document.documentElement.dataset.theme = next;
        // Mirror topbar state
        const topBtn = document.querySelector('#toggle-theme .ms');
        if (topBtn) topBtn.textContent = next === 'dark' ? 'dark_mode' : 'light_mode';
        const themeName = document.getElementById('theme-name');
        if (themeName) themeName.textContent = next === 'dark' ? 'Dark' : 'Light';
    });

    // Template cards
    root.querySelectorAll('[data-template]').forEach((el) => {
        el.addEventListener('click', () => {
            const id = el.dataset.template;
            options.onPickTemplate?.(id);
            close(true);
        });
    });

    // Recent cards
    root.querySelectorAll('[data-recent]').forEach((el) => {
        el.addEventListener('click', () => {
            const idx = Number(el.dataset.recent);
            const recent = options.recent || [];
            const item = recent[idx];
            if (item) {
                options.onPickRecent?.(item);
                close(true);
            }
        });
    });

    // Esc closes
    document.addEventListener('keydown', (e) => {
        if (open && e.key === 'Escape') {
            e.preventDefault();
            close(true);
        }
    });

    // ── API ────────────────────────────────────────────────────────────────
    function show() {
        open = true;
        root.classList.remove('is-hidden');
    }
    function close(persist = false) {
        open = false;
        root.classList.add('is-hidden');
        if (persist) {
            try { localStorage.setItem(STORAGE_KEY, String(Date.now())); } catch (_) { /* no-op */ }
        }
    }
    function isOpen() {
        return open;
    }

    if (options.autoShow !== false && fresh) {
        show();
    }

    return { open: show, close, isOpen };
}

// ── Templates ─────────────────────────────────────────────────────────────

/**
 * @type {Array<{ id: WelcomeTemplateId, label: string, build: () => string }>}
 */
const DEFAULT_TEMPLATES = [
    { id: 'blank',   label: 'Blank',   build: buildBlankThumb },
    { id: 'resume',  label: 'Resume',  build: buildResumeThumb },
    { id: 'letter',  label: 'Letter',  build: buildLetterThumb },
    { id: 'report',  label: 'Report',  build: buildReportThumb },
    { id: 'invoice', label: 'Invoice', build: buildInvoiceThumb },
];

function buildShell(version, templates, recent) {
    const templatesHtml = templates.map((t) => `
        <button class="welcome-template" data-template="${t.id}" title="Create ${t.label}">
            <div class="welcome-template-thumb">${t.build()}</div>
            <span class="welcome-template-label">${t.label}</span>
        </button>
    `).join('');

    let recentHtml;
    if (recent.length === 0) {
        recentHtml = `<div class="welcome-empty-recent">No recent documents — pick a template above to start writing.</div>`;
    } else {
        recentHtml = recent.map((r, i) => `
            <button class="welcome-recent-card" data-recent="${i}">
                <div class="welcome-recent-thumb">
                    <div class="welcome-recent-thumb-inner">
                        <div class="welcome-thumb-line thick dark" style="width: 35%;"></div>
                        <div class="welcome-thumb-line thin" style="width: 90%;"></div>
                        <div class="welcome-thumb-line thin" style="width: 80%;"></div>
                        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
                        <div class="welcome-thumb-line thin" style="width: 70%;"></div>
                        <div class="welcome-thumb-line thin" style="width: 95%;"></div>
                    </div>
                </div>
                <div class="welcome-recent-meta">
                    <span class="ms">description</span>
                    <div>
                        <div class="welcome-recent-name">${escapeHtml(r.name)}</div>
                        <div class="welcome-recent-time">${escapeHtml(r.subtitle || '')}</div>
                    </div>
                </div>
            </button>
        `).join('');
    }

    return `
        <header class="welcome-topbar">
            <div class="welcome-brand">
                <span class="welcome-brand-logo ms">edit_document</span>
                <span>Vela Docs ${escapeHtml(version)}</span>
                <span class="welcome-version">Playground</span>
            </div>
            <div class="welcome-actions">
                <button data-action="theme" title="Toggle dark mode">
                    <span class="ms">light_mode</span>
                </button>
                <button data-action="skip" class="welcome-skip" title="Skip welcome (Esc)">
                    Start writing <span class="ms">arrow_forward</span>
                </button>
            </div>
        </header>
        <main class="welcome-main">
            <section class="welcome-hero">
                <h1>A modern word processor that speaks your language.</h1>
                <p>Create, edit, and collaborate with seamless precision.
                   Vela Docs is designed to keep your focus entirely on the words.</p>
            </section>
            <section class="welcome-section">
                <h2>Start a new document</h2>
                <div class="welcome-templates">${templatesHtml}</div>
            </section>
            <section class="welcome-section welcome-recent">
                <h2>Recent documents</h2>
                <div class="welcome-recent-grid">${recentHtml}</div>
            </section>
        </main>
    `;
}

// ── Thumbnails ────────────────────────────────────────────────────────────

function buildBlankThumb() {
    return `<div class="welcome-thumb-add ms">add</div>`;
}

function buildResumeThumb() {
    return `
        <div class="welcome-thumb-line thick dark" style="width: 45%;"></div>
        <div class="welcome-thumb-line thin" style="width: 80%;"></div>
        <div class="welcome-thumb-line thin" style="width: 95%;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 85%;"></div>
        <div class="welcome-thumb-line thick dark" style="width: 32%; margin-top: 4px;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 92%;"></div>
    `;
}

function buildLetterThumb() {
    return `
        <div class="welcome-thumb-line med dark" style="width: 30%; align-self: flex-end;"></div>
        <div style="height: 6px;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 92%;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 88%;"></div>
        <div style="flex: 1;"></div>
        <div class="welcome-thumb-line thick dark" style="width: 30%;"></div>
    `;
}

function buildReportThumb() {
    return `
        <div class="welcome-thumb-line thick dark" style="width: 60%;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 95%;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thick dark" style="width: 45%; margin-top: 4px;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
        <div class="welcome-thumb-line thin" style="width: 80%;"></div>
        <div class="welcome-thumb-line thin" style="width: 100%;"></div>
    `;
}

function buildInvoiceThumb() {
    return `
        <div style="display: flex; justify-content: space-between;">
            <div class="welcome-thumb-line thick dark" style="width: 30%;"></div>
            <div class="welcome-thumb-line med dark" style="width: 22%;"></div>
        </div>
        <div style="height: 4px; border-bottom: 1px solid currentColor; opacity: 0.25;"></div>
        <div style="display: flex; justify-content: space-between;">
            <div class="welcome-thumb-line thin" style="width: 55%;"></div>
            <div class="welcome-thumb-line thin" style="width: 18%;"></div>
        </div>
        <div style="display: flex; justify-content: space-between;">
            <div class="welcome-thumb-line thin" style="width: 50%;"></div>
            <div class="welcome-thumb-line thin" style="width: 18%;"></div>
        </div>
        <div style="display: flex; justify-content: space-between;">
            <div class="welcome-thumb-line thin" style="width: 55%;"></div>
            <div class="welcome-thumb-line thin" style="width: 18%;"></div>
        </div>
        <div style="flex: 1;"></div>
        <div style="display: flex; justify-content: flex-end;">
            <div class="welcome-thumb-line thick dark" style="width: 30%;"></div>
        </div>
    `;
}

// ── Utils ──────────────────────────────────────────────────────────────────

function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, (c) => ({
        '&': '&', '<': '<', '>': '>', '"': '"', "'": '&#39;',
    }[c]));
}
