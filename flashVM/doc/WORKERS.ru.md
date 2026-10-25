> Актуальный статус: [ActionScript API](ACTIONSCRIPT_EXTENSIONS.ru.md) и [владение](CONCURRENCY_SAFETY.ru.md). Текст ниже — ранний этап/проектный документ; его ограничения не описывают текущий API.

# API рабочих потоков (этап 28)

Заголовок `flashvm/worker.h` совместим с C89 и может использоваться из C++98.

- `fvm_worker_run(config, callback, input, userdata, &worker)` запускает системный поток через `ports/<os>/thread.c`. Первым аргументом функция получает **новый** контекст интерпретатора. Входное значение копируется до запуска.
- `fvm_worker_join(worker, &result)` ожидает завершения и возвращает код результата функции; значение результата предоставляется по заимствованной ссылке. Повторный `join` допустим.
- `fvm_worker_destroy(worker)` освобождает дескриптор **после join**. До завершения потока вызов ничего не делает, чтобы не освободить используемую память.

Допустимые входные и выходные типы: undefined, null, boolean, number и UTF-8-строки с нулевым терминатором. Объекты не передаются. Результирующие строки копируются до уничтожения контекста рабочего потока и доступны до уничтожения дескриптора. `userdata` не копируется: вызывающая сторона отвечает за срок его жизни. Каждый поток получает независимые Runtime и Context. Указатели на объекты нельзя передавать между контекстами. Канал допустимо передать через `userdata`, только если его срок жизни согласован с рабочим потоком.

**Ограничения:** пока нет отмены, отложенного запуска, объекта `thread` в ActionScript, сериализации функций, автоматического копирования каналов и ожидания группы потоков. На Linux используется pthread. Переходники Windows/macOS/Android присутствуют, но здесь не проверялись. Таймаутов нет: `join` может ждать бесконечно.

Пример: `examples/embedding/workers/main.c`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_worker_destroy` | function | [include/flashvm/worker.h:98](../include/flashvm/worker.h#L98) |
| `fvm_worker_join` | function | [include/flashvm/worker.h:60](../include/flashvm/worker.h#L60) |
| `fvm_worker_run` | function | [include/flashvm/worker.h:25](../include/flashvm/worker.h#L25) |
<!-- source-links:end -->
