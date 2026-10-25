# Независимые наблюдатели ресурсов

Пример C89 создаёт два отдельных читателя одного ресурса. Каждый получает событие
sequence=1, missed=0, bytes=4. VM/SDL и сокеты не запускаются. Сессии 1 и 2 —
доверенные локальные ID, а не пример сетевой аутентификации.

```sh
cmake -S flashVM -B build-events -DFVM_BUILD_EXAMPLES=ON
cmake --build build-events --target flashvm_examples_embedding_resource_events_main
./build-events/flashvm_examples_embedding_resource_events_main
```

Из каталога компонента используйте `cmake -S .`. Нужны C89-компилятор и CMake;
внешние мультимедийные SDK не требуются. Пример линкуется только с flashvm_srecord.
Диспетчер владеет копиями событий. Подписки закрываются отдельно; общий cleanup
отключает обе сессии, затем освобождает диспетчер. Дополнительные потоки требуют
настоящих host lock/unlock и проверки потока владельца. При переполнении читатель
получает счётчик потерь; пример показывает успешную доставку без переполнения.

| API | Ownership / execution |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Own one bounded native dispatcher; NULL policy is single-owner use. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_register`](../../../include/flashvm/rdispatch.h#L115) | Copy a path and operation metadata; retain the host callback borrow. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Owner explicitly grants current session rights. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_subscribe`](../../../include/flashvm/revents.h#L63) | Create one independent queue; output handle is session-scoped. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_resources`](../../../include/flashvm/revents.h#L48) | Copy the session-visible resource snapshot without VM calls. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_publish`](../../../include/flashvm/revents.h#L70) | Owner copies an event to each reader without waiting for consumers. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_event_take`](../../../include/flashvm/revents.h#L79) | Copy and dequeue only this authenticated reader’s event. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_unsubscribe`](../../../include/flashvm/revents.h#L91) | Discard one reader’s remaining events and invalidate its watch. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_disconnect`](../../../include/flashvm/revents.h#L99) | Owner revokes a quiesced session and discards its pending native state. [Contract](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Release owned storage after consumers and accepted commands are quiesced. [Contract](../../../doc/RESOURCE_DISPATCH.ru.md). |

[Source](main.c), [guide](../../../doc/RESOURCE_EVENTS.ru.md), [catalog](../../README.md#example-embedding-resource_events).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_disconnect` | function | [include/flashvm/revents.h:99](../../../include/flashvm/revents.h#L99) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:79](../../../include/flashvm/revents.h#L79) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_publish` | function | [include/flashvm/revents.h:70](../../../include/flashvm/revents.h#L70) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_resources` | function | [include/flashvm/revents.h:48](../../../include/flashvm/revents.h#L48) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:63](../../../include/flashvm/revents.h#L63) |
| `fvm_rdispatch_unsubscribe` | function | [include/flashvm/revents.h:91](../../../include/flashvm/revents.h#L91) |
<!-- source-links:end -->
