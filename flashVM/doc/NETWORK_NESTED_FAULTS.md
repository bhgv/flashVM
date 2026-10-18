# Причины ошибок внутри сетевых сообщений v0.77 / Nested network capability faults

Сообщение очереди может содержать retained native mutex/channel. Если этот ресурс
закрывается после send, broker graph conversion на receive/peek получает CLOSED.
Ранее dispatch заменял эту structured cause на RESOURCE_FAILURE: receiving AS
получал общий broker error вместо причины закрытия вложенного ресурса.

Channel opcodes 6 (receive) и 9 (peek) теперь сохраняют pending native resource
code при ошибке conversion. Проверяется существующий диапазон resource codes;
status OK/NOT_FOUND/OOM и fault-free failures сохраняют прежний wire contract.
Протокол и формат ответа не меняются. Caller source/function/line/offset и URI
authority по-прежнему добавляются локально при формировании ResourceException.

Неудачный peek не извлекает packet; receive conversion error потребляет packet
по прежнему правилу. Закрытие вложенного объекта не закрывает внешнюю queue;
последующие обычные сообщения продолжают доставляться. **Полные inner-resource
kind/id/origin/path metadata пока не передаются** этим изменением: client context
может описывать внешнюю receiving channel. Их расширенный wire transport остаётся
отдельной открытой задачей; code CLOSED обозначает реальную вложенную причину.

channel_nested_fault_network использует настоящий loopback broker, поочерёдно
закрывает вложенный mutex/channel после enqueue, вызывает AS-shaped native
inspect/receive, проверяет code/source/network address, unchanged queue после
peek, consumed packet после failed receive и работоспособность outer queue.
При OS_PORTS off live TCP часть исключена. Полная приёмка стадий 6/7 не завершена.

## English

An enqueued packet may retain a native mutex/channel that closes before broker
conversion. Receive/peek dispatch used to replace that structured CLOSED cause
with generic RESOURCE_FAILURE. Opcodes 6/9 now preserve pending recognized native
resource codes without changing wire format or OK/NOT_FOUND/OOM behavior.
Caller locations and authority URI remain client-side ResourceException context.

Failed peek retains the packet; failed receive conversion consumes it under the
existing contract. The enclosing queue remains open and can deliver later values.
Inner kind/id/origin/path metadata is not transported by this change; client
context may still describe the outer receiving channel. Extended cause metadata
remains open. Live TCP regressions cover closed nested mutex/channel causes,
script locations/endpoints, packet retention/consumption and continued delivery.
OS_PORTS-disabled profiles omit live networking; stages 6/7 remain open.

Проверки v0.77 / verification: full headless 261/261; targeted channel/
resource/mutex/GC/serialization tests 72/72 in AGG, Cairo, portable and
ASan/UBSan. Portable excludes TCP, live fonts only in AGG/Cairo; heap/global
ASan and UBSan active, leak detection/stack ASan disabled. Linux only.

## Обновление v0.79 / v0.79 update

Ограничение передачи inner-resource metadata устранено для recognized pending
resource causes: optional FFME record переносит details (NETWORK_FAULT_CONTEXT.md).
Legacy peers/status-only faults по-прежнему используют прежний context fallback.
