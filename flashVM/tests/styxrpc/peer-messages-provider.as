// Copyright (c) 2026 bhgv
// EN: Publish two separately named exchange roots after the live peer has attached.
// RU: Опубликовать два отдельных корня обмена после подключения другой FlashVM.


// EN: Register the same schema with a different handler on each root.
// RU: Зарегистрировать одну схему с разными обработчиками в двух корнях.
function onEnterFrame() {
    if (!messageReady) {
        var firstName = "peer.sample";
        var first = firstName.split(",");
        var firstCount = first.push(1, 71, "firstMessage");
        var secondName = "peer.sample";
        var second = secondName.split(",");
        var secondCount = second.push(1, 72, "secondMessage");
        var firstRoot = vfs.publishMessages("color-input", _root, first);
        var secondRoot = os.publishMessages("color-render", _root, second);
        var callback = _root.messageReadyFile;
        messageReady = vfs.newnode("/tools/message-ready", "r", callback);
    }
}
