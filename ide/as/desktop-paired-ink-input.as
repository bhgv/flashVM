// Copyright (c) 2026 bhgv
// EN: Route valid Brush/Pen samples to the scene-local preview before the wire.
// RU: Передать точки кисти и пера в локальный предпросмотр перед отправкой.


// EN: Start on a committed vector page or route motion and release of its stroke.
// RU: Начать на показанной странице либо принять движение и отпускание её штриха.
function routeDesktopInk() {
    desktopInkInside = (desktopFrontVectorSlots.indexOf("|" + desktopPointerId + "|") + 1)
        * !Math.max(0, 0 - desktopPointerOffset);
    if (desktopInkClip) {
        desktopInkConsumed = true;
        desktopInkInside = desktopInkInside * !desktopInkSlot.indexOf(desktopPointerId);
        if (!(desktopPointerPhase - 2)) {
            if (desktopInkInside * Math.max(0, 4095 - desktopInkCount)) {
                if (desktopPointerOffset - desktopInkLastPoint) {
                    desktopInkSampled = _root.appendDesktopInkSample();
                    desktopInkPainted = _root.paintDesktopInkSample();
                }
            }
        } else {
            if (!(desktopPointerPhase - 3)) {
                desktopInkReleased = _root.releaseDesktopInk();
            }
        }
    } else {
        if (desktopInkInside * !(desktopPointerPhase - 1)) {
            desktopInkStyleAt = (("0x" + desktopPointerId) * 1 - 1) * 25;
            desktopInkStyle = desktopFrontInkStyles.substr(desktopInkStyleAt, 25);
            desktopInkTool = desktopInkStyle.substr(24, 1) * 1;
            if (!(desktopInkTool - 1) + !(desktopInkTool - 5)) {
                desktopInkStarted = _root.startDesktopInk();
            }
        }
    }
}
