// Copyright (c) 2026 bhgv
// EN: Start the worker once and sample its completion from a live movie.
// RU: Один раз запустить поток и читать его итог из работающего фильма.
function onEnterFrame() {
    if (!workerTask) {
        workerInput = vfs.status();
        workerInput.score = 40;
        workerTask = _root.compute(workerInput, workerInput, 2);
    }
    var snapshot = workerTask.poll();
    if (snapshot.done) {
        if (!snapshot.status) {
            if (snapshot.value) {
                if (!(snapshot.value - 42)) {
                    if (!(workerInput.score - 40)) {
                        workerReported = _root.reportPeerResult();
                    }
                }
            }
        }
    }
}
