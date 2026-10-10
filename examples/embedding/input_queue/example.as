// ActionScript 2.0 — DESIGN EXAMPLE, NOT YET EXECUTABLE BY FLASHVM.
// Future FlashVM touch events will be dispatched to MovieClip handlers.
_root.onMultiTap = function(fingers, x, y) {
    if (fingers == 2) trace("Two-finger tap at " + x + ", " + y);
};
_root.onSwipe = function(direction, fingers, x, y) {
    trace("Swipe: " + direction);
};
