// Copyright (c) 2026 bhgv
// EN: Send two references to a Limbo-owned Int32 and use the returned proxies.
// RU: Передать две ссылки на Int32 владельца Limbo и использовать возвращённые прокси.


// EN: Relay the held export, followed by a fresh reference after its replacement.
// RU: Переслать открытый экспорт, затем новую ссылку после его замены.
function onEnterFrame() {
    if (!moduleExchangeDone) {
        var path = "/mounted/counter.dis/.var/value";
        var writable = vfs.intVariable(path, true);
        var observer = os.intVariable(path);
        var before = writable.value;
        var pipe = vfs.chan("/dis-module-refs");
        var first = pipe.send(ref writable.value);
        var second = pipe.send(ref observer.value);
        var returned = pipe.recv();
        var readOnly = pipe.recv();
        var high = returned;
        returned = 0 - 2147483647 - 1;
        var low = readOnly;
        var proof = before + "," + high + "," + low;
        var sent = pipe.send(proof);
        var closed = pipe.delete();
        moduleExchangeDone = true;
    }
    if (!replacementExchangeDone) {
        var marker = os.stat("/module-ref-replacement");
        var status = marker.join(",");
        if (!status.indexOf("0,")) {
            var fresh = vfs.intVariable("/mounted/counter.dis/.var/value");
            var value = fresh.value;
            var replacement = os.chan("/dis-module-replacement");
            var seen = replacement.send("value=" + value);
            var sentRef = replacement.send(ref fresh.value);
            var retired = replacement.delete();
            replacementExchangeDone = true;
        }
    }
}
