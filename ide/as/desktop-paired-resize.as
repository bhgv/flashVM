// Copyright (c) 2026 bhgv
// EN: Propose a native window extent against the displayed scene ticket.
// RU: Предложить размер окна на основании билета показанного снимка.


// EN: Encode the already bounded native dimensions and preserve them for acknowledgement.
// RU: Закодировать допустимый размер окна и сохранить его до подтверждения.
function sendDesktopResize() {
    desktopWord = desktopResizeWidth;
    desktopResizeEncoded = _root.encodeDesktopWord();
    desktopResizeXWord = desktopHexWord;
    desktopWord = desktopResizeHeight;
    desktopResizeEncoded = _root.encodeDesktopWord();
    desktopOutgoing = "fvd1:000000010000000100010025" + desktopFrontTicket + desktopResizeXWord + desktopHexWord;
}
