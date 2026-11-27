// Copyright (c) 2026 bhgv
// EN: Check that a captured Dis method shares explicit proxy retirement.
// RU: Проверить, что извлечённый метод видит явное закрытие прокси.


// EN: Verify that an extracted method works before unload and fails afterwards.
// RU: Проверить работу извлечённого метода до выгрузки и отказ после неё.
asinc function proveClosedDisMethod() {
    var source = vfs.open("/scalar.dis");
    var instance = source.load();
    var closed = source.close();
    var saved = instance.other;
    var first = saved.call(instance, 7);
    var last = 0;
    if (!first) {
        var released = os.unload(instance);
        if (released) {
            var signalled = vfs.mkdir("/closed-method");
            last = saved.call(instance, 7);
        }
    }
    return last;
}
