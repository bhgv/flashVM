/* FlashVM: Embedding/ActionScript tutorial: shared mutex.
 * File: examples/actionscript/shared_mutex.as.
 */

// API illustration; compile to SWF/AVM1 with an external ActionScript compiler.
// Пример API; исходный .as необходимо скомпилировать внешним компилятором.
/* Implement protected Worker for this subsystem. */
/* Объявляем функцию или назначаем обработчик события. */
function protectedWorker(input)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Выполняем вызов input.lock.set с указанными аргументами. */
    input.lock.set();
    /* Выполняем вызов input.pipe.send с указанными аргументами. */
    input.pipe.send("protected work completed");
    /* Выполняем вызов input.lock.reset с указанными аргументами. */
    input.lock.reset();
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return input.lock;
    /* Завершаем текущий блок или описание объекта. */
}
/* Объявляем lock и сохраняем начальное значение. */
var lock = mutex.new();
/* Объявляем pipe и сохраняем начальное значение. */
var pipe = channel.create();
/* Объявляем input и сохраняем начальное значение. */
var input = new Object();
/* Сохраняем вычисленное значение: input.lock. */
input.lock = lock;
/* Сохраняем вычисленное значение: input.pipe. */
input.pipe = pipe;
/* Объявляем task и сохраняем начальное значение. */
var task = thread.run("protectedWorker", input);
/* Объявляем message и сохраняем начальное значение. */
var message = pipe.receive();
/* Объявляем sameLock и сохраняем начальное значение. */
var sameLock = task.join();
// sameLock identifies the same native mutex as lock, rather than a copied lock.
// sameLock — тот же нативный мутекс. Поля state и origin доступны только для чтения.
/* Объявляем held и сохраняем начальное значение. */
var held = sameLock.touch();
/* Объявляем availableLocks и сохраняем начальное значение. */
var availableLocks = mutex.list();
