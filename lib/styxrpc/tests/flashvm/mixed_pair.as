// Copyright (c) 2026 bhgv
// EN: Echo exactly one ready record per frame over the player's application vfs.
// RU: Возвращать одну готовую запись за кадр через порт приложения плеера.


// EN: Retain the received value through backpressure and drain subsequent records in order.
// RU: Удерживать при задержке полученное значение и забирать следующие по порядку.
function onEnterFrame() {
    if (!createdRpcFile) {
        var session = vfs.status();
        var input = os.open("/rpc-source");
        var content = input.read(5);
        var inClosed = input.close();
        var made = vfs.mkdir("/created/nested");
        var entered = os.cd("/created/nested");
        var temporary = vfs.open("./temporary", "wx");
        var tempClosed = temporary.close();
        var fileRemoved = os.rm("temporary");
        var parent = vfs.cd("..");
        var directoryRemoved = vfs.rm("nested");
        var output = vfs.open("rpc-file", "wx");
        var saved = output.write(content);
        var outClosed = output.close();
        var rootOutput = os.open("/rpc-root-file", "wx");
        var rootSaved = rootOutput.write(content);
        var rootClosed = rootOutput.close();
        createdRpcFile = true;
    }
    if (!pending) {
        var ready = vfs.ready();
        if (ready) {
            current = vfs.recv();
            pending = true;
        }
    }
    if (pending) {
        var sent = vfs.send(current);
        if (sent) {
            pending = false;
        }
    }
}
