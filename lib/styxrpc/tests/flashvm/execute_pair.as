// Copyright (c) 2026 bhgv
// EN: Publish independently callable nodes on the running movie's session.
// RU: Опубликовать независимые вызываемые узлы в сессии запущенного фильма.


// EN: Register executable and read-only entries once after startup.
// RU: После запуска один раз зарегистрировать исполняемые и читаемый узлы.
function onEnterFrame() {
    if (!published) {
        calls = 0;
        var apply = _root.applyNode;
        var refuse = _root.refuseNode;
        var publishedApply = vfs.newnode("/tools/apply", "x", apply);
        var publishedFail = vfs.newnode("/tools/fail", "x", refuse);
        var publishedRead = vfs.newnode("/tools/read", "r", refuse);
        published = true;
    }
}
