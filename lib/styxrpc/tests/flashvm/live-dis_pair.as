// Copyright (c) 2026 bhgv
// EN: Overlap a direct movie call with a detached task on independent scopes.
// RU: Совместить прямой вызов фильма с отделённой задачей в независимых контекстах.


// EN: Keep the player available while the task makes its scoped calls.
// RU: Оставить плеер доступным, пока задача выполняет контекстные вызовы.
function onEnterFrame() {
    if (!cancelled) {
        if (!cancelTask) {
            cancelTask = _root.loadSlowDis();
        }
        if (!cancelSent) {
            var gate = vfs.open("/cancel-gate");
            var permit = gate.read(1);
            var gateClosed = gate.close();
            if (!(permit[0] - 49)) {
                var requested = cancelTask.cancel();
                cancelSent = true;
            }
        }
        var cancelledResult = cancelTask.poll();
        if (cancelledResult.done) {
            if (cancelledResult.status) {
                if (cancelSent) {
                    cancelled = vfs.mkdir("/cancelled");
                }
            }
        }
    }
    if (cancelled) {
        if (!callbackLoaded) {
            if (!callbackTask) {
                callbackTask = _root.loadSlowDis();
            }
            var callbackResult = callbackTask.poll();
            if (callbackResult.done) {
                if (!callbackResult.status) {
                    if (!callbackResult.value) {
                        callbackLoaded = vfs.mkdir("/callback-image");
                    }
                }
            }
        }
    }
    if (!refusedTask) {
        refusedTask = _root.refuseMalformed();
    }
    var refusal = refusedTask.poll();
    if (refusal.done) {
        if (refusal.status) {
            if (!liveTask) {
                var source = vfs.open("/scalar.dis");
                var scalar = source.load();
                var closed = source.close();
                var checked = scalar.other(7);
                if (!checked) {
                    var released = os.unload(scalar);
                    if (released) {
                        liveTask = _root.liveRoundtrip("library/target.dis");
                    }
                }
            }
        }
    }
    if (liveTask) {
        if (!directDone) {
            var direct = _root.directDisProof();
            if (!direct.indexOf("movie!|41|41|42|true|false|")) {
                directDone = vfs.mkdir("/direct");
            }
        }
        var result = liveTask.poll();
        if (result.done) {
            if (!result.status) {
                if (!(result.value - 12)) {
                    if (!completed) {
                        if (!closedTask) {
                            closedTask = _root.proveClosedDisMethod();
                        }
                        var closedResult = closedTask.poll();
                        if (closedResult.done) {
                            if (closedResult.status) {
                                var reused = _root.proveDisReuse();
                                if (reused) {
                                    completed = vfs.mkdir("/done");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
