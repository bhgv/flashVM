// Copyright (c) 2026 bhgv
// EN: Match the caller and leave a worker waiting for graceful teardown on disconnect.
// RU: Обменяться с вызывающим и оставить ожидающий поток для проверки отключения.
function onEnterFrame() {
    if (!ready) {
        pending = _root.useChannel("/no-peer", true);
        var port = vfs.chan("/work");
        var sent = port.send("typed");
        var echo = port.recv();
        var closed = port.delete();
        ready = true;
    }
}
