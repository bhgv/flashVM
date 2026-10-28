# Синхронная и фоновая нативная отрисовка

[English](README.md) · [C89 с комментариями каждой строки](main.c) ·
[Руководство очереди](../../../doc/RENDER_ASYNC.ru.md) · [Пакетный API](../../../doc/RENDER_THREADS.md)

Исполняемый пример показывает два контракта. Сначала четыре потока заполняют
64 независимых пикселя, а [`fvm_render_jobs_run`](../../../include/flashvm/render_jobs.h#L25) ждёт их завершения. Затем постоянный
фоновый worker заполняет отдельно выделенную картинку: submit не ждёт его,
владелец продолжает работу. Последний take намеренно ждёт, чтобы консольный пример
проверил результат до выхода. В интерактивном хосте используют wait=0 и продолжают
VM/event-loop при [FVM_NOT_FOUND](../../../include/flashvm/flashvm.h#L49).

Это пример очереди и владения памятью, а не полный рендерер MovieClip.
Настоящая растеризация контуров IDE использует эту же очередь. В руководстве
описаны ограничения кадра, согласование кликов с изображением, replay/capture и
непроверенные платформы. Соседний [example.as](example.as) иллюстрирует обычные
обновления в потоке владельца; он не создаёт потоков и не заменяет C-программу.

## Сборка и запуск

Нужна настроенная сборка FlashVM с RENDER_JOBS в DEFS.txt и включёнными примерами.
Подготовка SDK: [первые шаги](../../../../README-FIRSTSTEPS.md).

```sh
cmake --build /путь/к/сборке --target flashvm_examples_embedding_parallel_render_main
/путь/к/сборке/flashvm_examples_embedding_parallel_render_main
```

Ожидаются строки:

```text
Owner continues after submit; native raster may still be running.
PASS joined batch and asynchronous native snapshot
```

## API и освобождение

| Функция | Проверяемый контракт |
| --- | --- |
| [`fvm_render_jobs_run`](../../../include/flashvm/render_jobs.h#L25) | Буфер на стеке заимствуется только до возврата с join; callbacks пишут в разные пиксели. |
| [`fvm_render_queue_create`](../../../include/flashvm/render_queue.h#L32) | Владелец получает очередь и постоянный worker; callbacks живут до конца destroy. |
| [`fvm_render_queue_submit`](../../../include/flashvm/render_queue.h#L40) | Выделенный snapshot передаётся только при успехе; при отказе его освобождает вызывающий. |
| [`fvm_render_queue_take`](../../../include/flashvm/render_queue.h#L47) | Завершённый буфер возвращается владельцу вместе с отдельным статусом вычисления. Даже ошибочный результат нужно освободить. |
| [`fvm_render_queue_destroy`](../../../include/flashvm/render_queue.h#L54) | Дожидается потоков и освобождает невзятые задания до уничтожения зависимостей. |

Callbacks работают только со своими нативными байтами. Запрещены обращения к VM,
SDL, шрифтам, исключения и повторный вход в очередь. `release` безопасен и на worker,
и на владельце. После передачи указатель обнуляется; общий блок очистки освобождает
только оставшиеся у вызывающего pending/completed. Память очереди освобождает либо
destroy, либо take с дальнейшим release, без двойного удаления.
Физическое окно или устройство не требуется.

[Каталог примеров](../../README.md#example-embedding-parallel_render).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../../../include/flashvm/flashvm.h#L49) |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../../../include/flashvm/render_jobs.h#L25) |
| `fvm_render_queue_create` | function | [include/flashvm/render_queue.h:32](../../../include/flashvm/render_queue.h#L32) |
| `fvm_render_queue_destroy` | function | [include/flashvm/render_queue.h:54](../../../include/flashvm/render_queue.h#L54) |
| `fvm_render_queue_submit` | function | [include/flashvm/render_queue.h:40](../../../include/flashvm/render_queue.h#L40) |
| `fvm_render_queue_take` | function | [include/flashvm/render_queue.h:47](../../../include/flashvm/render_queue.h#L47) |
<!-- source-links:end -->
