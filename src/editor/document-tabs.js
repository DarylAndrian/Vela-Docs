// ════════════════════════════════════════════════════════════════════════════
//  document-tabs.js — Vela Docs Editor v0.4.2
//
//  Google Docs-style "Document tabs" sidebar.
//
//  The sidebar slides in from the left of the editor when the user clicks
//  the document-tabs button (in the left rail / topbar). It shows all the
//  currently-open documents organized into groups (Ongoing / Queue / Done /
//  etc.), with a + button to create a new document. Each tab is switchable,
//  closable, and renameable (right-click or kebab menu).
//
//  In production (src/editor/), this is wired to EditorBridge::openDocument
//  / closeDocument. In the playground, we mock the document list with a
//  realistic sample so the feature can be demonstrated.
//
//  Lives in editor-playground/src/ (mirrored in src/editor/ for production).
// ════════════════════════════════════════════════════════════════════════════

const DEFAULT_DOCS = [
    {
        id: 'background',
        title: 'Background',
        group: 'none',
        initial: 'B',
    },
    {
        id: 'driver-app',
        title: 'Driver App Version',
        group: 'none',
        initial: 'D',
    },
    {
        id: 'merchant-app',
        title: 'Merchant App Application',
        group: 'none',
        initial: 'M',
    },
    {
        id: 'general-rule',
        title: 'General Rule & Variables',
        group: 'none',
        initial: 'G',
    },
    {
        id: '__divider-1__',
        type: 'divider',
    },
    {
        id: 'group-ongoing',
        type: 'group',
        title: '1 Ongoing Features',
    },
    {
        id: 'migration-react',
        title: 'Migration to React',
        group: 'ongoing',
        initial: 'M',
    },
    {
        id: 'rev-data-rework',
        title: 'REV - Data Rework v1.0',
        group: 'ongoing',
        initial: 'R',
    },
    {
        id: 'feature-self-reg',
        title: 'Feature - Self Registration',
        group: 'ongoing',
        initial: 'F',
    },
    {
        id: 'hf-uber',
        title: 'HF Uber Integration',
        group: 'ongoing',
        initial: 'H',
    },
    {
        id: '__divider-2__',
        type: 'divider',
    },
    {
        id: 'group-queue',
        type: 'group',
        title: '2 Queue Features',
    },
    {
        id: 'chat-review',
        title: 'Chat + Review',
        group: 'queue',
        initial: 'C',
    },
    {
        id: 'feature-summary',
        title: 'Feature - Summary',
        group: 'queue',
        initial: 'F',
    },
    {
        id: 'all-features-layout',
        title: 'All Features Final Layout',
        group: 'queue',
        initial: 'A',
    },
    {
        id: 'rework-layered',
        title: 'Rework - Multi-layered Lab…',
        group: 'queue',
        initial: 'R',
    },
    {
        id: 'foods-dashboard',
        title: 'Foods Merchant Dashboard',
        group: 'queue',
        initial: 'F',
    },
    {
        id: '__divider-3__',
        type: 'divider',
    },
];

