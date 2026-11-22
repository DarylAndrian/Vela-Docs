// ════════════════════════════════════════════════════════════════════════════
//  style-manager.js — v0.4.0
//
//  In-memory style map for paragraphs. Built-in styles are seeded once at
//  startup; custom styles are registered by the user and persisted via
//  localStorage (web) and the EditorBridge (production).
//
//  A style is { id, name, basedOn, type: 'paragraph'|'inline', font,
//  size, bold, italic, underline, color, alignment, spacingBefore,
//  spacingAfter, builtin: boolean, outlineLevel?: number }.
//
//  Usage:
//      import { StyleManager } from './style-manager.js';
//      StyleManager.seedBuiltinStyles();
//      StyleManager.register({ id: 'Heading6', name: 'Heading 6', ... });
//      StyleManager.updateToMatch('Heading1', node);   // capture from a node
//      StyleManager.apply(editor, 'Heading1');
//      StyleManager.toDocx();                          // serialize
// ════════════════════════════════════════════════════════════════════════════

const STORAGE_KEY = 'vela.customStyles.v1';

const BUILTIN_STYLES = [
    {
        id: 'Normal', name: 'Normal Text', type: 'paragraph', basedOn: null,
        font: 'Source Serif 4', size: 12, bold: false, italic: false, underline: false,
        color: '#222222', alignment: 'left', spacingBefore: 0, spacingAfter: 6,
        builtin: true,
    },
    {
        id: 'Title', name: 'Title', type: 'paragraph', basedOn: 'Normal',
        font: 'Source Serif 4', size: 26, bold: true, italic: false, underline: false,
        color: '#3525CD', alignment: 'center', spacingBefore: 0, spacingAfter: 12,
        builtin: true, outlineLevel: 0,
    },
    {
        id: 'Subtitle', name: 'Subtitle', type: 'paragraph', basedOn: 'Normal',
        font: 'Source Serif 4', size: 16, bold: false, italic: true, underline: false,
        color: '#464555', alignment: 'center', spacingBefore: 0, spacingAfter: 24,
        builtin: true,
    },
    {
        id: 'Heading1', name: 'Heading 1', type: 'paragraph', basedOn: 'Normal',
        font: 'Inter', size: 22, bold: true, italic: false, underline: false,
        color: '#3525CD', alignment: 'left', spacingBefore: 24, spacingAfter: 6,
        builtin: true, outlineLevel: 0,
    },
    {
        id: 'Heading2', name: 'Heading 2', type: 'paragraph', basedOn: 'Normal',
        font: 'Inter', size: 18, bold: true, italic: false, underline: false,
        color: '#464555', alignment: 'left', spacingBefore: 18, spacingAfter: 4,
        builtin: true, outlineLevel: 1,
    },
    {
        id: 'Heading3', name: 'Heading 3', type: 'paragraph', basedOn: 'Normal',
        font: 'Inter', size: 16, bold: true, italic: false, underline: false,
        color: '#222222', alignment: 'left', spacingBefore: 12, spacingAfter: 3,
        builtin: true, outlineLevel: 2,
    },
    {
        id: 'Heading4', name: 'Heading 4', type: 'paragraph', basedOn: 'Normal',
        font: 'Inter', size: 14, bold: true, italic: false, underline: false,
        color: '#222222', alignment: 'left', spacingBefore: 10, spacingAfter: 2,
        builtin: true, outlineLevel: 3,
    },
    {
        id: 'Heading5', name: 'Heading 5', type: 'paragraph', basedOn: 'Normal',
        font: 'Inter', size: 12, bold: true, italic: false, underline: false,
        color: '#222222', alignment: 'left', spacingBefore: 8, spacingAfter: 2,
        textTransform: 'uppercase', letterSpacing: '0.04em',
        builtin: true, outlineLevel: 4,
    },
    {
        id: 'Quote', name: 'Quote', type: 'paragraph', basedOn: 'Normal',
        font: 'Source Serif 4', size: 13, bold: false, italic: true, underline: false,
        color: '#464555', alignment: 'left', spacingBefore: 8, spacingAfter: 8,
        indentLeft: 24,
        builtin: true,
    },
    {
        id: 'Code', name: 'Code', type: 'paragraph', basedOn: 'Normal',
        font: 'JetBrains Mono', size: 12, bold: false, italic: false, underline: false,
        color: '#222222', alignment: 'left', spacingBefore: 4, spacingAfter: 4,
        background: '#F4F4F4',
        builtin: true,
    },
    {
        id: 'ListParagraph', name: 'List Paragraph', type: 'paragraph', basedOn: 'Normal',
        font: 'Source Serif 4', size: 12, bold: false, italic: false, underline: false,
        color: '#222222', alignment: 'left', spacingBefore: 0, spacingAfter: 2,
        builtin: true,
    },
];

