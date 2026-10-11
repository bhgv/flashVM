// API illustration; compile to SWF/AVM1 with an external ActionScript compiler.
// Пример API; исходный .as необходимо скомпилировать внешним компилятором.
function protectedWorker(input) {
    input.lock.set();
    input.pipe.send("protected work completed");
    input.lock.reset();
    return input.lock;
}
var lock = mutex.new();
var pipe = channel.create();
var input = new Object();
input.lock = lock;
input.pipe = pipe;
var task = thread.run("protectedWorker", input);
var message = pipe.receive();
var sameLock = task.join();
// sameLock identifies the same native mutex as lock, rather than a copied lock.
// sameLock — тот же нативный мутекс. Поля state и origin доступны только для чтения.
var held = sameLock.touch();
var availableLocks = mutex.list();
