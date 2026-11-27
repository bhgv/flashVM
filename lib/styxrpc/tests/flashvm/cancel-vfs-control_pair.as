// Copyright (c) 2026 bhgv
// EN: Cancel each suspended operation after an independent VFS read succeeds.
// RU: Отменить каждую ожидающую операцию после успешного независимого чтения VFS.


// EN: Start one detached operation per stage and supervise it across player frames.
// RU: Запускать одну отделённую операцию на этап и проверять её на кадрах плеера.
function onEnterFrame() {
    if (!controlStarted) {
        controlStage = 0;
        controlStarted = true;
    }
    if (!controlDone) {
        if (!controlTask) {
            controlTask = _root.controlWait(controlStage);
        }
        if (!controlRequested) {
            var gate = os.open("/control-gate");
            var ready = gate.read(1);
            var gateClosed = gate.close();
            if (!(ready[0] - 49 - controlStage)) {
                var neighbour = os.open("/neighbour");
                var bytes = neighbour.read(1);
                var neighbourClosed = neighbour.close();
                if (!(bytes[0] - 90)) {
                    var request = controlTask.cancel();
                    controlRequested = true;
                }
            }
        }
        if (controlRequested) {
            var result = controlTask.poll();
            if (result.done) {
                if (result.status) {
                    var advance = true;
                    if (!(controlStage - 1)) {
                        var recovered = os.opendir("/hold-dir");
                        var first = recovered.next_file();
                        var recoveredClosed = recovered.close();
                        var entryName = first.name;
                        if (entryName.indexOf("child")) {
                            advance = false;
                        }
                    }
                    if (advance) {
                        controlStage = controlStage + 1;
                        controlTask = null;
                        controlRequested = false;
                        if (!(controlStage - 5)) {
                            controlDone = vfs.mkdir("/control-done");
                        }
                    }
                }
            }
        }
    }
}