const state = {
    /** id → style def */
    map: new Map(),
};

export const StyleManager = {
    seedBuiltinStyles() {
        BUILTIN_STYLES.forEach((s) => {
            if (!state.map.has(s.id)) state.map.set(s.id, { ...s });
        });
        this._loadCustomStyles();
    },

    register(styleDef) {
        if (!styleDef?.id) throw new Error('StyleManager.register: styleDef.id required');
        const existing = state.map.get(styleDef.id);
        const merged = { ...(existing || {}), ...styleDef };
        if (existing?.builtin) merged.builtin = true;
        state.map.set(styleDef.id, merged);
        this._saveCustomStyles();
        return merged;
    },

    unregister(id) {
        const existing = state.map.get(id);
        if (!existing || existing.builtin) return false;
        state.map.delete(id);
        this._saveCustomStyles();
        return true;
    },

    get(id) {
        const s = state.map.get(id);
        return s ? { ...s } : null;
    },

    list() {
        const builtin = BUILTIN_STYLES.map((s) => state.map.get(s.id)).filter(Boolean);
        const custom = [...state.map.values()]
            .filter((s) => !s.builtin)
            .sort((a, b) => a.name.localeCompare(b.name));
        return [...builtin, ...custom];
    },

    currentStyleId(editor) {
        if (!editor) return 'Normal';
        if (editor.isActive('title')) return 'Title';
        if (editor.isActive('subtitle')) return 'Subtitle';
        for (let lvl = 1; lvl <= 6; lvl++) {
            if (editor.isActive('heading', { level: lvl })) return 'Heading' + lvl;
        }
        if (editor.isActive('blockquote')) return 'Quote';
        if (editor.isActive('codeBlock')) return 'Code';
        const { $from } = editor.state.selection;
        for (let depth = $from.depth; depth >= 0; depth--) {
            const node = $from.node(depth);
            if (node.type.name === 'paragraph' || node.type.name === 'heading') {
                const sid = node.attrs?.styleId;
                if (sid && state.map.has(sid)) return sid;
            }
        }
        return 'Normal';
    },

    apply(editor, id) {
        if (!editor) return;
        const chain = editor.chain().focus();
        switch (id) {
            case 'Normal':         chain.setParagraph().run(); break;
            case 'Title':          chain.setParagraph().setNode('title').run(); break;
            case 'Subtitle':       chain.setParagraph().setNode('subtitle').run(); break;
            case 'Quote':          chain.setParagraph().setBlockquote().run(); break;
            case 'Code':           chain.setParagraph().setCodeBlock().run(); break;
            default:
                if (id.startsWith('Heading') && /^Heading\d+$/.test(id)) {
                    const lvl = parseInt(id.slice(7), 10);
                    chain.toggleHeading({ level: lvl }).run();
                } else if (state.map.has(id)) {
                    chain.setParagraph().updateAttributes('paragraph', { styleId: id }).run();
                }
        }
    },

    updateToMatch(id, node) {
        if (!id || !node) return false;
        const existing = state.map.get(id);
        if (!existing) return false;

        const textNode = findFirstText(node);
        const marks = textNode?.marks || [];
        const attrs = node.attrs || {};

        const captured = {
            font:        textNode?.marks?.find((m) => m.type === 'textStyle')?.attrs?.fontFamily || existing.font,
            size:        parseFloat(textNode?.marks?.find((m) => m.type === 'textStyle')?.attrs?.fontSize) || existing.size,
            bold:        marks.some((m) => m.type === 'bold'),
            italic:      marks.some((m) => m.type === 'italic'),
            underline:   marks.some((m) => m.type === 'underline'),
            color:       marks.find((m) => m.type === 'textStyle')?.attrs?.color || existing.color,
            alignment:   attrs.textAlign || existing.alignment,
        };

        const merged = { ...existing, ...captured };
        state.map.set(id, merged);
        this._saveCustomStyles();
        return true;
    },

    serialize() {
        return [...state.map.values()].map((s) => ({ ...s }));
    },

    toDocx() {
        const out = [];
        for (const s of state.map.values()) {
            out.push(styleToDocxXml(s));
        }
        return out.join('\n');
    },

    fromDocx(fragments) {
        const parser = new DOMParser();
        fragments.forEach((xml) => {
            try {
                const doc = parser.parseFromString(xml, 'application/xml');
                const styleEl = doc.querySelector('style');
                if (!styleEl) return;
                const def = parseDocxStyle(styleEl);
                if (def) state.map.set(def.id, def);
            } catch (_) {
                /* ignore malformed */
            }
        });
    },

    _saveCustomStyles() {
        const custom = [...state.map.values()].filter((s) => !s.builtin);
        try {
            localStorage.setItem(STORAGE_KEY, JSON.stringify(custom));
        } catch (_) {
            /* ignore */
        }
        if (window.vela?.setCustomStyles) {
            try {
                window.vela.setCustomStyles(JSON.stringify(custom));
            } catch (_) {
                /* ignore */
            }
        }
    },

    _loadCustomStyles() {
        let raw = null;
        try {
            raw = localStorage.getItem(STORAGE_KEY);
        } catch (_) {
            /* ignore */
        }
        if (!raw && window.vela?.getCustomStyles) {
            try {
                raw = window.vela.getCustomStyles();
            } catch (_) {
                /* ignore */
            }
        }
        if (!raw) return;
        try {
            const list = JSON.parse(raw);
            if (Array.isArray(list)) {
                list.forEach((s) => {
                    if (s?.id) state.map.set(s.id, { ...s, builtin: false });
                });
            }
        } catch (_) {
            /* ignore */
        }
    },
};

