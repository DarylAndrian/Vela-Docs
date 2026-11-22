// modals.js — v0.3.1
//
// Insert-Table and Insert-Image modals in the Vela Precision style. Lives in
// the editor bundle; called from the ribbon via `window.__openInsertTable`,
// `window.__openInsertImage`, `window.__openInsertLink`, etc.
//
// All modals follow the same pattern:
//   1. Build the DOM
//   2. Attach to a host element (an off-screen container)
//   3. Show + animate
//   4. Resolve via a Promise when the user clicks Insert / Cancel

const VELA_PALETTE = ['#3525cd', '#5656e3', '#a4a4f0', '#f0b400', '#e85f5c', '#3ab795'];

function buildOverlay() {
  const root = document.createElement('div');
  root.className = 'vela-modal-root';
  root.innerHTML = `
    <div class="vela-modal-backdrop"></div>
    <div class="vela-modal" role="dialog" aria-modal="true">
      <div class="vela-modal__header">
        <span class="vela-modal__title"></span>
        <button class="vela-modal__close" aria-label="Close">×</button>
      </div>
      <div class="vela-modal__body"></div>
      <div class="vela-modal__footer">
        <button class="vela-modal__btn vela-modal__btn--secondary" data-act="cancel">Cancel</button>
        <button class="vela-modal__btn vela-modal__btn--primary"   data-act="ok">Insert</button>
      </div>
    </div>
  `;
  document.body.appendChild(root);
  return root;
}

function hideModal(root) {
  root.classList.add('vela-modal-root--leaving');
  setTimeout(() => root.remove(), 180);
}

export function openInsertTable(defaults = {}) {
  return new Promise(resolve => {
    const root = buildOverlay();
    root.querySelector('.vela-modal__title').textContent = 'Insert table';
    const body = root.querySelector('.vela-modal__body');
    body.innerHTML = `
      <label class="vela-field">
        <span>Rows</span>
        <input type="number" name="rows" min="1" max="40" value="${defaults.rows ?? 3}">
      </label>
      <label class="vela-field">
        <span>Columns</span>
        <input type="number" name="cols" min="1" max="20" value="${defaults.cols ?? 3}">
      </label>
      <label class="vela-field vela-field--inline">
        <input type="checkbox" name="header" ${defaults.withHeaderRow !== false ? 'checked' : ''}>
        <span>First row is a header</span>
      </label>
    `;
    const close = (payload) => { hideModal(root); resolve(payload); };
    root.addEventListener('click', e => {
      if (e.target.matches('[data-act="cancel"], [data-act="ok"]')) {
        if (e.target.matches('[data-act="cancel"]')) close(null);
        else {
          const rows = parseInt(body.querySelector('[name="rows"]').value, 10);
          const cols = parseInt(body.querySelector('[name="cols"]').value, 10);
          const withHeaderRow = body.querySelector('[name="header"]').checked;
          close({ rows, cols, withHeaderRow });
        }
      }
    });
    root.querySelector('.vela-modal__close').onclick = () => close(null);
  });
}

export function openInsertImage(defaults = {}) {
  return new Promise(resolve => {
    const root = buildOverlay();
    root.querySelector('.vela-modal__title').textContent = 'Insert image';
    const body = root.querySelector('.vela-modal__body');
    body.innerHTML = `
      <label class="vela-field">
        <span>Image URL or data URI</span>
        <input type="text" name="url" placeholder="https://… or paste a file" value="${defaults.url ?? ''}">
      </label>
      <label class="vela-field">
        <span>Alt text (accessibility)</span>
        <input type="text" name="alt" placeholder="Describe the image" value="${defaults.alt ?? ''}">
      </label>
    `;
    const close = (payload) => { hideModal(root); resolve(payload); };
    root.addEventListener('click', e => {
      if (e.target.matches('[data-act="cancel"]')) close(null);
      if (e.target.matches('[data-act="ok"]')) {
        close({
          url: body.querySelector('[name="url"]').value.trim(),
          alt: body.querySelector('[name="alt"]').value.trim(),
        });
      }
    });
    root.querySelector('.vela-modal__close').onclick = () => close(null);
  });
}

export function openInsertLink(defaults = {}) {
  return new Promise(resolve => {
    const root = buildOverlay();
    root.querySelector('.vela-modal__title').textContent = 'Insert link';
    const body = root.querySelector('.vela-modal__body');
    body.innerHTML = `
      <label class="vela-field">
        <span>URL</span>
        <input type="url" name="href" placeholder="https://" value="${defaults.href ?? ''}">
      </label>
    `;
    const close = (payload) => { hideModal(root); resolve(payload); };
    root.addEventListener('click', e => {
      if (e.target.matches('[data-act="cancel"]')) close(null);
      if (e.target.matches('[data-act="ok"]')) {
        const href = body.querySelector('[name="href"]').value.trim();
        close(href ? { href } : null);
      }
    });
    root.querySelector('.vela-modal__close').onclick = () => close(null);
  });
}

// Export a small global namespace for the ribbon HTML.
if (typeof window !== 'undefined') {
  window.__vela_modals = { openInsertTable, openInsertImage, openInsertLink };
  window.VELA_PALETTE = VELA_PALETTE;
}
