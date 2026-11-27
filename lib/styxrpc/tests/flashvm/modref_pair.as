// Copyright (c) 2026 bhgv
// EN: Publish a Flash-owned Int32, receive its two aliases and send them back.
// RU: Опубликовать Int32 во FlashVM, принять две ссылки и вернуть их обратно.


// EN: Keep serving VFS until Limbo has captured both fids.
// RU: Обслуживать VFS, пока Limbo не захватит оба fid.
function onEnterFrame() {
    if (!modulePublished) {
        Math.exportCounter = 41;
        var methodText = "magnitude,abs";
        var methods = methodText.split(",");
        var variableText = "counter,exportCounter,var,rw";
        var variables = variableText.split(",");
        var image = vfs.publishObject("ref-pair.dis", Math, methods, variables);
        modulePublished = true;
    }
    if (!exchangeDone) {
        var marker = os.stat("/module-ref-ready");
        var status = marker.join(",");
        if (!status.indexOf("0,")) {
            var scopeInfo = os.stat("/.varref");
            var scopeStatus = scopeInfo.join(",");
            var pipe = vfs.chan("/module-refs");
            var writable = pipe.recv();
            var observer = pipe.recv();
            var before = observer;
            writable = 0 - 73;
            var current = Math.exportCounter;
            var observed = observer;
            var proof = before + "," + current + "," + observed;
            if (!scopeStatus.indexOf("0,")) {
                proof = "scope became visible";
            }
            var reply = pipe.send(proof);
            var first = pipe.send(ref writable);
            var second = pipe.send(ref observer);
            var closed = pipe.delete();
            exchangeDone = true;
        }
    }
}
