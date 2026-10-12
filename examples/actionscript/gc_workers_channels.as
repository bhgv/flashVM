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
    /* Вызываем sharedChannel.send: отправляем сообщение через общий канал.
 * input — передаваемое значение. */
    sharedChannel.send(input);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return input;
    /* Завершаем текущий блок или описание объекта. */
}
/* Вызываем channel.create: создаём канал выбранного вида.
 * 4 — вид канала или настройки. */
pipe = channel.create(4);
/* Вызываем thread.run_suspended: создаём рабочий поток, ожидающий запуска.
 * "echo" — функция рабочего потока.
 * 42 — начальное значение.
 * pipe — аргумент 3 функции. */
worker = thread.run_suspended("echo", 42, pipe);
/* Вызываем worker.start: разрешаем выполнение приостановленного потока. */
worker.start();
/* Вызываем pipe.receive: ожидаем и получаем сообщение канала. */
message = pipe.receive();
/* Вызываем worker.join: ожидаем завершения рабочего потока и получаем результат. */
result = worker.join();
/* Сохраняем вычисленное значение: worker. */
worker = null;
/* Вызываем gc.collect: запускаем сборку недостижимых объектов. */
collected = gc.collect();
/* Вызываем pipe.close: закрываем канал и пробуждаем ожидающих. */
pipe.close();
/* Вызываем gc.mode: задаём режим сборки мусора.
 * 100 — режим: автоматический, приостановленный или полный. */
gc.mode(100);
/* Вызываем gc.state: читаем текущий режим сборки мусора. */
mode = gc.state();
