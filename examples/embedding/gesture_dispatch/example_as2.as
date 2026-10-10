// ActionScript 2.0: DESIGN TARGET, NOT YET SUPPORTED by the runtime.
// Native gesture callbacks currently reach C only; AVM1 event dispatch
// requires MovieClip hit-testing and the Flash event bridge.
_root.onTap = function(fingers, x, y) { trace("tap " + fingers); };
_root.onMultiTap = function(fingers, x, y) { trace("multitap " + fingers); };
_root.onSwipe = function(fingers, direction, x, y) { trace("swipe " + direction); };
