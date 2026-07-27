# Session Report: C3 — Restore Visible "=" Button (5-Row Grid)

**Date:** 2026-07-27  
**Duration:** ~20 minutes  
**Session type:** Source + boot-stability verification  
**Verdict:** **C3 SOURCE PASS** — "=" button restored, 5×4 grid fits within window, all C2 features preserved

---

## Problem

C2 removed the mouse-clickable "=" button from the 4×4 grid (replaced by ".") because the grid had exactly 16 slots (10 digits + 4 operators + C + .), full with no room for "=". Mouse-only users lost the ability to evaluate a calculation with a familiar "=" click — they had to use keyboard "Enter"/"=" or rely on operator chain-evaluation.

## Solution

Expand the button grid from **4 rows × 4 columns** to **5 rows × 4 columns**, reducing button height from 60px to 48px and gap from 10px to 6px to keep the grid within the existing 390px window height.

## Layout Change

```
Before (C2, 4×4):              After (C3, 5×4):
  7  8  9  /                     7  8  9  /
  4  5  6  *                     4  5  6  *
  1  2  3  -                     1  2  3  -
  C  0  .  +                     C  0  .  +
                                  ·  ·  ·  =    ← NEW: '=' at bottom-right (col 3)
```

The "=" button sits at **row 4, column 3** (bottom-right, directly under the "+" operator) — the standard calculator layout. Columns 0-2 of row 4 are empty (not rendered), keeping a clean visual.

## Grid Height Calculation

| Metric | Value |
|--------|-------|
| Button height (bh) | 48px |
| Gap | 6px |
| Rows | 5 |
| Grid height | 5×48 + 4×6 = 264px |
| Grid start Y | w.y + 110 |
| Grid end Y | 110 + 264 = 374 |
| Window height | 390px |
| Bottom padding | 16px ✅ |

## File Changed

| File | Status | Final SHA256 |
|------|--------|-------------|
| `gui/compositor/apps/app_calculator.hpp` | **Modified** | `c3c803c75a4f5b101844d49d80f443d23f4c0be564638e28e87a268a6cc1658c` |

**Only 1 file changed** (per spec: all other files forbidden).

## Changes in Detail

The only section modified was the button grid rendering loop in `draw_app_calculator`. All other code (calc state, `calc_press`, `calc_handle_key`, `calc_clear`, `parse_scaled`, `format_scaled`, `rebuild_value`, display rendering) is **identical to C2**.

### Diff Summary

```diff
-    // ── Button grid (4×4) ───────────────────────────────────────────────
-    uint32_t bh = 60;
-    for (int r = 0; r < 4; r++) {
-        for (int c = 0; c < 4; c++) {
-            uint32_t cy = by + r * (bh + 10);
+    // ── Button grid (5×4) ───────────────────────────────────────────────
+    uint32_t bh = 48;
+    uint32_t gap = 6;
+    for (int r = 0; r < 5; r++) {
+        for (int c = 0; c < 4; c++) {
+            uint32_t cy = by + r * (bh + gap);
+            char key = 0;  // default: empty cell
             ...
+            } else if (r == 4) {
+                // Row 4: '=' at column 3 (rightmost, under '+'), empty elsewhere
+                if (c == 3) key = '=';
+            }
+            // Only draw and handle clicks for cells that have a key
+            if (key != 0) {
                 ...
+            }
```

Key changes:
1. `bh`: 60 → 48 (button height reduced)
2. Gap: hardcoded `10` → variable `uint32_t gap = 6`
3. Loop bound: `r < 4` → `r < 5`
4. Row 4 key assignment: `if (c == 3) key = '=';`
5. `key != 0` guard wraps draw/click logic to skip empty cells
6. All column/row layout for rows 0-3 is unchanged from C2

## Verification

### Build
```
MAKE_EXIT=0 — clean compile, no errors
```

### Build Artifact Consistency
```
cmp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf → IDENTICAL
(172,400 bytes each)
```

### Boot-Stability (headless QEMU)
```
QEMU_EXIT=0 (timeout)
COMP MARKS: MARK 1-6 all present (6/6)
Last frame: 1620
Crash indicators: none (CLEAN)
```

### Code Review
**code-reviewer-deepseek-flash** — reviewed 3 times this session, **PASS** each time.

Reviewer notes:
> *Layout-only change: 4×4→5×4 with smaller buttons, `=` at bottom-right. All state/arithmetic untouched from C2. Grid fits within window (5×48+4×6=264, ends at y=374 < 390). No issues.*

## Preserved from C2

All C2 features remain fully functional because the state/logic code is completely unchanged:

| C2 Feature | Status |
|------------|--------|
| Decimal entry via keyboard `.` | ✅ Unchanged in calc_handle_key |
| Decimal entry via mouse `.` button | ✅ Button still at row 3 col 2 |
| Double-decimal blocking | ✅ calc_has_dot flag untouched |
| Scaled int64_t arithmetic | ✅ calc_press identical |
| `parse_scaled` / `format_scaled` | ✅ Unchanged |
| Division by zero error | ✅ calc_err logic untouched |
| 'c' / Escape error recovery | ✅ calc_clear untouched |
| Integer fallback (5+3=) | ✅ Same calc_press path |
| Keyboard `Enter`/`=` evaluation | ✅ Same calc_press('=') path |

## As-Built Verification Checklist

| Requirement | Status | Notes |
|-------------|--------|-------|
| "=" button visible | ✅ | Row 4 col 3, bottom-right |
| "=" button clickable | ✅ | `if (key != 0)` guard allows clicks on '=' |
| "." button still works | ✅ | Row 3 col 2, identical to C2 |
| Layout doesn't clip/overlap | ✅ | Grid ends at y=374 < 390 window height |
| Keyboard Enter/=/./ still work | ✅ | calc_handle_key unchanged |
| Full mouse calc 1,2,.,5,*,3,-,4,= | ✅ | All buttons present at their C2 positions + new '=' |

## Final Verdict

**C3 SOURCE PASS** — The "=" button is restored as a visible, clickable mouse button at the bottom-right of a 5-row calculator grid. All C2 features (decimal entry, double-decimal blocking, floating-point arithmetic, error handling, integer fallback, keyboard input) are fully preserved. The grid fits cleanly within the existing 390px window with 16px of bottom padding.
