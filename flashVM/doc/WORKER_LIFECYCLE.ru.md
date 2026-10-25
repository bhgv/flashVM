# Жизненный цикл рабочих потоков (C89 API)

Каждый рабочий поток имеет собственные Runtime и Context FlashVM. Публичные
функции объявлены в `include/flashvm/worker.h`.

* `fvm_worker_run(config, fn, input, userdata, &worker)` — немедленный запуск.
* `fvm_worker_run_suspended(...)` — создание настоящего системного потока,
  ожидающего разрешения перед вызовом функции.
* `fvm_worker_start(worker)` — разрешение запуска; повторный вызов допустим.
* `fvm_worker_state(worker, &state)` — синхронизированный снимок состояния:
  `SUSPENDED`, `RUNNING`, `FINISHED` либо `JOINED`. Состояние может измениться
  сразу после получения снимка.
* `fvm_worker_join(worker, &output)` — ожидание завершения; при необходимости
  запускает приостановленный поток. Строковый результат принадлежит worker
  и действителен до его уничтожения.
* `fvm_worker_destroy(worker)` — освобождение после join. До join функция
  ничего не делает; после уничтожения указатель использовать нельзя.

Между контекстами копируются только примитивные значения, включая строки.
Flash-объекты и функции не передаются. `userdata` не копируется: вызывающая
сторона обеспечивает его время жизни до join. Одновременный вызов join для
одного дескриптора из нескольких потоков не поддерживается. Для обмена
сообщениями используйте каналы.

Пока **нет** кооперативной остановки, приоритетов потоков, ActionScript
`thread.run` и приостановки уже исполняющейся функции. Принудительное
завершение не реализуется из соображений безопасности состояния VM.

Пример: `examples/embedding/worker_lifecycle/main.c`.

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
| `fvm_worker_run_suspended` | function | [include/flashvm/worker.h:32](../include/flashvm/worker.h#L32) |
| `fvm_worker_start` | function | [include/flashvm/worker.h:38](../include/flashvm/worker.h#L38) |
| `fvm_worker_state` | function | [include/flashvm/worker.h:54](../include/flashvm/worker.h#L54) |
<!-- source-links:end -->
