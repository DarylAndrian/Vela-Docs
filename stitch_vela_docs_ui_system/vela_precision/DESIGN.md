---
name: Vela Precision
colors:
  surface: '#f9f9ff'
  surface-dim: '#d3daef'
  surface-bright: '#f9f9ff'
  surface-container-lowest: '#ffffff'
  surface-container-low: '#f1f3ff'
  surface-container: '#e9edff'
  surface-container-high: '#e1e8fd'
  surface-container-highest: '#dce2f7'
  on-surface: '#141b2b'
  on-surface-variant: '#464555'
  inverse-surface: '#293040'
  inverse-on-surface: '#edf0ff'
  outline: '#777587'
  outline-variant: '#c7c4d8'
  surface-tint: '#4d44e3'
  primary: '#3525cd'
  on-primary: '#ffffff'
  primary-container: '#4f46e5'
  on-primary-container: '#dad7ff'
  inverse-primary: '#c3c0ff'
  secondary: '#4953bc'
  on-secondary: '#ffffff'
  secondary-container: '#8792fe'
  on-secondary-container: '#17228f'
  tertiary: '#7e3000'
  on-tertiary: '#ffffff'
  tertiary-container: '#a44100'
  on-tertiary-container: '#ffd2be'
  error: '#ba1a1a'
  on-error: '#ffffff'
  error-container: '#ffdad6'
  on-error-container: '#93000a'
  primary-fixed: '#e2dfff'
  primary-fixed-dim: '#c3c0ff'
  on-primary-fixed: '#0f0069'
  on-primary-fixed-variant: '#3323cc'
  secondary-fixed: '#e0e0ff'
  secondary-fixed-dim: '#bdc2ff'
  on-secondary-fixed: '#000767'
  on-secondary-fixed-variant: '#2f3aa3'
  tertiary-fixed: '#ffdbcc'
  tertiary-fixed-dim: '#ffb695'
  on-tertiary-fixed: '#351000'
  on-tertiary-fixed-variant: '#7b2f00'
  background: '#f9f9ff'
  on-background: '#141b2b'
  surface-variant: '#dce2f7'
typography:
  display-editor:
    fontFamily: Source Serif 4
    fontSize: 48px
    fontWeight: '700'
    lineHeight: '1.2'
    letterSpacing: -0.02em
  h1-ui:
    fontFamily: Inter
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
    letterSpacing: -0.01em
  h2-ui:
    fontFamily: Inter
    fontSize: 18px
    fontWeight: '600'
    lineHeight: 24px
  body-editor:
    fontFamily: Source Serif 4
    fontSize: 18px
    fontWeight: '400'
    lineHeight: '1.6'
  body-ui:
    fontFamily: Inter
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  label-md:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '500'
    lineHeight: 16px
  label-sm:
    fontFamily: Inter
    fontSize: 11px
    fontWeight: '500'
    lineHeight: 14px
    letterSpacing: 0.02em
  mono:
    fontFamily: JetBrains Mono
    fontSize: 13px
    fontWeight: '400'
    lineHeight: 20px
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  base: 4px
  xs: 4px
  sm: 8px
  md: 12px
  lg: 16px
  xl: 24px
  2xl: 32px
  3xl: 48px
  4xl: 64px
  page-margin: min(10%, 96px)
  gutter: 16px
---

## Brand & Style
The design system for this application is built on the principles of **Refined Professionalism** and **Modern Minimalism**. It is specifically tailored for a high-performance native Windows word processor, where the interface must recede to prioritize the user's content.

The aesthetic is characterized by:
- **Quietude:** Minimal "chrome" and interface noise to reduce cognitive load.
- **Precision:** Tight alignment, subtle borders, and razor-sharp typography reminiscent of high-end developer tools.
- **Speed:** Visual cues that imply responsiveness, such as low-latency hover states and purposeful transitions.

Drawing inspiration from modern productivity software, the system utilizes a "Surface-on-Surface" architecture, where the document (the Page) is treated as a physical object resting upon a functional workbench (the App).

## Colors
The color palette is divided into two functional sets to support a native OS experience.

