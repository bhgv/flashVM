// Copyright (c) 2026 bhgv
// EN: Load live Dis libraries and retain native calls across a VFS rebind.
// RU: Загрузить библиотеки Dis и сохранить нативные вызовы при перепривязке VFS.


// EN: Compare the new loaded module with the state of the retained typed proxy.
// RU: Сравнить новый загруженный модуль с состоянием удержанного прокси.
asinc function liveRoundtrip(path) {
    var source = vfs.open(path);
    var held = source.load();
    var closed = source.close();
    var added = held.increment(7);
    var label = held.label("ready");
    var text = label + "|";
    var answer = 0;
    if (!(added - 12)) {
        if (!text.indexOf("ready!|")) {
            var ready = vfs.mkdir("/loaded");
            var signal = 0;
            while (signal - 49) {
                var gate = vfs.open("/gate");
                var bytes = gate.read(1);
                var gateClosed = gate.close();
                signal = bytes[0];
            }
            var current = os.open(path);
            var fresh = current.load();
            var fileClosed = current.close();
            var replacement = fresh.other(7);
            if (!replacement) {
                var retained = held.current(0);
                var queue = vfs.localChannel("i", 1);
                var sent = queue.trySend(41);
                var reply = held.bounce(queue);
                held = 0;
                var received = reply.recv();
                var retired = reply.close();
                var queueClosed = queue.close();
                if (!(received - 42)) {
                    answer = retained;
                }
            }
        }
    }
    return answer;
}
