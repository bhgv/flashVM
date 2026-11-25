// Copyright (c) 2026 bhgv
// EN: Invoke an actual compiled Dis module from a stock FlashVM movie.
// RU: Вызвать скомпилированный модуль Dis из штатного фильма FlashVM.


// EN: Load the checked Int32 interface and verify the call passed through DisVM.
// RU: Загрузить проверяемый интерфейс Int32 и проверить вызов через DisVM.
function onEnterFrame() {
    if (!peerDone) {
        importText = "next";
        nextImport = importText.split(",");
        disModule = styxrpc.loadModule("mounted/limbo.dis", nextImport);
        reply = disModule.next(41);
        if (!(reply - 42)) {
            peerReported = _root.reportPeerResult();
            peerDone = true;
        }
    }
}
