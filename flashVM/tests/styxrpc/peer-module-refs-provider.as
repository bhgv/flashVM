// Copyright (c) 2026 bhgv
// EN: Publish a real module variable and receive its references on the issuing VM.
// RU: Опубликовать переменную модуля и принять её ссылки в VM-владельце.


// EN: Gate the receive until the caller has opened its fids, then check local write-through.
// RU: Начать приём после открытия fid вызывающим и проверить запись в исходный слот.
function onEnterFrame() {
    if (!modulePublished) {
        Math.exportCounter = 41;
        var methodText = "magnitude,abs";
        var method = methodText.split(",");
        var variableText = "counter,exportCounter,var,rw";
        var variable = variableText.split(",");
        var image = vfs.publishObject("ref-peer.dis", Math, method, variable);
        var start = _root.startModuleRefs;
        modulePublished = vfs.newnode("/module-ref-start", "w", start);
    }
    if (moduleRefReady) {
        if (!finishedTransfer) {
            var pipe = os.chan("module-ref-work");
            var local = pipe.recv();
            var readOnly = pipe.recv();
            var before = readOnly;
            local = 0 - 73;
            var after = Math.exportCounter;
            var observed = readOnly;
            var proof = before + "," + after + "," + observed;
            var reply = pipe.send(proof);
            var first = pipe.send(ref local);
            var second = pipe.send(ref readOnly);
            var closed = pipe.delete();
            finishedTransfer = true;
        }
    }
}
