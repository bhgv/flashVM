// Copyright (c) 2026 bhgv
// EN: Load the same image on the paired mount and exercise checked method admission.
// RU: Загрузить образ из парного монтирования и пройти проверку перед вызовом.


// EN: Borrow the stage number. Call a scalar or shaped method at its stage.
// At recovery, require fresh instance state and a verified string reply.
// RU: Принять номер этапа. Вызвать скалярный или типизированный метод по этапу.
// При восстановлении проверить исходное состояние и строковый ответ.
asinc function moduleSetup(stage) {
    var source = os.open("/setup.dis");
    var module = source.load();
    var closed = source.close();
    var answer = 0;
    if (!(stage - 3)) {
        answer = module.increment(7);
    }
    if (!(stage - 4)) {
        var cancelled = module.label("cancelled");
    }
    if (!(stage - 5)) {
        var total = module.current(0);
        var label = module.label("ready");
        var text = label + "|";
        var released = os.unload(module);
        if (!(total - 5)) {
            if (!text.indexOf("ready!|")) {
                if (released) {
                    answer = 42;
                }
            }
        }
    }
    return answer;
}
