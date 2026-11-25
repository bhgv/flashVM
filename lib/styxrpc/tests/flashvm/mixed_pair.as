// Copyright (c) 2026 bhgv
// EN: Echo exactly one ready record per frame over the player's application port.
// RU: Возвращать одну готовую запись за кадр через порт приложения плеера.


// EN: Retain the received value through backpressure and drain subsequent records in order.
// RU: Удерживать при задержке полученное значение и забирать следующие по порядку.
function onEnterFrame() {
    if (!port) {
        port = rpc.listen("/.rpc");
    }
    if (!pending) {
        var ready = port.ready();
        if (ready) {
            current = port.recv();
            pending = true;
        }
    }
    if (pending) {
        var sent = port.send(current);
        if (sent) {
            pending = false;
        }
    }
}
