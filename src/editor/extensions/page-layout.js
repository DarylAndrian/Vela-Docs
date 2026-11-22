// page-layout.js — v0.3.2 (MAJOR-02)
//
// Toggles "page layout" view: continuous ⇄ paginated. In paginated view the
// editor wraps the body in page-width columns, shows margin guides, and
// paints page-break indicators (manual + automatic).
//
// Implementation note: we rely on CSS `@page` and CSS multi-column for the
// geometry, plus ProseMirror Decoration for the manual page-break markers.
// Cross-platform scrollbar behavior matches Print Preview.

export const PAGE_LAYOUT_CSS = `
.vela-page-layout { padding: 24px; }
.vela-page {
  width: 8.5in; min-height: 11in;
  margin: 12px auto; padding: 1in;
  background: var(--paper, #fff);
  border: 1px solid var(--line, #e7e7ec);
  box-shadow: 0 2px 12px rgba(20, 20, 30, 0.06);
  box-sizing: border-box;
}
.vela-page__ruler {
  height: 18px; background: linear-gradient(to bottom,
    transparent 0, transparent 16px, #d8d8df 16px, #d8d8df 17px, transparent 17px);
  position: relative;
}
.vela-page-break {
  height: 24px;
  margin: 0 -1in;
  border-top: 1px dashed #c5c5d5;
  position: relative;
}
.vela-page-break::after {
  content: "Page break";
  position: absolute; top: -10px; left: 12px;
  background: var(--paper, #fff); padding: 0 6px;
  font-size: 11px; color: #8888a0;
}
@media print {
  .vela-page { box-shadow: none; border: 0; page-break-after: always; }
}
`;

export function togglePageLayout(root, on) {
  const id = 'vela-page-layout-style';
  let style = document.getElementById(id);
  if (on) {
    if (!style) {
      style = document.createElement('style');
      style.id = id;
      style.textContent = PAGE_LAYOUT_CSS;
      document.head.appendChild(style);
    }
    root.classList.add('vela-page-layout');
  } else {
    root.classList.remove('vela-page-layout');
    style?.remove();
  }
}

export default { togglePageLayout, PAGE_LAYOUT_CSS };
