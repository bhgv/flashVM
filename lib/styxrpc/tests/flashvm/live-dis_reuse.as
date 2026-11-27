// Copyright (c) 2026 bhgv
// EN: Exercise explicit module retirement past the live generation slot limit.
// RU: Проверить явное освобождение модулей за пределами числа слотов поколений.


// EN: Load and call fresh instances even while the script retains every closed proxy.
// RU: Загружать и вызывать свежие экземпляры, удерживая все закрытые прокси в скрипте.
function proveDisReuse() {
    var source = vfs.open("/scalar.dis");
    var empty = "";
    var retired = empty.split("");
    var loads = 40; // More than the 32 simultaneous request slots.
    var i = 0;
    var complete = true;
    while (loads - i) {
        var instance = source.load();
        var answer = instance.other(7);
        var released = os.unload(instance);
        if (answer) {
            complete = false;
        }
        if (!released) {
            complete = false;
        }
        var total = retired.push(instance);
        i = i + 1;
    }
    var closed = source.close();
    return complete;
}
