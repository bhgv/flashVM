// Copyright (c) 2026 bhgv
// EN: Publish composed queues and drive two typed result endpoints in the stock player.
// RU: Публикация составных очередей и работа двух концов результата в штатном плеере.


// EN: Retain a duplex string endpoint and queue its first outgoing value.
// RU: Удержать двустороннюю строковую очередь и поставить первое исходящее значение.
function buffered(seed) {
    bufferedPort = vfs.resultChannel("s", 1);
    var first = bufferedPort.trySend(seed);
    var full = bufferedPort.trySend("unwanted");
    var result = "incorrect buffer capacity";
    if (first) {
        if (!full) {
            result = bufferedPort;
        }
    }
    return result;
}