function findFirstText(node) {
    if (!node) return null;
    if (node.isText || node.type === 'text') return node;
    if (node.content) {
        for (const child of node.content) {
            const t = findFirstText(child);
            if (t) return t;
        }
    }
    return null;
}

function styleToDocxXml(s) {
    const isCustom = !s.builtin;
    const basedOn = s.basedOn ? '<w:basedOn w:val="' + escapeXml(s.basedOn) + '"/>' : '';
    const next = s.basedOn ? '<w:next w:val="' + escapeXml(s.basedOn) + '"/>' : '';

    const pPrParts = [];
    if (s.outlineLevel !== undefined) {
        pPrParts.push('<w:outlineLvl w:val="' + s.outlineLevel + '"/>');
    }
    if (s.spacingBefore !== undefined || s.spacingAfter !== undefined) {
        const before = Math.round((s.spacingBefore || 0) * 20);
        const after  = Math.round((s.spacingAfter || 0) * 20);
        pPrParts.push('<w:spacing w:before="' + before + '" w:after="' + after + '"/>');
    }
    if (s.alignment && s.alignment !== 'left') {
        pPrParts.push('<w:jc w:val="' + s.alignment + '"/>');
    }
    if (s.indentLeft) {
        pPrParts.push('<w:ind w:left="' + Math.round(s.indentLeft * 20) + '"/>');
    }
    const pPr = pPrParts.length ? '<w:pPr>' + pPrParts.join('') + '</w:pPr>' : '';

    const rPrParts = [];
    if (s.font) {
        rPrParts.push(
            '<w:rFonts w:ascii="' + escapeXml(s.font) + '" w:hAnsi="' + escapeXml(s.font) + '"/>',
        );
    }
    if (s.bold)     rPrParts.push('<w:b/>');
    if (s.italic)   rPrParts.push('<w:i/>');
    if (s.underline) rPrParts.push('<w:u w:val="single"/>');
    if (s.color)    rPrParts.push('<w:color w:val="' + stripHash(s.color) + '"/>');
    if (s.size)     rPrParts.push('<w:sz w:val="' + Math.round(s.size * 2) + '"/>');
    if (s.textTransform === 'uppercase') rPrParts.push('<w:caps/>');
    if (s.background) rPrParts.push('<w:shd w:val="clear" w:color="auto" w:fill="' + stripHash(s.background) + '"/>');
    const rPr = rPrParts.length ? '<w:rPr>' + rPrParts.join('') + '</w:rPr>' : '';

    return '<w:style w:type="' + (s.type || 'paragraph') + '"' +
        (isCustom ? ' w:customStyle="1"' : '') +
        ' w:styleId="' + escapeXml(s.id) + '">' +
        '<w:name w:val="' + escapeXml(s.name) + '"/>' +
        basedOn + next +
        pPr + rPr +
    '</w:style>';
}

