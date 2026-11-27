// Copyright (c) 2026 bhgv
// EN: Publish the actual writable FlashVM Int32 observed by a resident Limbo worker.
// RU: Опубликовать настоящую записываемую Int32 FlashVM для резидента Limbo.


// EN: Publish once per movie. Restart gives the same path a different Qid and a new value.
// RU: Опубликовать один раз на фильм. После перезапуска путь получает другой Qid и значение.
function onEnterFrame() {
    if (!modulePublished) {
        Math.exportCounter = 17;
        var methodText = "magnitude,abs";
        var methods = methodText.split(",");
        var variableText = "counter,exportCounter,var,rw";
        var variables = variableText.split(",");
        var image = vfs.publishObject("ref-port.dis", Math, methods, variables);
        modulePublished = true;
    }
}
