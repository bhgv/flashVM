// Copyright (c) 2026 bhgv
// EN: Propose a pointer target from the committed MovieClip tree and shaped TextField.
// RU: Предложить цель указателя по показанным MovieClip и раскладке TextField.


// EN: Hit a visible child at the host's logical pixel and send the current Dis ticket.
// RU: Найти видимую панель в логических пикселях и отправить текущий билет Dis.
function sendDesktopPointer() {
    desktopPointerX = hostInput.get("x");
    desktopPointerY = hostInput.get("y");
    desktopPicked = _root.pickDesktopPane();
    desktopPointerOffset = 0 - 1;
    if (desktopPointerPane) {
        if (desktopFrontVectorSlots.indexOf("|" + desktopPointerId + "|") + 1) {
            desktopVectorPointMapped = _root.routeDesktopVectorPointer();
        } else {
            if (!(desktopPointerCode - 1)) {
                desktopPointerScalar = desktopPointerPane.getTextScalarAtPoint(2, desktopPointerX, desktopPointerY);
                if (desktopPointerScalar + 1) {
                    desktopPointerCaption = desktopPointerPane.getInstanceAtDepth(1);
                    if (desktopPointerCaption) {
                        desktopPointerOffset = desktopPointerScalar + desktopPointerCaption.getScalarCount() + 1;
                    }
                }
            }
        }
    }
    desktopInkConsumed = false;
    if (!(desktopPointerCode - 1)) {
        desktopInkRouted = _root.routeDesktopInk();
    }
    if (!desktopInkConsumed) {
        desktopPointerQueued = _root.queueDesktopPointer();
    }
}
