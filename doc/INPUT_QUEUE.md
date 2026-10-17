# Deferred input queue / Отложенная очередь ввода

## English

`flashvm/input_queue.h` introduces [`fvm_input_enqueue`](../include/flashvm/input_queue.h#L20), [`fvm_input_pump`](../include/flashvm/input_queue.h#L26), and
[`fvm_input_pending`](../include/flashvm/input_queue.h#L29). Events are copied into a FIFO (capacity: 1024) and
callbacks execute only when [`fvm_input_pump`](../include/flashvm/input_queue.h#L26) is called. `max_events=0` drains
the queue; otherwise it limits the number of dispatched events. The callback
receives the mouse mapping configured with [`fvm_input_set_rule`](../include/flashvm/input.h#L52).

**Concurrency:** this initial implementation is **not thread-safe**. Both
producers and consumers must use the VM-owning thread. Native input threads
must marshal events to that thread via the host's thread-safe message queue.
Callbacks may enqueue more events but must not destroy the context or call
[`fvm_input_pump`](../include/flashvm/input_queue.h#L26) recursively. Events are removed before callbacks run.

See `examples/embedding/input_queue/main.c` (working C89 sample) and
`example.as` (future ActionScript event-handler design, not implemented).
This API does not yet call MovieClip handlers, nor does it provide automatic
OS event polling. Gesture recognition is separately available through
`gesture_recognizer.h`.

## Русский

Заголовок `flashvm/input_queue.h` добавляет [`fvm_input_enqueue`](../include/flashvm/input_queue.h#L20),
[`fvm_input_pump`](../include/flashvm/input_queue.h#L26) и [`fvm_input_pending`](../include/flashvm/input_queue.h#L29). События копируются в FIFO-очередь
ёмкостью 1024 записи. Обработчики вызываются только при [`fvm_input_pump`](../include/flashvm/input_queue.h#L26).
Значение `max_events=0` обрабатывает всю очередь; положительное число
ограничивает количество событий за вызов. Сопоставление жестов кнопкам мыши
задаётся через [`fvm_input_set_rule`](../include/flashvm/input.h#L52).

**Потоки:** текущая очередь **не потокобезопасна**. Добавление и обработка
событий должны выполняться в потоке-владельце VM. Системный поток ввода
должен передавать события через потокобезопасную очередь приложения-хоста.
Обработчик может добавить новые события, но не должен уничтожать контекст
или рекурсивно вызывать [`fvm_input_pump`](../include/flashvm/input_queue.h#L26). Событие извлекается до вызова
обработчика.

Рабочий пример C89: `examples/embedding/input_queue/main.c`.
`example.as` показывает **планируемый**, пока не реализованный API событий
ActionScript. Автоматического вызова обработчиков MovieClip и опроса
системного ввода ещё нет. Распознавание жестов доступно отдельно через
`gesture_recognizer.h`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_enqueue` | function | [include/flashvm/input_queue.h:20](../include/flashvm/input_queue.h#L20) |
| `fvm_input_pending` | function | [include/flashvm/input_queue.h:29](../include/flashvm/input_queue.h#L29) |
| `fvm_input_pump` | function | [include/flashvm/input_queue.h:26](../include/flashvm/input_queue.h#L26) |
| `fvm_input_set_rule` | function | [include/flashvm/input.h:52](../include/flashvm/input.h#L52) |
<!-- source-links:end -->
