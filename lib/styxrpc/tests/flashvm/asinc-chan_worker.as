// Copyright (c) 2026 bhgv
// EN: Wait for the other side on a channel leased by this worker.
// RU: Дождаться другой стороны на канале, открытом этим потоком.
asinc function useChannel(path, sending) {
    var port = os.chan(path);
    var value = 0;
    if (sending) {
        var sent = port.send("orphan");
    }
    else {
        value = port.recv();
        var echoed = port.send(value);
    }
    var closed = port.delete();
    return value + "|";
}
