// Copyright (c) 2026 bhgv
// EN: Publish a live cell and two typed callbacks on one FlashVM movie image.
// RU: Опубликовать живую ячейку и два типизированных колбэка в образе фильма.

// EN: Publish on the first owner frame, then accept and answer a UniBus credit.
// RU: Опубликовать при первом кадре и ответить на разрешение UniBus.
function onEnterFrame() {
    if (!peerPublished) {
        Math.peerCount = 41;
        peerDescriptorText = "magnitude,abs";
        peerDescriptor = peerDescriptorText.split(",");
        peerRealDescriptorText = "realMagnitude,abs";
        peerRealDescriptor = peerRealDescriptorText.split(",");
        peerRealLength = peerRealDescriptor.push(1147267322 + 2000000000, "(fs)", "(f)");
        peerGraphDescriptorText = "graphMagnitude,abs";
        peerGraphDescriptor = peerGraphDescriptorText.split(",");
        peerGraphLength = peerGraphDescriptor.push(663077686, "(fr[r[i]])", "(f)", "graph");
        peerCountDescriptorText = "count,peerCount,var,rw";
        peerCountDescriptor = peerCountDescriptorText.split(",");
        peerPublished = styxrpc.publishObject("peer.dis", Math, peerDescriptor, peerRealDescriptor, peerGraphDescriptor, peerCountDescriptor);
    }
    if (!peerSentFrame) {
        peerSentFrame = styxTerminal.trySendFrame("fvd1:012345678901234567890123456789012345678901234567");
    }
}
