/* FlashVM: Embedding/ActionScript tutorial: shared mutex.
* File: examples/actionscript/shared_mutex.as.
*/

// API illustration; compile to SWF/AVM1 with an external ActionScript compiler.
// Пример API; исходный .as необходимо скомпилировать внешним компилятором.
/* Implement protected Worker for this subsystem. */


/* Объявляем функцию или назначаем обработчик события. */
function protectedWorker(input) // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Вызываем input.lock.set: захватываем полученный общий мутекс с ожиданием. */
    input.lock.set(); // Вызываем input.lock.set: захватываем полученный общий мутекс с ожиданием.
    /* Вызываем input.pipe.send: отправляем сообщение через полученный общий канал.
    * "protected work completed" — передаваемое значение. */
    input.pipe.send("protected work completed"); // Вызываем input.pipe.send: отправляем сообщение через полученный общий канал. "protected work completed" — передаваемое значение.
    /* Вызываем input.lock.reset: освобождаем полученный общий мутекс. */
    input.lock.reset(); // Вызываем input.lock.reset: освобождаем полученный общий мутекс.
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return input.lock; // Возвращаем результат вызывающей стороне и завершаем функцию.
    /* Завершаем текущий блок или описание объекта. */
} // End the documented branch/scope.

/* Объявляем lock и сохраняем начальное значение. */
var lock = mutex.new(); // Объявляем lock и сохраняем начальное значение.
/* Объявляем pipe и сохраняем начальное значение. */
var pipe = channel.create(); // Объявляем pipe и сохраняем начальное значение.
/* Объявляем input и сохраняем начальное значение. */
var input = new Object(); // Объявляем input и сохраняем начальное значение.
/* Сохраняем вычисленное значение: input.lock. */
input.lock = lock; // Сохраняем вычисленное значение: input.lock.
/* Сохраняем вычисленное значение: input.pipe. */
input.pipe = pipe; // Сохраняем вычисленное значение: input.pipe.
/* Объявляем task и сохраняем начальное значение. */
var task = thread.run("protectedWorker", input); // Объявляем task и сохраняем начальное значение.
/* Объявляем message и сохраняем начальное значение. */
var message = pipe.receive(); // Объявляем message и сохраняем начальное значение.
/* Объявляем sameLock и сохраняем начальное значение. */
var sameLock = task.join(); // Объявляем sameLock и сохраняем начальное значение.
// sameLock identifies the same native mutex as lock, rather than a copied lock.
// sameLock — тот же нативный мутекс. Поля state и origin доступны только для чтения.
/* Объявляем held и сохраняем начальное значение. */
var held = sameLock.touch(); // Объявляем held и сохраняем начальное значение.
/* Объявляем availableLocks и сохраняем начальное значение. */
var availableLocks = mutex.list(); // Объявляем availableLocks и сохраняем начальное значение.
