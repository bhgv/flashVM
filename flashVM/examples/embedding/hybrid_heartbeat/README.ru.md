# Гибридная политика heartbeat

Исполняемый пример с виртуальным временем, без сокетов/сети: периодический UDP
через 30 секунд, TCP через 300 секунд, но аутентифицированный UDP-совет может
запросить раннюю проверку TCP. Это иллюстративные интервалы, не рекомендация NAT.

```sh
cmake -S flashVM -B build-heartbeat -DFVM_BUILD_EXAMPLES=ON
cmake --build build-heartbeat --target flashvm_examples_embedding_hybrid_heartbeat_main
./build-heartbeat/flashvm_examples_embedding_hybrid_heartbeat_main
```

Требуются C89-компилятор и CMake; из компонента используйте `-S .`. Target
линкуется только с flashvm_srecord, без VM/SDL. Все структуры на стеке; heap и
cleanup не нужны. Политика показывает отдельные решения для UDP/TCP, сохранение
TCP receive-silence после send, явное подтверждение receive и кодирование hint.
Вместо виртуальных activity реальный host сообщает только успешный I/O, защищает
вход до accept, сериализует state и применяет суммарные квоты/jitter. Пример не
реализует аутентификацию, STUN, broadcast, multicast, NAT traversal и listener.

| API | Контракт и владение |
| --- | --- |
| [`fvm_rheartbeat_accept`](../../../include/flashvm/rheartbeat.h#L92) | Принимает уже аутентифицированный совет после проверок эпохи, sequence и прав. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |
| [`fvm_rheartbeat_activity`](../../../include/flashvm/rheartbeat.h#L86) | Отмечает успешный send/receive одного пути; UDP не обновляет TCP. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |
| [`fvm_rheartbeat_clear`](../../../include/flashvm/rheartbeat.h#L97) | Host отклоняет reconnect без ложного подтверждения живости и сетевого действия. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |
| [`fvm_rheartbeat_due`](../../../include/flashvm/rheartbeat.h#L81) | Продвигает виртуальное monotonic-время и предлагает действия без подтверждения I/O. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |
| [`fvm_rheartbeat_hint_write`](../../../include/flashvm/rheartbeat.h#L106) | Кодирует компактные поля и CRC в буфер; аутентификация не добавляется. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |
| [`fvm_rheartbeat_init`](../../../include/flashvm/rheartbeat.h#L76) | Копирует policy, устанавливает аутентифицированную эпоху и раздельные ages. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |
| [`fvm_rheartbeat_next`](../../../include/flashvm/rheartbeat.h#L102) | Выдаёт не оборачивающуюся исходящую идентичность; копия сохраняется для retry. [Contract](../../../doc/HYBRID_HEARTBEAT.ru.md). |

[Source](main.c), [guide](../../../doc/HYBRID_HEARTBEAT.ru.md), [catalog](../../README.md#example-embedding-hybrid_heartbeat).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rheartbeat_accept` | function | [include/flashvm/rheartbeat.h:92](../../../include/flashvm/rheartbeat.h#L92) |
| `fvm_rheartbeat_activity` | function | [include/flashvm/rheartbeat.h:86](../../../include/flashvm/rheartbeat.h#L86) |
| `fvm_rheartbeat_clear` | function | [include/flashvm/rheartbeat.h:97](../../../include/flashvm/rheartbeat.h#L97) |
| `fvm_rheartbeat_due` | function | [include/flashvm/rheartbeat.h:81](../../../include/flashvm/rheartbeat.h#L81) |
| `fvm_rheartbeat_hint_write` | function | [include/flashvm/rheartbeat.h:106](../../../include/flashvm/rheartbeat.h#L106) |
| `fvm_rheartbeat_init` | function | [include/flashvm/rheartbeat.h:76](../../../include/flashvm/rheartbeat.h#L76) |
| `fvm_rheartbeat_next` | function | [include/flashvm/rheartbeat.h:102](../../../include/flashvm/rheartbeat.h#L102) |
<!-- source-links:end -->
