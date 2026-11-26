// Copyright (c) 2026 bhgv
// EN: Echo exactly one ready record per frame over the player's application rpc.
// RU: Возвращать одну готовую запись за кадр через порт приложения плеера.


// EN: Retain the received value through backpressure and drain subsequent records in order.
// RU: Удерживать при задержке полученное значение и забирать следующие по порядку.
function onEnterFrame() {
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
