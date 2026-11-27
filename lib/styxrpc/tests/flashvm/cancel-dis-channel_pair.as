// Copyright (c) 2026 bhgv
// EN: Cancel three returned-channel waits while keeping a separate channel usable.
// RU: Отменить три ожидания на возвращённых каналах, сохранив отдельный канал рабочим.


// EN: Use the survivor's held lease before and after cancelling the worker operations.
// RU: Использовать удержанную аренду соседнего канала до и после отмены задач.
function onEnterFrame() {
    if (!receiveTask) {
        var source = vfs.open("/queues.dis");
        var remote = source.load();
        survivor = remote.queue(40);
        var closed = source.close();
        var unloaded = os.unload(remote);
        receiveTask = _root.parkDisChannel(0);
        sendTask = _root.parkDisChannel(1);
        selectTask = _root.parkDisChannel(2);
    }
    if (!cancelSent) {
        var gate = vfs.open("/channel-gate");
        var bytes = gate.read(1);
        var gateClosed = gate.close();
        if (!(bytes[0] - 49)) {
            var before = survivor.recv();
            var receiving = receiveTask.poll();
            var sending = sendTask.poll();
            var selecting = selectTask.poll();
            var early = receiving.done + sending.done + selecting.done;
            if (!early) {
                if (!(before - 40)) {
                    var receiveStop = receiveTask.cancel();
                    var sendStop = sendTask.cancel();
                    var selectStop = selectTask.cancel();
                    cancelSent = true;
                }
            }
        }
    }
    if (cancelSent) {
        if (!finished) {
            var receive = receiveTask.poll();
            var send = sendTask.poll();
            var select = selectTask.poll();
            var completedCount = receive.done + send.done + select.done;
            var errorCount = receive.status * send.status * select.status;
            if (!(completedCount - 3)) {
                if (errorCount) {
                    var filled = survivor.send(43);
                    var after = survivor.recv();
                    if (!(after - 43)) {
                        if (filled) {
                            var released = survivor.close();
                            finished = vfs.mkdir("/channel-finished");
                        }
                    }
                }
            }
        }
    }
}
