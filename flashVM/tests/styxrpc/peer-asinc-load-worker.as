// Copyright (c) 2026 bhgv
// EN: Keep a checked Dis module proxy in the detached worker's own context.
// RU: Хранить проверенный прокси модуля Dis в отдельном контексте задачи.


// EN: Load the published image at path, close its handle and invoke next with input.
// RU: Загрузить образ по path, закрыть дескриптор и вызвать next с аргументом input.
asinc function callDis(path, input) {
    var image = os.open(path);
    var proxy = image.load();
    var closed = image.close();
    return proxy.next(input);
}
