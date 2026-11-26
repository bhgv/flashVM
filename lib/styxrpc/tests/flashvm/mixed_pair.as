// Copyright (c) 2026 bhgv
// EN: Echo exactly one ready record per frame over the player's application rpc.
// RU: Возвращать одну готовую запись за кадр через порт приложения плеера.


// EN: Retain the received value through backpressure and drain subsequent records in order.
// RU: Удерживать при задержке полученное значение и забирать следующие по порядку.
function onEnterFrame() {
    if (!createdRpcFile) {
        var session = rpc.status();
        var input = os.open("/rpc-source");
        var content = input.read(5);
        var inClosed = input.close();
        var output = rpc.open("/created/rpc-file", "wx");
        var saved = output.write(content);
        var outClosed = output.close();
        var rootOutput = os.open("/rpc-root-file", "wx");
        var rootSaved = rootOutput.write(content);
        var rootClosed = rootOutput.close();
        createdRpcFile = true;
    }
    if (!pending) {
        var ready = rpc.ready();
        if (ready) {
            current = rpc.recv();
            pending = true;
        }
    }
    if (pending) {
        var sent = rpc.send(current);
        if (sent) {
            pending = false;
        }
    }
}
