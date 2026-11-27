// Copyright (c) 2026 bhgv
// EN: Receive and relay references originated by their Limbo owner.
// RU: Принять и переслать ссылки, выданные их владельцем Limbo.


// EN: Relay the held export, followed by a fresh reference after its replacement.
// RU: Переслать открытый экспорт, затем новую ссылку после его замены.
function onEnterFrame() {
    if (!moduleExchangeDone) {
        var pipe = vfs.chan("/dis-module-refs");
        var writable = pipe.recv();
        var observer = pipe.recv();
        var before = writable;
        var first = pipe.send(ref writable);
        var second = pipe.send(ref observer);
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
            var replacement = os.chan("/dis-module-replacement");
            var fresh = replacement.recv();
            var value = fresh;
            var seen = replacement.send("value=" + value);
            var sentRef = replacement.send(ref fresh);
            var retired = replacement.delete();
            replacementExchangeDone = true;
        }
    }
}
