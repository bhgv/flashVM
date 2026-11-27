// Copyright (c) 2026 bhgv
// EN: Cancel one waiting read while the movie and a second read use the same session.
// RU: Отменить одно чтение, пока фильм и второе чтение используют ту же сессию.


// EN: Dispatch the plan once, and signal each admission or completion to Limbo.
// RU: Запустить сценарий один раз и сообщать Limbo о допуске и завершении.
function onEnterFrame() {
    if (!slowRead) {
        slowRead = _root.readRemoteFile("/slow-data", 2);
        otherRead = _root.readRemoteFile("/other-data", 1);
    }
    if (!stopSent) {
        var gate = os.open("/read-gate");
        var permit = gate.read(1);
        var gateClosed = gate.close();
        if (!(permit[0] - 49)) {
            if (!neighbourDone) {
                var neighbour = os.open("/neighbour");
                var bytes = neighbour.read(1);
                var neighbourClosed = neighbour.close();
                if (!(bytes[0] - 90)) {
                    neighbourDone = vfs.mkdir("/read-before");
                }
            }
        }
        if (!(permit[0] - 50)) {
            if (neighbourDone) {
                var requested = slowRead.cancel();
                stopSent = true;
            }
        }
    }
    if (stopSent) {
        if (!stopDone) {
            var stopped = slowRead.poll();
            if (stopped.done) {
                if (stopped.status) {
                    stopDone = vfs.mkdir("/read-cancelled");
                }
            }
        }
    }
    if (stopDone) {
        if (!freshRead) {
            freshRead = _root.readRemoteFile("/slow-data", 1);
        }
        var other = otherRead.poll();
        var fresh = freshRead.poll();
        if (other.done) {
            if (fresh.done) {
                if (!other.status) {
                    if (!fresh.status) {
                        var values = other.value + "|" + fresh.value + "|";
                        if (!values.indexOf("61|82|")) {
                            if (!finished) {
                                finished = vfs.mkdir("/read-finished");
                                disconnectRead = _root.readRemoteFile("/disconnect-data", 1);
                            }
                        }
                    }
                }
            }
        }
    }
}
