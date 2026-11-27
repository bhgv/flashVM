// Copyright (c) 2026 bhgv
// EN: Drive Dis module calls from detached AS tasks and inspect the replies on the movie thread.
// RU: Вызвать модуль Dis из отдельных задач AS и проверить ответы в потоке фильма.


// EN: Require a successful call after closing the image, then a remote execution failure.
// RU: Проверить вызов после закрытия образа, затем проверить ошибку удалённого выполнения.
function onEnterFrame() {
    if (!disTask) {
        disTask = _root.callDis("/mounted/limbo.dis", 41);
        disFailure = _root.callDis("/mounted/limbo.dis", 0);
    }
    if (!peerDone) {
        var reply = disTask.poll();
        var failure = disFailure.poll();
        if (reply.done) {
            if (failure.done) {
                if (!reply.status) {
                    if (!(reply.value - 42)) {
                        if (!(failure.status + 6)) {  // FVM_EXCEPTION
                            var reported = _root.reportPeerResult();
                            peerDone = true;
                        }
                    }
                }
            }
        }
    }
}
