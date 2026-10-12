/* FlashVM: Embedding/ActionScript tutorial: gc workers channels.
 * File: examples/actionscript/gc_workers_channels.as.
 */

// Named-function extension; host must call fvm_extensions_install(context).
// Compile with a tool supporting FlashVM's documented AVM1 subset.
/* Implement echo for this subsystem. */
/* Объявляем функцию или назначаем обработчик события. */
function echo(input)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Выполняем вызов sharedChannel.send с указанными аргументами. */
    sharedChannel.send(input);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return input;
    /* Завершаем текущий блок или описание объекта. */
}
/* Выполняем вызов channel.create с указанными аргументами. */
pipe = channel.create(4);
/* Выполняем вызов thread.run_suspended с указанными аргументами. */
worker = thread.run_suspended("echo", 42, pipe);
/* Выполняем вызов worker.start с указанными аргументами. */
worker.start();
/* Выполняем вызов pipe.receive с указанными аргументами. */
message = pipe.receive();
/* Выполняем вызов worker.join с указанными аргументами. */
result = worker.join();
/* Сохраняем вычисленное значение: worker. */
worker = null;
/* Выполняем вызов gc.collect с указанными аргументами. */
collected = gc.collect();
/* Выполняем вызов pipe.close с указанными аргументами. */
pipe.close();
/* Выполняем вызов gc.mode с указанными аргументами. */
gc.mode(100);
/* Выполняем вызов gc.state с указанными аргументами. */
mode = gc.state();
