# Vegetable Tactics (Axmol 3.0)

Rebuild of the original "Veg Soup" game. **All graphics are drawn with `ax::DrawNode` only** –
no sprites, textures or fonts (even the text uses a 5x7 pixel font made of DrawNode rectangles).
Only the original WAV files are reused (`Content/Audio`).

## Play
* Mouse down on a vegetable selects the group of touching vegetables of the same type; release over the group to
  remove it (3+ needed, multiplier x3..x6, 6+ = "Show off").
* **Add Veg** (Space) drops 3 more, **Shake** (S) un-jams the pile, **R** restarts, Esc quits.
* Vegetables also drop on their own (see the NEXT VEG timer). If the pile stays above the rim you lose a chance
  ("Pot out"). Clearing the goal levels up (more vegetable types, faster drops).

## Build
```bash
export AX_ROOT=/path/to/axmol      # Axmol 3.0 checkout
cd VegetableTactics
axmol build -p linux               # or: -p win32 / -p mac
```
(Standard Axmol 3.0 cpp template layout; code is in `Source/GameScene.*`.)
