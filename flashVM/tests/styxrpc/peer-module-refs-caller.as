// Copyright (c) 2026 bhgv
// EN: Relay read/write and read-only views of one exported Int32 over a named channel.
// RU: Переслать записываемое и читаемое представления одного Int32 по именованному каналу.


// EN: Open the exact remote export, transfer both refs and use the returned aliases.
// RU: Открыть точный удалённый экспорт, передать ссылки и использовать возвращённые привязки.
function onEnterFrame() {
    if (!moduleRefsDone) {
        var ready = os.stat("/module-ref-start");
        var status = ready.join(",");
        if (!status.indexOf("0,")) {
            var writable = vfs.intVariable("/mounted/ref-peer.dis/.var/counter", true);
            var observer = os.intVariable("/mounted/ref-peer.dis/.var/counter");
            var before = writable.value;
            var start = os.open("/module-ref-start", "w");
            var seed = "";
            var signal = seed.split("");
            var length = signal.push(1);
            var go = start.write(signal);
            var pipe = vfs.chan("/module-ref-work");
            var sent = pipe.send(ref writable.value);
            var observed = pipe.send(ref observer.value);
            var proof = pipe.recv();
            var returned = pipe.recv();
            var returnedObserver = pipe.recv();
            var closed = pipe.delete();
            var startClosed = start.close();
            var changed = returned;
            returned = 2147483647;
            var after = writable.value;
            var last = returnedObserver;
            var record = "[" + proof + "|" + before + "," + changed + "," + after + "," + last + "]";
            if (!record.indexOf("[41,-73,-73|41,-73,2147483647,2147483647]")) {
                var success = _root.reportPeerResult();
                moduleRefsDone = true;
            }
        }
    }
}
