// Copyright (c) 2026 bhgv
// EN: Import live module methods and exchange registered messages with a peer.
// RU: Импортировать методы живого модуля и обменяться сообщениями с другой FlashVM.


// EN: Verify connection, call exact exports and dispatch the registered UniBus reply.
// RU: Проверить соединение, вызвать точные экспорты и доставить ответ UniBus.
function onEnterFrame() {
    if (!peerReady) {
        peerFile = styxrpc.stat("mounted/peer.dis");
        peerStatus = peerFile.join(",");
        if (!peerStatus.indexOf("0,")) {
            peerIntImportText = "magnitude";
            peerIntImport = peerIntImportText.split(",");
            peerRealImportText = "realMagnitude,managed";
            peerRealImport = peerRealImportText.split(",");
            peerRealLength = peerRealImport.push(1147267322 + 2000000000, "(fs)", "(f)");
            peerGraphImportText = "graphMagnitude,graph";
            peerGraphImport = peerGraphImportText.split(",");
            peerGraphLength = peerGraphImport.push(663077686, "(fr[r[i]])", "(f)");
            peerModule = styxrpc.loadModule("/mounted/peer.dis", peerIntImport, peerRealImport, peerGraphImport);
            peerResult = peerModule.magnitude(0 - 12);
            peerReal = peerModule.realMagnitude(0 - 25 / 2, "signed");
            peerEmptyText = "";
            peerLeaf = peerEmptyText.split("");
            peerBox = peerEmptyText.split("");
            peerLeafLength = peerLeaf.push(43);
            peerBoxLength = peerBox.push(peerLeaf);
            peerGraph = peerModule.graphMagnitude(0 - 29 / 2, peerBox);
            peerImage = os.open("mounted/peer.dis");
            peerAutoModule = peerImage.load();
            peerImageClosed = peerImage.close();
            peerAutoGraph = peerAutoModule.graphMagnitude(0 - 31 / 2, peerBox);
            peerOldCall = styxrpc.callInt("mounted/peer.dis", "magnitude", 0 - 8);
            peerVar = styxrpc.intVariable("/mounted/peer.dis/.var/count", true);
            peerRefQueue = channel.create("ref", 1);
            peerRefSent = peerRefQueue.send(ref peerVar.value);
            peerAlias = peerRefQueue.recv();
            peerAlias = 77;
            peerReadBack = peerAlias;
            peerHidden = styxrpc.stat("mounted/peer.dis/.var/count");
            peerHiddenText = peerHidden.join(",");
            peerCreditText = "_inferno.credit";
            peerCredit = peerCreditText.split(",");
            peerCreditLength = peerCredit.push(1, 47, "onPeerCredit");
            peerFrameText = "_inferno.fvd1";
            peerFrame = peerFrameText.split(",");
            peerFrameLength = peerFrame.push(1, 0, "onPeerFrame");
            peerMessages = styxrpc.openMessages(".unibus", peerCredit, peerFrame);
            peerEmptyPayloadText = "";
            peerEmptyPayload = peerEmptyPayloadText.split("");
            peerQueued = peerMessages.send("_inferno.credit", 1, peerEmptyPayload);
            peerReady = true;
        }
    }
    if (peerReady) {
        if (!peerDone) {
            peerReceived = peerMessages.pump(_root);
            if (peerReceivedFrame) {
                if (peerReceivedCredit) {
                    peerResultText = "[" + peerResult + "," + peerReal + "," + peerGraph + "," + peerAutoGraph + "," + peerOldCall + "," + peerQueued + "," + peerReadBack + "]";
                    if (!peerHiddenText.indexOf("1,")) {
                        if (!peerResultText.indexOf("[12,12.5,14.5,15.5,8,0,77]")) {
                            peerClosed = peerMessages.close();
                            peerClosedAgain = peerMessages.close();
                            peerReported = _root.reportPeerResult();
                            peerDone = true;
                        }
                    }
                }
            }
        }
    }
}
