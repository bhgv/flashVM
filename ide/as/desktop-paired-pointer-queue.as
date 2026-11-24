// Copyright (c) 2026 bhgv
// EN: Propose a pointer target from the committed MovieClip tree and shaped TextField.
// RU: Предложить цель указателя по показанным MovieClip и раскладке TextField.


// EN: Encode the pointer proposal after the hit test.
// RU: Закодировать запрос указателя после проверки попадания.
function queueDesktopPointer() {
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
