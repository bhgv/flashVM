// Copyright (c) 2026 bhgv
// EN: Ask a native task to load a malformed regular Dis file.
// RU: Попросить нативную задачу загрузить повреждённый обычный файл Dis.


// EN: Propagate the loader's failure through the task result.
// RU: Передать отказ загрузчика в результат задачи.
asinc function refuseMalformed() {
    var source = vfs.open("/malformed.dis");
    var proxy = source.load();
    return 1;
}
