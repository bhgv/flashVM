// Copyright (c) 2026 bhgv
// EN: Check the whole bridge stage in one source-built movie and one paired session.
// RU: Проверить весь этап моста в одном фильме из исходника и в одной парной сессии.


// EN: Probe transfer and write-back before replacing the provider's VFS publication.
// RU: Проверить обмен и обратную запись перед заменой публикации VFS провайдера.
function onEnterFrame() {
    if (!started) {
        started = true;
        var file = vfs.open("/stage-library.dis");
        var proxy = file.load();
        var sourceClosed = file.close();
        var sequences = _root.proveSequences(proxy);
        var mixed = _root.proveMixed(proxy);
        var live = _root.proveLiveVariables();
        if (!sequences.indexOf("end|9223372036854775806|☀|-9223372036854775807|null|12|☀|moved|false|end|")) {
            if (!mixed.indexOf("-9223372036854775801|9223372036854775807|-9223372036854775801|9223372036854775807|-9223372036854775808|true|")) {
                if (!live.indexOf("3|3|-17|42|42|start|4|d|9223372036854775807|-9223372036854775808|9223372036854775807|true|")) {
                    var mutated = vfs.mkdir("/live-stage-mutated");
                    staleTask = _root.rejectStale();
                }
            }
        }
    }
    if (staleTask) {
        var outcome = staleTask.poll();
        if (outcome.done) {
            if (!(outcome.status + 6)) {
                var current = vfs.liveVariable("/live/.var");
                var numbers = current.field("numbers");
                var value = numbers.get(0);
                var closed = numbers.close();
                var retired = current.close();
                if (!(value - 99)) {
                    var done = vfs.mkdir("/live-stage-done");
                }
            }
        }
    }
}
