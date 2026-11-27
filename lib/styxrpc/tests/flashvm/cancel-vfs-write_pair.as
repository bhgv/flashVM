// Copyright (c) 2026 bhgv
// EN: Cancel a write while a read and the movie share the same session.
// RU: Отменить запись, пока чтение и фильм используют ту же сессию.


// EN: Dispatch the plan once, and signal each admission or completion to Limbo.
// RU: Запустить сценарий один раз и сообщать Limbo о допуске и завершении.
function onEnterFrame() {
    if (!slowWrite) {
        slowWrite = _root.writeRemoteFile("/slow-write", 17, 29);
        otherRead = _root.readRemoteFile("/other-data", 1);
    }
    if (!stopSent) {
        var gate = os.open("/write-gate");
        var permit = gate.read(1);
        var gateClosed = gate.close();
        if (!(permit[0] - 49)) {
            if (!neighbourDone) {
                var neighbour = os.open("/neighbour");
                var bytes = neighbour.read(1);
                var neighbourClosed = neighbour.close();
                if (!(bytes[0] - 90)) {
                    neighbourDone = vfs.mkdir("/write-before");
                }
            }
        }
        if (!(permit[0] - 50)) {
            if (neighbourDone) {
                var requested = slowWrite.cancel();
                stopSent = true;
            }
        }
    }
    if (stopSent) {
        if (!stopDone) {
            var stopped = slowWrite.poll();
            if (stopped.done) {
                if (stopped.status) {
                    stopDone = vfs.mkdir("/write-cancelled");
                }
            }
        }
    }
    if (stopDone) {
        if (!freshWrite) {
            freshWrite = _root.writeRemoteFile("/slow-write", 44, 45);
        }
        var other = otherRead.poll();
        var fresh = freshWrite.poll();
        if (other.done) {
            if (fresh.done) {
                if (!other.status) {
                    if (!fresh.status) {
                        var values = other.value + "|" + fresh.value + "|";
                        if (!values.indexOf("61|2|")) {
                            if (!finished) {
                                finished = vfs.mkdir("/write-finished");
                                disconnectWrite = _root.writeRemoteFile("/disconnect-write", 67, 68);
                            }
                        }
                    }
                }
            }
        }
    }
}
