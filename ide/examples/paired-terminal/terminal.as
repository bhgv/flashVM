// Copyright (c) 2026 bhgv
// EN: Apply portable view frames on timeline ticks and retain a reply until it is accepted.
// RU: Применять кадры представления при обновлении клипа, сохраняя ответ до приёма.

// EN: Root timeline callback. Limit work to one queued reply or one incoming view per tick.
// RU: Обработчик корневого клипа. За кадр обработать один ответ либо одно представление.
function onEnterFrame() {
    if (terminalPending) {
        terminalAccepted = styxTerminal.trySendFrame(terminalPending);
        if (terminalAccepted) {
            terminalPending = "";
        }
    } else {
        terminalFrame = styxTerminal.tryReceiveFrame();
        if (terminalFrame) {
            // The 53-character fvd1 header and one prefix character precede the offset.
            // Смещению предшествует 53-символьный заголовок fvd1 и один символ префикса.
            terminalX = terminalFrame.substr(54) * 1;
            _root._x = terminalX;
            terminalPending = terminalFrame + " AS2";
        }
    }
}
