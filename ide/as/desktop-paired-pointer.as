// Copyright (c) 2026 bhgv
// EN: Propose a pointer target from the committed MovieClip tree and shaped TextField.
// RU: Предложить цель указателя по показанным MovieClip и раскладке TextField.


// EN: Hit a visible child at the host's logical pixel and send the current Dis ticket.
// RU: Найти видимую панель в логических пикселях и отправить текущий билет Dis.
function sendDesktopPointer() {
    desktopPointerX = hostInput.get("x");
    desktopPointerY = hostInput.get("y");
    desktopPointerDepth = desktopFront.getChildDepthAtPoint(desktopPointerX, desktopPointerY);
    desktopPointerId = "00000000";
    desktopPointerOffset = 0 - 1;
    if (desktopPointerDepth) {
        desktopPointerId = desktopFrontOrder.substr((desktopPointerDepth - 1) * 8, 8);
        if (!(desktopPointerCode - 1)) {
            desktopPointerPane = desktopFront.getInstanceAtDepth(desktopPointerDepth);
            desktopPointerScalar = desktopPointerPane.getTextScalarAtPoint(2, desktopPointerX, desktopPointerY);
            if (desktopPointerScalar + 1) {
                desktopPointerCaption = desktopPointerPane.getInstanceAtDepth(1);
                if (desktopPointerCaption) {
                    desktopPointerOffset = desktopPointerScalar + desktopPointerCaption.getScalarCount() + 1;
                }
            }
        }
    }
    desktopWord = Math.floor(desktopPointerX);
    desktopEncoded = _root.encodeDesktopWord();
    desktopPointerXWord = desktopHexWord;
    desktopWord = Math.floor(desktopPointerY);
    desktopEncoded = _root.encodeDesktopWord();
    desktopPointerYWord = desktopHexWord;
    desktopWord = desktopPointerPhase;
    desktopEncoded = _root.encodeDesktopWord();
    desktopPointerPhaseWord = desktopHexWord;
    desktopWord = desktopPointerCode;
    desktopEncoded = _root.encodeDesktopWord();
    desktopPointerCodeWord = desktopHexWord;
    desktopWord = desktopPointerOffset;
    desktopEncoded = _root.encodeDesktopWord();
    desktopOutgoing = "fvd1:000000010000000100010024" + desktopFrontTicket + desktopPointerXWord
        + desktopPointerYWord + desktopPointerPhaseWord + desktopPointerCodeWord + desktopPointerId + desktopHexWord;
}
