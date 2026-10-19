# Mixed host input / Смешанный ввод хоста

Executable C89 tutorial [main.c](main.c), every nonempty line explained.
It captures Mouse-down, a copied Key snapshot with its event-time pointer,
and Mouse-up into one per-context FIFO. A two-event pump verifies order and
snapshot independence; cancellation drops the pending up, and the host
explicitly resets held keys. No window, graphics, physical input, SDL or
Clutter SDK is required. This is an input example, not another display frontend.

Русский: пример объединяет контакт и клавиатурный снимок в очереди VM.
Изменение исходного снимка после enqueue не влияет на доставку. Отмена очереди
не генерирует события и не сбрасывает клавиши: это отдельная политика хоста.
Нет фонового потока и нет обещания потокобезопасности.

## Build/run / Сборка и запуск

From the VM component directory, any VM profile including NO_GRAPHICS:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_mixed_input_main
./build/flashvm_examples_embedding_mixed_input_main
```

Expected output / ожидаемый вывод:

```text
mixed input: pointer=30,40 key snapshot copied; pending event cancelled
```

All calls use the creating VM thread. Context owns pending plain native copies;
producer snapshots may be reused immediately. No managed pointers escape, so
no GC pins are needed. Stop callbacks before context teardown, release context
before runtime, and preserve errors on partial acquisition. This example does
not register script listeners: delivery still updates pointer/Key polling.
See [Mouse listeners](../mouse_listeners/README.md) for rooted script receivers.

## Individual APIs / Отдельные API

| API | Parameters, result and ownership |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Writable `config`: initialize public defaults before acquiring a VM. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Borrow initialized config, return owned runtime through `out`; release after contexts. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Live runtime, writable `out`: acquire independent VM/FIFO state on caller thread. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_input_enqueue_mouse`](../../../include/flashvm/input_queue.h#L25) | Context, DOWN/MOVE/UP, finite world x/y: copy a transition; no callback/publication until pump; capacity/allocation errors retain FIFO. [Queue](../../../doc/INPUT_QUEUE.md). |
| [`fvm_input_enqueue_keyboard`](../../../include/flashvm/input_queue.h#L34) | Context, borrowed validated state, pressed=0/1, finite event-time world x/y: copy all native fields; producer may immediately reuse storage. [Queue](../../../doc/INPUT_QUEUE.md). |
| [`fvm_input_pump`](../../../include/flashvm/input_queue.h#L45) | Context, max_events (zero means entry batch), optional processed: synchronously dispatch in order on owner thread; additions wait for next pump, errors consume current and retain later entries. [Queue](../../../doc/INPUT_QUEUE.md). |
| [`fvm_input_get_keyboard`](../../../include/flashvm/keyboard.h#L71) | Context, writable state with exact struct_size: copy committed polling state without script/OS activity. [Keyboard](../../../doc/KEYBOARD.md). |
| [`fvm_input_get_pointer`](../../../include/flashvm/input.h#L75) | Context, distinct writable world x/y: copy current committed pointer, no allocation. [Input](../../../include/flashvm/input.h). |
| [`fvm_input_pending`](../../../include/flashvm/input_queue.h#L48) | Context, writable count: total pending entries across variants; invalid arguments preserve output. [Queue](../../../doc/INPUT_QUEUE.md). |
| [`fvm_input_clear_queue`](../../../include/flashvm/input_queue.h#L39) | Context: cancel pending entries without callbacks/state reset; invalidates active entry batch if called during delivery. [Queue](../../../doc/INPUT_QUEUE.md). |
| [`fvm_input_reset_keyboard`](../../../include/flashvm/keyboard.h#L73) | Context: clear held/latest/toggle polling state without synthesizing key callbacks. [Keyboard](../../../doc/KEYBOARD.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Acquired context: release FIFO and VM state after delivery stops; rejected during an active pump. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Acquired runtime after context destruction: release outer owner; no callback may still use it. [Embedding](../../../doc/EMBEDDING_API.md). |

Before enqueue, SDL/Clutter adapters must map their own event coordinates once,
select primary contact and suppress native duplicates. An OS worker must marshal
copies through a host-owned thread-safe channel before calling VM enqueue.
This change adds no automatic SDK adapter, physical-device/IME, automatic Flash
focus, audio clock or complete SWF/player acceptance. [Full RU+EN contract](../../../doc/INPUT_QUEUE.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_input_clear_queue` | function | [include/flashvm/input_queue.h:39](../../../include/flashvm/input_queue.h#L39) |
| `fvm_input_enqueue_keyboard` | function | [include/flashvm/input_queue.h:34](../../../include/flashvm/input_queue.h#L34) |
| `fvm_input_enqueue_mouse` | function | [include/flashvm/input_queue.h:25](../../../include/flashvm/input_queue.h#L25) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../../../include/flashvm/keyboard.h#L71) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../../../include/flashvm/input.h#L75) |
| `fvm_input_pending` | function | [include/flashvm/input_queue.h:48](../../../include/flashvm/input_queue.h#L48) |
| `fvm_input_pump` | function | [include/flashvm/input_queue.h:45](../../../include/flashvm/input_queue.h#L45) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:73](../../../include/flashvm/keyboard.h#L73) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
<!-- source-links:end -->
