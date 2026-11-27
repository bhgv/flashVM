// Copyright (c) 2026 bhgv
// EN: Stop five setup operations while ordinary traffic continues on the same session.
// RU: Отменить пять этапов подготовки, не прерывая обычный обмен в той же сессии.


// EN: Wait across frames for the relay's readiness byte before cancelling each worker.
// RU: Дождаться в следующих кадрах байта готовности и отменить каждую задачу.
function onEnterFrame() {
    if (!setupStarted) {
        setupStage = 0;
        setupStarted = true;
    }
    if (!setupDone) {
        if (!setupTask) {
            setupTask = _root.setupOperation(setupStage);
        }
        if (setupStage - 5) {
            if (!setupRequested) {
                var gate = os.open("/control-gate");
                var ready = gate.read(1);
                var gateClosed = gate.close();
                if (!(ready[0] - 49 - setupStage)) {
                    var neighbour = os.open("/neighbour");
                    var bytes = neighbour.read(1);
                    var neighbourClosed = neighbour.close();
                    if (!(bytes[0] - 90)) {
                        var request = setupTask.cancel();
                        setupRequested = true;
                    }
                }
            }
            if (setupRequested) {
                var result = setupTask.poll();
                if (result.done) {
                    if (result.status) {
                        setupStage = setupStage + 1;
                        setupTask = null;
                        setupRequested = false;
                    }
                }
            }
        } else {
            var readyTask = setupTask.poll();
            if (readyTask.done) {
                if (!readyTask.status) {
                    if (!(readyTask.value - 42)) {
                        setupDone = vfs.mkdir("/control-done");
                    }
                }
            }
        }
    }
}