- **Primary Accent:** Used sparingly for focus states, primary actions, and active text selections. In dark mode, the accent shifts to a higher-vibrancy Indigo to maintain accessibility.
- **Neutral Hierarchy:** A strict scale of grays handles the structural integrity. The `app_bg` provides a soft contrast against the stark `page_bg`, creating a clear mental model of the "Canvas" versus the "Controls."
- **Semantic States:** 
  - **Hover:** 5% overlay of the primary accent or a slight lightening of neutral surfaces.
  - **Active:** 10% overlay for tactile feedback.
  - **Disabled:** 40% opacity on text and backgrounds.

## Typography
This design system employs a dual-font strategy to distinguish between work and management.

1.  **UI Elements (Inter):** Highly legible, functional, and systematic. Used for all menus, sidebars, and status bars.
2.  **Editor Content (Source Serif 4):** A sophisticated serif optimized for long-form reading. It provides the "writerly" feel necessary for a word processor.
3.  **Technical UI (JetBrains Mono):** Used for word counts, character counts, and developer-facing features (like Markdown source view).

Scale headlines down by 15% for mobile/compact views. For the editor body, maintain a 1.6x line height to ensure readability regardless of device.

## Layout & Spacing
The layout follows a **Fixed-Fluid Hybrid** model:
- **Application Shell:** Fixed-width sidebars (standard 240px) and a fluid central work area.
- **The Page:** A fixed-width container (e.g., 816px for US Letter equivalent) centered within the fluid work area.
- **Spacing Rhythm:** Based on a 4px grid. All padding and margins must be multiples of 4 to ensure visual alignment.

On mobile devices, the sidebars transform into a bottom sheet or a full-screen overlay, and the page margins collapse to a minimum of 16px.

## Elevation & Depth
Depth is signaled through a combination of **Tonal Layering** and **Subtle Ambient Shadows**. 

- **Level 0 (App Background):** The lowest layer. Neutral and flat.
- **Level 1 (Sidebars/Panels):** Separated from the background by 1px borders in the theme's border color. No shadow.
- **Level 2 (The Document Page):** Sits above the app background. Uses the "Card" shadow (0 2px 8px rgba(0,0,0,0.06)) to create a gentle lift.
- **Level 3 (Modals/Popovers):** Highest elevation. Uses the "Modal" shadow (0 12px 32px rgba(0,0,0,0.12)) to cast a significant depth shadow, effectively dimming the content beneath.

In Dark Mode, reduce shadow opacity by 50% and rely more on border contrast to define depth.

## Shapes
The shape language is "Soft-Geometric." It avoids fully rounded "pill" shapes to maintain a professional, desktop-first character.

- **Inputs:** Sharpest corners (4px) to imply precision and data entry.
- **Buttons:** Slightly softer (6px) to distinguish them from input fields.
- **Containers:** Progressively more rounded as they increase in size or importance (8px for cards, up to 16px for major UI panels).

## Components

### Buttons
- **Primary:** Background color `accent`, text `white`. 6px radius.
- **Secondary:** Transparent background, `border` stroke, `text_primary`. 
- **Ghost:** No background or border. Text `text_secondary`. Hover state shows a 5% neutral tint.

### Inputs & Text Fields
- **Default:** 1px border `border`, 4px radius.
- **Focus:** 1px border `accent` with a 2px soft outer glow of the accent color at 20% opacity.
- **Editor Area:** No borders or background; the focus is solely on the cursor and typography.

### Lists & Navigation
- **Sidebar Items:** 4px vertical padding, 8px horizontal. Hovering an item applies a subtle background tint (`#F1F1F4` in light, `#2A2A2A` in dark).
- **Active State:** A 2px vertical "pill" indicator of the `accent` color on the left edge of the list item.

### Cards & Panels
- **The Document:** White surface, 8px radius, subtle shadow.
- **Toolbars:** Fixed at the top of the editor or floating. If floating, use 12px radius and the "Card" shadow.

### Status Indicators
- **Save Status:** Uses `label-sm` in `text_muted`.
- **Active User/Cursor:** Small 2px vertical bar with a high-visibility version of the `accent` color.