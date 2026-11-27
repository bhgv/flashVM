// Copyright (c) 2026 bhgv
// EN: Enter one named or typed channel operation and verify a fresh exchange after cancel.
// RU: Войти в именованный или типизированный канал и проверить новый обмен после отмены.


// EN: Borrow the stage number. Earlier stages stop before any typed guard is sent.
// Return 42 only after confirming a real named enrollment and native typed receive.
// RU: Принять номер этапа. До последнего этапа ветвь типизированного обмена не отправляется.
// Вернуть 42 после успешного входа в именованный канал и типизированного приёма.
asinc function setupOperation(stage) {
    var answer = 0;
    if (!stage) {
        var port = os.chan("/cancel-open");
        var closed = port.delete();
    }
    if (!(stage - 1)) {
        var enrolled = vfs.chan("/cancel-enroll");
        var retired = enrolled.delete();
    }
    if (stage) {
        if (stage - 1) {
            if (!(stage - 5)) {
                var reopened = vfs.chan("/cancel-enroll");
                var released = reopened.delete();
            }
            var image = os.open("/queues.dis");
            var module = image.load();
            var sourceClosed = image.close();
            var queue = module.queue(41);
            var unloaded = os.unload(module);
            var value = queue.recv();
            var queueClosed = queue.close();
            if (unloaded) {
                answer = value + 1;
            }
        }
    }
    return answer;
}
