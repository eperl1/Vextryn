# UI Design Audit & Visual Specification

## 1. Overall Theme & Palette
- **Surface Layers (Dark Graphite / Deep-Blue):** 
  - Base background: Deep blue-gray (`0xFF1E293B`).
  - Elevated/Sidebar panels: Dark graphite/deep-blue (`0xFF0F172A`).
- **Typography & Accents:**
  - Primary text: Off-white/slate (`0xFFF8FAFC`) for accessible contrast.
  - Muted text: Slate gray (`0xFF64748B`).
  - **Restrained Cyan/Teal Accent:** Replace blinding primary cyan (`0xFF00F0FF`) with a softer, restrained teal or muted cyan to provide a futuristic but refined look.

## 2. Component Design Principles
- **No Generic Giant Rectangles:** Replace solid blocky fills (like the current solid hover and button backgrounds) with refined geometries—such as thin borders, 1px accent lines, or subtle background tinting. 
- **Accessible Contrast:** Ensure interactive elements provide clear feedback without relying on drastic, jarring color flips. Highlight states should use slightly lighter slate (`0xFF334155`) or a translucent accent wash rather than solid neon fills.
- **No Rainbow Boxes:** Eliminate the multi-color theme picker with neon pink, green, red, and yellow in the Settings app. Use a restrained palette exclusively. High-alert colors (like the red Delete button in Files) should be muted or replaced with a restrained accent border.

## 3. Files App (`app_file_manager.hpp`)
- **Sidebar & Main Area:** Maintain the structural divide but improve separation (e.g., a 1px vertical border instead of relying solely on contrasting heavy fills).
- **Consistent Icon Geometry:** The current file icons (a cyan square inside a dark square) must be redesigned. Use consistent geometric outlines (e.g., a folder or document shape drawn with lines) instead of solid rectangles.
- **File List & Toolbar:** Shift from solid filled rectangles for the "New File", "Back", "Rename", and "Delete" buttons to wireframe buttons (a bounding box with a 1px stroke) or clean text with an underline accent.

## 4. Settings App (`app_settings.hpp`)
- **Toggles & Selectors:** The Mouse Sensitivity and Taskbar Density controls currently use giant solid rectangles. Redesign these as segmented control groups with thin outlines, where the active state is denoted by a subtle teal outline or a restrained inner fill, not a heavy neon block.
- **Preserve Taskbar Controls:** Keep the functional toggles for Compact/Big taskbar density, but apply the new restrained visual style to their representation. 
- **Theme Selection:** Remove the "rainbow boxes" entirely. If theme selection is retained, limit it to subtle variations of the restrained teal/cyan palette.
