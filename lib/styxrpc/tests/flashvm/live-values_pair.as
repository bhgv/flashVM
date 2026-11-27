// Copyright (c) 2026 bhgv
// EN: Run the typed value exchange on the movie owner and release its original proxy.
// RU: Выполнить типизированный обмен у владельца фильма и освободить исходный прокси.


// EN: Publish a completion marker after the copied values, ref and channel all pass.
// RU: Выставить метку завершения после проверки копий, ref и канала.
function onEnterFrame() {
    if (!valuesStarted) {
        valuesStarted = true;
        var file = vfs.open("/wide.dis");
        var proxy = file.load();
        var closed = file.close();
        var reply = _root.proveWideValues(proxy);
        var released = os.unload(proxy);
        var text = reply + "|";
        if (released) {
            if (!text.indexOf("9223372036854775806|-9223372036854775807|")) {
                var marker = vfs.mkdir("/wide-done");
            }
        }
    }
}
