// Copyright (c) 2026 bhgv
// EN: Cancel only after the paired Dis owner has admitted the scoped call.
// RU: Отменить задачу только после допуска контекстного вызова владельцем Dis.


// EN: Retain an independent proxy and verify its calls before and after cancellation.
// RU: Удерживать отдельный прокси и проверить его вызовы до и после отмены.
function onEnterFrame() {
    if (!channelTask) {
        var source = vfs.open("/cancel-target.dis");
        survivor = source.load();
        var closed = source.close();
        channelTask = _root.parkDisCall(0);
        textTask = _root.parkDisCall(1);
        graphTask = _root.parkDisCall(2);
    }
    if (!cancelSent) {
        var gate = vfs.open("/call-gate");
        var bytes = gate.read(1);
        var gateClosed = gate.close();
        if (!(bytes[0] - 49)) {
            var before = survivor.echo(40);
            if (!(before - 40)) {
                var channelStop = channelTask.cancel();
                var textStop = textTask.cancel();
                var graphStop = graphTask.cancel();
                cancelSent = true;
            }
        }
    }
    if (cancelSent) {
        if (!finished) {
            var channel = channelTask.poll();
            var text = textTask.poll();
            var graph = graphTask.poll();
            var completedCount = channel.done + text.done + graph.done;
            var errorCount = channel.status * text.status * graph.status;
            if (!(completedCount - 3)) {
                if (errorCount) {
                    var after = survivor.echo(42);
                    if (!(after - 42)) {
                        var released = os.unload(survivor);
                        finished = vfs.mkdir("/call-finished");
                    }
                }
            }
        }
    }
}
