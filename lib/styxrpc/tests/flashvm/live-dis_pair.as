// Copyright (c) 2026 bhgv
// EN: Overlap a direct movie call with a detached task on independent scopes.
// RU: Совместить прямой вызов фильма с отделённой задачей в независимых контекстах.


// EN: Keep the player available while the task makes its scoped calls.
// RU: Оставить плеер доступным, пока задача выполняет контекстные вызовы.
function onEnterFrame() {
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
                    liveTask = _root.liveRoundtrip("library/target.dis");
                }
            }
        }
    }
    if (liveTask) {
        if (!directDone) {
            var direct = _root.directDisProof();
            if (!direct.indexOf("movie!|41|41|42|")) {
                directDone = vfs.mkdir("/direct");
            }
        }
        var result = liveTask.poll();
        if (result.done) {
            if (!result.status) {
                if (!(result.value - 12)) {
                    if (!completed) {
                        completed = vfs.mkdir("/done");
                    }
                }
            }
        }
    }
}