function parseDocxStyle(el) {
    const id = el.getAttribute('w:styleId');
    const name = el.querySelector(':scope > w\\:name')?.getAttribute('w:val') || id;
    const basedOn = el.querySelector(':scope > w\\:basedOn')?.getAttribute('w:val') || null;
    const isCustom = el.getAttribute('w:customStyle') === '1';

    const pPr = el.querySelector(':scope > w\\:pPr');
    const rPr = el.querySelector(':scope > w\\:rPr');

    const def = {
        id,
        name,
        basedOn,
        builtin: !isCustom && BUILTIN_STYLES.some((b) => b.id === id),
        type: el.getAttribute('w:type') || 'paragraph',
    };

    if (pPr) {
        const spacing = pPr.querySelector(':scope > w\\:spacing');
        if (spacing) {
            def.spacingBefore = parseInt(spacing.getAttribute('w:before') || '0', 10) / 20;
            def.spacingAfter = parseInt(spacing.getAttribute('w:after') || '0', 10) / 20;
        }
        const jc = pPr.querySelector(':scope > w\\:jc');
        if (jc) def.alignment = jc.getAttribute('w:val');
        const outline = pPr.querySelector(':scope > w\\:outlineLvl');
        if (outline) def.outlineLevel = parseInt(outline.getAttribute('w:val') || '0', 10);
        const ind = pPr.querySelector(':scope > w\\:ind');
        if (ind) def.indentLeft = parseInt(ind.getAttribute('w:left') || '0', 10) / 20;
    }

    if (rPr) {
        const font = rPr.querySelector(':scope > w\\:rFonts');
        if (font) def.font = font.getAttribute('w:ascii') || font.getAttribute('w:hAnsi');
        if (rPr.querySelector(':scope > w\\:b'))      def.bold = true;
        if (rPr.querySelector(':scope > w\\:i'))      def.italic = true;
        if (rPr.querySelector(':scope > w\\:u'))      def.underline = true;
        const color = rPr.querySelector(':scope > w\\:color');
        if (color) def.color = '#' + color.getAttribute('w:val');
        const sz = rPr.querySelector(':scope > w\\:sz');
        if (sz) def.size = parseInt(sz.getAttribute('w:val') || '24', 10) / 2;
    }

    return def;
}

// Escape XML special characters. Uses String.fromCharCode() to build the
// entity prefixes so this function never contains a literal '&' in source —
// that way HTML-entity-decoding layers can't mangle it.
function escapeXml(s) {
    var AMP = String.fromCharCode(38);  // '&'
    return String(s)
        .split(AMP).join(AMP + 'amp;')
        .split('<').join(AMP + 'lt;')
        .split('>').join(AMP + 'gt;')
        .split('"').join(AMP + 'quot;')
        .split("'").join(AMP + 'apos;');
}

function stripHash(s) {
    return String(s || '').replace(/^#/, '').toUpperCase();
}