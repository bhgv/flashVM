# Render jobs / Потоки отрисовки

[Background queue, EN](RENDER_ASYNC.md) · [Фоновая очередь, RU](RENDER_ASYNC.ru.md) ·
[Runnable tutorial](../examples/embedding/parallel_render/README.md)

## English

`fvm_render_jobs_run(job_count, workers, callback, userdata)` distributes independent
native jobs and **waits** for completion. workers=0 runs on the caller. Callback
inputs must remain alive until return; outputs must be disjoint or synchronized.
No live AVM1 object mutation or throwing through the C boundary is allowed.
Return 0 on success, -1 for invalid input, allocation or thread creation failure.
All partitions are initialized before launch. A partial launch completes unstarted
partitions synchronously, joins started workers, then reports failure.

[`fvm_render_queue_create`](../include/flashvm/render_queue.h#L32) supplies a separate persistent asynchronous queue.
Submit returns without waiting for raster work; nonblocking take publishes only
the newest result. The editor uses it for detached native paths/private pixels.
See the [full ownership, bounds and shutdown contract](RENDER_ASYNC.md).
Neither executor by itself implements an entire Flash SWF display renderer.

Both APIs use the selected `ports/windows`, `ports/linux`, `ports/mac` or
`ports/android` adapter. Select RENDER_JOBS / NO_RENDER_JOBS in DEFS.txt;
CMake does not silently override that selection with an unrelated cache switch.
The public ABI is C89-compatible; no C++11 or STL is required.

## Русский

[`fvm_render_jobs_run`](../include/flashvm/render_jobs.h#L25) распределяет независимые нативные задания и **ждёт**
завершения. workers=0 выполняет их на вызывающем потоке. Данные живут до возврата;
выходные области не пересекаются без синхронизации. Запрещены изменение живых
объектов AVM1 и исключения через C-границу. Возврат: 0 при успехе, -1 при неверных
аргументах, ошибке памяти или запуска потока. Все участки инициализируются заранее.
При частичном отказе незапущенные выполняются синхронно, запущенные завершаются
через join, после чего возвращается ошибка.

[`fvm_render_queue_create`](../include/flashvm/render_queue.h#L32) предоставляет отдельную постоянную асинхронную очередь.
Submit не ждёт растеризации; опрос без ожидания возвращает только последний
готовый кадр. Редактор передаёт туда отделённые контуры и собственные пиксели:
[полный контракт владения, ограничений и остановки](RENDER_ASYNC.ru.md).
Полный рендерер Flash SWF сам по себе ни одним исполнителем не реализуется.

Платформа выбирает ports/windows, linux, mac либо android. Возможность включается
ключом RENDER_JOBS / NO_RENDER_JOBS в DEFS.txt. Публичный ABI совместим с C89,
C++11 и STL не требуются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../include/flashvm/render_jobs.h#L25) |
| `fvm_render_queue_create` | function | [include/flashvm/render_queue.h:32](../include/flashvm/render_queue.h#L32) |
<!-- source-links:end -->
