// Copyright (c) 2026 bhgv
// EN: Check local aliases once, then wait for publication by the attached peer.
// RU: Один раз проверить локальные ссылки, затем дождаться публикации второй FlashVM.


// EN: Check write-through, forwarding and snapshots before the remote readiness exchange.
// RU: Проверить запись через ссылку, пересылку и снимки до обмена готовностью с партнёром.
function onEnterFrame() {
    if (!refChecked) {
        var counter = 41;
        var first = channel.create("ref", 2);
        var second = channel.create("ref", 2);
        var queued = first.send(ref counter);
        var alias = first.recv();
        alias = alias + 1;
        var snapshotQueued = second.send(alias);
        var referenceQueued = second.send(ref alias);
        var snapshot = second.recv();
        var forwarded = second.recv();
        forwarded = forwarded + 3;
        Math.refTestCell = 15;
        var propertyQueued = first.send(ref Math.refTestCell);
        var propertyAlias = first.recv();
        propertyAlias = propertyAlias + 2;
        var firstClosed = first.close();
        var secondClosed = second.close();
        alias = alias + 1;
        var values = counter + "," + alias + "," + snapshot + "," + forwarded + "," + propertyAlias + "," + Math.refTestCell;
        var sends = queued + "," + snapshotQueued + "," + referenceQueued + "," + propertyQueued;
        var proof = "[" + values + "|" + sends + "]";
        if (!proof.indexOf("[46,46,42,46,17,17|true,true,true,true]")) {
            refValuesMatch = true;
        }
        // The following frames must not bind the same names again.
        // Следующие кадры не должны повторно привязывать эти имена.
        refChecked = true;
    }
    if (!refDone) {
        if (refValuesMatch) {
            if (!refSawMissing) {
                var missing = vfs.stat("/ref-ready");
                var missingStatus = missing.join(",");
                if (!missingStatus.indexOf("1,")) {
                    refSawMissing = true;
                }
            } else {
                if (!refRequested) {
                    var gate = vfs.stat("/ref-release");
                    var gateStatus = gate.join(",");
                    if (!gateStatus.indexOf("0,")) {
                        var released = vfs.exec("/ref-release");
                        refRequested = true;
                    }
                } else {
                    var ready = vfs.stat("/ref-ready");
                    var readyStatus = ready.join(",");
                    if (!readyStatus.indexOf("0,")) {
                        var reported = _root.reportPeerResult();
                        refDone = true;
                    }
                }
            }
        }
    }
}
