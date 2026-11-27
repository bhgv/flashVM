// Copyright (c) 2026 bhgv
// EN: Enter a blocking operation on a returned channel after its proxy is unloaded.
// RU: Войти в блокирующую операцию возвращённого канала после выгрузки прокси.


// EN: Wait in recv on an empty FIFO, send on a full FIFO or select one empty receive.
// RU: Ждать recv на пустой, send на полной очереди или select с пустой ветвью приёма.
asinc function parkDisChannel(mode) {
    var source = vfs.open("/queues.dis");
    var remote = source.load();
    var closed = source.close();
    var seed = 0;
    if (mode) {
        if (!(mode - 1)) {
            seed = 41;
        }
    }
    var queue = remote.queue(seed);
    var unloaded = os.unload(remote);
    var entered = vfs.mkdir("/channel-entered-" + mode);
    if (mode) {
        if (mode - 1) {
            var empty = "";
            var guard = empty.split("");
            var arity = guard.push(queue);
            var guards = empty.split("");
            var cases = guards.push(guard);
            var selected = queue.select(guards);
        } else {
            var sent = queue.send(42);
        }
    } else {
        var received = queue.recv();
    }
    return 1;
}
