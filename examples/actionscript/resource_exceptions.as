// SWF 7/8 ActionScript example. Install FlashVM extension namespaces first.
// Пример SWF 7/8: приложение должно сначала установить расширения FlashVM.
var lock = mutex.new();
var acquired = false;
try {
    lock.set();
    acquired = true;
    // Work with the shared resource / Работа с общим ресурсом.
} catch (failure) {
    trace(failure.code + ": " + failure.message);
    trace(failure.context.transport);
    trace(failure.context.path);
    trace(failure.context.address);
} finally {
    if (acquired) {
        lock.reset();
    }
}
lock.close();
try {
    lock.touch();
} catch (closed) {
    // 1001 = resource.closed. Catching leaves a copied debugger event.
    // Перехват не удаляет копию события из очереди отладчика.
    trace(closed.code);
}
