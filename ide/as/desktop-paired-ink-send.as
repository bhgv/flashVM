// Copyright (c) 2026 bhgv
// EN: Encode a bounded fragment of a released stroke for UniBus.
// RU: Закодировать ограниченный фрагмент завершённого штриха для UniBus.


// EN: Stage at most 87 timed points within the old byte budget. Advance on credit.
// RU: Подготовить до 87 точек со временем в прежнем лимите байтов. Продвигаться по кредиту.
function queueDesktopInkSend() {
    desktopInkChunk = Math.min(87, desktopInkCount - desktopInkSent);
    desktopWord = desktopInkDownX;
    desktopInkEncoded = _root.encodeDesktopWord();
    desktopInkXWord = desktopHexWord;
    desktopWord = desktopInkDownY;
    desktopInkEncoded = _root.encodeDesktopWord();
    desktopInkYWord = desktopHexWord;
    desktopWord = desktopInkSent;
    desktopInkEncoded = _root.encodeDesktopWord();
    desktopInkOffsetWord = desktopHexWord;
    desktopWord = desktopInkCount;
    desktopInkEncoded = _root.encodeDesktopWord();
    desktopOutgoing = "fvd1:00000001000000010001002a" + desktopInkTicket + desktopInkXWord + desktopInkYWord
        + desktopInkSlot + desktopInkRevision + desktopInkOffsetWord + desktopHexWord + "t"
        + desktopInkPoints.substr(desktopInkSent * 11, desktopInkChunk * 11);
}
