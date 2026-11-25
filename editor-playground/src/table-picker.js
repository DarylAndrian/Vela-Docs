// ════════════════════════════════════════════════════════════════════════════
//  table-picker.js — Vela Docs Editor v0.4.2
//
//  Google Docs-style table grid picker. Hovering the Insert Table button
//  reveals a 5×5 grid; the highlighted region shows rows × columns. The
//  default size (3×3) is shown highlighted until the user moves the mouse.
//
//  Click to insert a table with that size and switch to the Table ribbon tab.
//  Auto-detects cursor context: inserts after the current block if the
//  cursor is in the document; opens a 1×1 default if it can't figure it out.
//
//  Lives in editor-playground/src/ (mirrored in src/editor/ for production).
// ════════════════════════════════════════════════════════════════════════════

const DEFAULT_ROWS = 3;
const DEFAULT_COLS = 3;
const MAX_ROWS = 5;
const MAX_COLS = 5;

export function mountTableGridPicker({ button, popover, grid, label, onInsert }) {
    let rows = DEFAULT_ROWS;
    let cols = DEFAULT_COLS;
    let keyboardOpen = false;
    button.setAttribute('aria-expanded', 'false');
    button.setAttribute('aria-controls', popover.id);
    label.setAttribute('aria-live', 'polite');

    // Build the 5×5 grid
    grid.innerHTML = '';
    for (let r = 0; r < MAX_ROWS; r++) {
        for (let c = 0; c < MAX_COLS; c++) {
            const cell = document.createElement('button');
            cell.type = 'button';
            cell.tabIndex = -1;
            cell.setAttribute('aria-label', `${r + 1} rows by ${c + 1} columns`);
            cell.className = 'table-grid-cell';
            if (r === DEFAULT_ROWS - 1 && c === DEFAULT_COLS - 1) {
                cell.classList.add('is-default');
            }
            cell.dataset.row = r;
            cell.dataset.col = c;
            grid.appendChild(cell);
        }
    }

    const updateHighlight = () => {
        grid.querySelectorAll('.table-grid-cell').forEach((cell) => {
            const r = parseInt(cell.dataset.row, 10);
            const c = parseInt(cell.dataset.col, 10);
            const inRange = r <= rows - 1 && c <= cols - 1;
            cell.classList.toggle('is-selected', inRange);
        });
        label.textContent = `${rows} × ${cols}`;
    };

    updateHighlight();

    const show = () => {
        popover.hidden = false;
        const rect = button.getBoundingClientRect();
        popover.style.left = `${Math.max(0, Math.min(rect.left, window.innerWidth - popover.offsetWidth))}px`;
        popover.style.top = `${rect.bottom + 2}px`;
        button.setAttribute('aria-expanded', 'true');
    };
    const hide = () => {
        keyboardOpen = false;
        popover.hidden = true;
        button.setAttribute('aria-expanded', 'false');
    };
    const focusCell = () => grid.children[(rows - 1) * MAX_COLS + cols - 1].focus();

    // Hover over a cell — expand the selection
    grid.addEventListener('mouseover', (e) => {
        if (keyboardOpen) return;
        const cell = e.target.closest('.table-grid-cell');
        if (!cell) return;
        rows = parseInt(cell.dataset.row, 10) + 1;
        cols = parseInt(cell.dataset.col, 10) + 1;
        updateHighlight();
    });

    // Click — insert
    grid.addEventListener('click', (e) => {
        const cell = e.target.closest('.table-grid-cell');
        if (!cell) return;
        const r = parseInt(cell.dataset.row, 10) + 1;
        const c = parseInt(cell.dataset.col, 10) + 1;
        hide();
        onInsert({ rows: r, cols: c });
    });

    grid.addEventListener('keydown', (e) => {
        switch (e.key) {
            case 'ArrowUp': rows = Math.max(1, rows - 1); break;
            case 'ArrowDown': rows = Math.min(MAX_ROWS, rows + 1); break;
            case 'ArrowLeft': cols = Math.max(1, cols - 1); break;
            case 'ArrowRight': cols = Math.min(MAX_COLS, cols + 1); break;
            case 'Escape': hide(); button.focus(); e.stopPropagation(); return;
            default: return;
        }
        e.preventDefault();
        updateHighlight();
        focusCell();
    });
    grid.addEventListener('focusout', (e) => {
        if (keyboardOpen && !grid.contains(e.relatedTarget)) hide();
    });

    // Show/hide the popover
    let hoverTimeout;
    button.addEventListener('mouseenter', () => {
        hoverTimeout = setTimeout(show, 80);
    });
    button.addEventListener('mouseleave', () => {
        clearTimeout(hoverTimeout);
        hoverTimeout = setTimeout(() => {
            // Only hide if we're not over the popover
            if (!keyboardOpen && !popover.matches(':hover')) hide();
        }, 120);
    });
    popover.addEventListener('mouseenter', () => clearTimeout(hoverTimeout));
    popover.addEventListener('mouseleave', () => {
        hoverTimeout = setTimeout(() => { if (!keyboardOpen) hide(); }, 100);
    });

    button.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        keyboardOpen = true;
        show();
        focusCell();
    });
}
