// Copyright (c) 2026 bhgv
// EN: Encode a bounded fragment of a released stroke for UniBus.
// RU: Закодировать ограниченный фрагмент завершённого штриха для UniBus.


// EN: Stage at most 192 points. The send callback advances only on credit.
// RU: Подготовить до 192 точек. Обработчик продвигается только при наличии кредита.
function queueDesktopInkSend() {
    desktopInkChunk = Math.min(192, desktopInkCount - desktopInkSent);
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
        + desktopInkSlot + desktopInkRevision + desktopInkOffsetWord + desktopHexWord
        + desktopInkPoints.substr(desktopInkSent * 5, desktopInkChunk * 5);
}
