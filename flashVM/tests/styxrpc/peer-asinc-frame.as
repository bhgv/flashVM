// Copyright (c) 2026 bhgv
// EN: Start the worker once and sample its completion from a live movie.
// RU: Один раз запустить поток и читать его итог из работающего фильма.
function onEnterFrame() {
    if (!workerTask) {
        workerTask = _root.compute();
    }
    var snapshot = workerTask.poll();
    if (snapshot.done) {
        if (!snapshot.status) {
            if (snapshot.value) {
                if (!(snapshot.value - 42)) {
                    workerReported = _root.reportPeerResult();
                }
            }
        }
    }
}