export function mountDocumentTabs({ sidebar, list, header, addBtn, onSelect, onClose, onCreate }) {
    let activeId = 'background';
    let docs = [...DEFAULT_DOCS];

    const render = () => {
        list.innerHTML = '';
        docs.forEach((doc) => {
            if (doc.type === 'divider') {
                const divider = document.createElement('li');
                divider.className = 'doc-tab-divider';
                divider.textContent = '------------------------';
                list.appendChild(divider);
                return;
            }
            if (doc.type === 'group') {
                const li = document.createElement('li');
                li.className = 'doc-tab-group';
                li.textContent = doc.title;
                list.appendChild(li);
                return;
            }
            // Regular tab
            const li = document.createElement('li');
            li.className = 'doc-tab';
            if (doc.id === activeId) li.classList.add('is-active');
            li.dataset.id = doc.id;

            li.innerHTML = `
                <button class="doc-tab-icon" title="${doc.title}">
                    <span class="ms">description</span>
                </button>
                <span class="doc-tab-title">${doc.title}</span>
                <button class="doc-tab-kebab" title="More actions" aria-label="More actions">
                    <span class="ms">more_vert</span>
                </button>
                <button class="doc-tab-close" title="Close" aria-label="Close tab">
                    <span class="ms">close</span>
                </button>
            `;

            // Click on the body → switch
            li.querySelector('.doc-tab-icon').addEventListener('click', (e) => {
                e.stopPropagation();
                selectDoc(doc.id);
            });
            li.querySelector('.doc-tab-title').addEventListener('click', (e) => {
                e.stopPropagation();
                selectDoc(doc.id);
            });

            // Kebab menu → small popover (rename / move group / delete)
            li.querySelector('.doc-tab-kebab').addEventListener('click', (e) => {
                e.stopPropagation();
                openKebabMenu(doc, li);
            });

            // Close button
            li.querySelector('.doc-tab-close').addEventListener('click', (e) => {
                e.stopPropagation();
                closeDoc(doc.id);
            });

            list.appendChild(li);
        });
    };

    const selectDoc = (id) => {
        activeId = id;
        render();
        onSelect?.(id);
    };

    const closeDoc = (id) => {
        const idx = docs.findIndex((d) => d.id === id);
        if (idx < 0) return;
        const doc = docs[idx];
        docs.splice(idx, 1);
        // If closing the active, pick the next non-meta item
        if (id === activeId) {
            const next = docs.find((d) => !d.type);
            if (next) activeId = next.id;
        }
        render();
        onClose?.(doc);
    };

    const createDoc = () => {
        const newId = `doc-${Date.now()}`;
        const newDoc = {
            id: newId,
            title: 'Untitled document',
            group: 'none',
            initial: 'U',
        };
        docs.push(newDoc);
        activeId = newId;
        render();
        onCreate?.(newDoc);
    };

    const openKebabMenu = (doc, anchor) => {
        // Close any existing menu
        closeKebabMenu();
        const menu = document.createElement('div');
        menu.className = 'doc-tab-kebab-menu';
        menu.innerHTML = `
            <button data-action="rename"><span class="ms">edit</span> Rename</button>
            <button data-action="move-up"><span class="ms">arrow_upward</span> Move up</button>
            <button data-action="move-down"><span class="ms">arrow_downward</span> Move down</button>
            <button data-action="duplicate"><span class="ms">content_copy</span> Duplicate</button>
            <hr/>
            <button data-action="delete" class="danger"><span class="ms">delete</span> Close</button>
        `;
        const rect = anchor.getBoundingClientRect();
        menu.style.position = 'fixed';
        menu.style.top = `${rect.bottom + 4}px`;
        menu.style.left = `${rect.right - 160}px`;
        document.body.appendChild(menu);
        menu.dataset.docId = doc.id;
        menu.addEventListener('click', (e) => {
            const btn = e.target.closest('button[data-action]');
            if (!btn) return;
            const action = btn.dataset.action;
            handleKebabAction(doc.id, action);
            closeKebabMenu();
        });
        // Close on outside click
        setTimeout(() => {
            document.addEventListener('click', closeKebabMenu, { once: true });
        }, 0);
    };

    const closeKebabMenu = () => {
        document.querySelectorAll('.doc-tab-kebab-menu').forEach((m) => m.remove());
    };

    const handleKebabAction = (id, action) => {
        const idx = docs.findIndex((d) => d.id === id);
        if (idx < 0) return;
        const doc = docs[idx];
        if (action === 'rename') {
            const newName = window.prompt('Rename document', doc.title);
            if (newName && newName.trim()) {
                doc.title = newName.trim();
                doc.initial = newName.trim().charAt(0).toUpperCase();
                render();
            }
        } else if (action === 'move-up' && idx > 0) {
            [docs[idx - 1], docs[idx]] = [docs[idx], docs[idx - 1]];
            render();
        } else if (action === 'move-down' && idx < docs.length - 1) {
            [docs[idx + 1], docs[idx]] = [docs[idx], docs[idx + 1]];
            render();
        } else if (action === 'duplicate') {
            const dup = { ...doc, id: `${doc.id}-copy-${Date.now()}`, title: `${doc.title} (copy)` };
            docs.splice(idx + 1, 0, dup);
            render();
        } else if (action === 'delete') {
            closeDoc(id);
        }
    };

    addBtn?.addEventListener('click', createDoc);

    // Initial render
    render();

    return {
        open: () => { sidebar.hidden = false; },
        close: () => { sidebar.hidden = true; },
        toggle: () => { sidebar.hidden = !sidebar.hidden; },
        isOpen: () => !sidebar.hidden,
        selectDoc,
        createDoc,
    };
}