// Copyright (c) 2026 bhgv
// EN: Run named operations concurrently and cancel both unmatched tasks after the exchange.
// RU: Запустить обмен параллельно и отменить обе задачи без пары после ответа.
function onEnterFrame() {
    if (!good) {
        good = _root.useChannel("/work", false);
        receiveTask = _root.useChannel("/wait-read", false);
        sendTask = _root.useChannel("/wait-write", true);
    }
    var answer = good.poll();
    if (answer.done) {
        if (!answer.status) {
            var value = answer.value;
            if (!value.indexOf("typed|")) {
                if (!cancelled) {
                    var stoppedRead = receiveTask.cancel();
                    var stoppedWrite = sendTask.cancel();
                    cancelled = true;
                }
                var readState = receiveTask.poll();
                var writeState = sendTask.poll();
                if (readState.done) {
                    if (writeState.done) {
                        if (readState.status) {
                            if (writeState.status) {
                                if (!completed) {
                                    completed = vfs.mkdir("/ack");
                                    abandoned = _root.useChannel("/abandoned", false);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
