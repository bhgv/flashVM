// Copyright (c) 2026 bhgv
// EN: Distinguish ordinary values from aliases across a local FIFO.
// RU: Различить обычные значения и привязки в локальной очереди.


// EN: Check write-through, re-forwarding, snapshots and a captured property receiver.
// RU: Проверить запись через ссылку, пересылку, снимок и захваченного владельца свойства.
function onEnterFrame() {
    if (!refDone) {
        var counter = 41;
        var first = channel.create("ref", 2);
        var second = channel.create("ref", 2);
        queued = first.send(ref counter);
        var alias = first.recv();
        alias = alias + 1;
        snapshotQueued = second.send(alias);
        referenceQueued = second.send(ref alias);
        var snapshot = second.recv();
        var forwarded = second.recv();
        forwarded = forwarded + 3;
        Math.refTestCell = 15;
        propertyQueued = first.send(ref Math.refTestCell);
        var propertyAlias = first.recv();
        propertyAlias = propertyAlias + 2;
        if (!(counter - 45)) {
            if (!(alias - 45)) {
                if (!(snapshot - 42)) {
                    if (!(propertyAlias - 17)) {
                        var provider = vfs.stat("/mounted/peer.dis");
                        if (!provider[0]) {
                            reported = _root.reportPeerResult();
                            refDone = true;
                        }
                    }
                }
            }
        }
    }
}
