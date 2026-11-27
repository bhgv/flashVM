// Copyright (c) 2026 bhgv
// EN: Poll two workers that use the same Dis module with typed channel arguments and results.
// RU: Опросить две задачи, использующие один модуль Dis с типизированными каналами на входе и выходе.


// EN: Check both scoped exchanges and emit the harness marker on success.
// RU: Проверить оба контекстных обмена и вывести маркер успешного теста.
function onEnterFrame() {
    if (!scopedFirst) {
        scopedFirst = _root.callScoped(40);
        scopedSecond = _root.callScoped(54);
    }
    if (!peerDone) {
        var first = scopedFirst.poll();
        var second = scopedSecond.poll();
        if (first.done) {
            if (!first.status) {
                if (!(first.value - 42)) {
                    firstPassed = true;
                }
            }
        }
        if (second.done) {
            if (!second.status) {
                if (!(second.value - 56)) {
                    secondPassed = true;
                }
            }
        }
        if (firstPassed) {
            if (secondPassed) {
                var reported = _root.reportPeerResult();
                peerDone = true;
            }
        }
    }
}
