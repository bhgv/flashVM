// Copyright (c) 2026 bhgv
// EN: Run one scoped call on a detached AS worker until it is cancelled.
// RU: Выполнить контекстный вызов в отделённой задаче AS до её отмены.


// EN: Send a lent queue, string or graph to the corresponding waiting export.
// RU: Передать очередь, строку или граф соответствующему ожидающему экспорту.
asinc function parkDisCall(mode) {
    var source = vfs.open("/cancel-target.dis");
    var remote = source.load();
    var closed = source.close();
    if (mode) {
        if (mode - 1) {
            var empty = "";
            var leaf = empty.split("");
            var leafCount = leaf.push(40);
            var box = empty.split("");
            var boxCount = box.push(leaf);
            var graph = remote.parkGraph(box);
        } else {
            var text = remote.parkText("task");
        }
    } else {
        var queue = vfs.localChannel("i", 1);
        var sent = queue.trySend(41);
        var blank = "";
        var pending = blank.split("");
        var fields = pending.push(40);
        var channel = remote.park(pending, queue);
    }
    return 1;
}
