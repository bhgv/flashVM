// Copyright (c) 2026 bhgv
// EN: Drain a released stroke without interleaving new desktop input.
// RU: Передать завершённый штрих без перемежения новым вводом.


// EN: Advance after a successful send. Wait for EndView after the last fragment.
// RU: Продвинуться после успешной отправки. После последней части ждать EndView.
function advanceDesktopInkSend() {
    desktopInkSent = desktopInkSent + desktopInkChunk;
    if (desktopInkCount - desktopInkSent) {
        desktopInkQueued = _root.queueDesktopInkSend();
    } else {
        desktopInkSending = false;
        desktopInkPoints = "";
    }
}
