// Copyright (c) 2026 bhgv
// EN: Wait for the movie image and call it from another FlashVM.
// RU: Дождаться образа фильма и вызвать его из другой FlashVM.


// EN: Check publication, then issue one exact signed application call.
// RU: Проверить публикацию, затем сделать один вызов со знаковым аргументом.
function onEnterFrame() {
    if (!peerDone) {
        peerFile = styxrpc.stat("mounted/peer.dis");
        peerStatus = peerFile.join(",");
        if (!peerStatus.indexOf("0,")) {
            peerDone = true;
            peerResult = styxrpc.callInt("mounted/peer.dis", "magnitude", 0 - 12);
            peerResultText = "[" + peerResult + "]";
            if (!peerResultText.indexOf("[12]")) {
                peerReported = _root.reportPeerResult();
            }
        }
    }
}
