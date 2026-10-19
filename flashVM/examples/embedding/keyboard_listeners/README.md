# Keyboard listeners / Слушатели клавиатуры

Executable strict-C89 example of registering one context-local object and
synchronously delivering a complete press/release pair without a movie tick.
Useful for embedding a movie into an existing event loop and for replaying
short taps. No window, input device, microphone or platform permission is used.

Исполняемый пример C89: регистрирует объект-слушатель и доставляет нажатие и
отпускание подряд, без кадра между ними. Каждая непустая строка C объяснена
комментарием; ошибки обрабатываются обычными кодами возврата, без assertions.

## Build and run / Сборка и запуск

From the detached FlashVM component directory:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_keyboard_listeners_main
./build/flashvm_examples_embedding_keyboard_listeners_main
```

Expected / ожидается:

```text
event 1: Left down
event 2: Left up
removed=1 deliveries=2
focus selected=1
event 3: Left down
event 4: Left up
```

## Implementation and ownership / Реализация и владение

Install standard objects and Key, create/pin a managed receiver, define its two
native methods, then register it strongly with the context. The same receiver
is passed as `this` for either callback; both receive zero positional arguments.
Each callback reads the state already published by dispatch. Main owns the
counter borrowed as userdata until unregistration/context destruction.
The temporary pin also protects failure cleanup; context destruction removes
remaining registrations before main's storage leaves scope. No callback destroys
the VM, dispatches recursively, changes threads or keeps borrowed snapshot pointers.

`set_keyboard` alone is polling publication and sends no events. Here dispatch
is explicit; the host supplies complete post-transition state. A callback error
stops the remaining recipients but keeps already published state and effects;
retrying that event can duplicate earlier callbacks. Listener additions/removals
affect the next delivery's recipient snapshot.

В SDL-плеере переходы сохраняются в ограниченной очереди и доставляются до
movie tick. Здесь очередь не нужна: синхронный C-хост сам задаёт порядок.
[Полный EN-контракт](../../../doc/KEYBOARD_LISTENERS.md) /
[RU](../../../doc/KEYBOARD_LISTENERS.ru.md).

## Demonstrated APIs / Используемые функции

| Function | Purpose and guide / Назначение и руководство |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned defaults; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquire the runtime owner; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquire independent state/registrations; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | Install standard objects before Key; [KEYBOARD.md](../../../doc/KEYBOARD.md#installation). |
| [`fvm_standard_keyboard_install`](../../../include/flashvm/keyboard.h#L95) | Explicitly publish Key methods and inspection getter; [KEYBOARD_LISTENERS.md](../../../doc/KEYBOARD_LISTENERS.md#script-interface). |
| [`fvm_object_create`](../../../include/flashvm/value.h#L62) | Create the managed callback receiver; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | Protect construction-time object lifetime; [GARBAGE_COLLECTION.md](../../../doc/GARBAGE_COLLECTION.md). |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | Define zero-argument receiver-bound handlers, borrowing userdata; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_input_add_key_listener`](../../../include/flashvm/keyboard.h#L70) | Retain one unique same-context object; [KEYBOARD_LISTENERS.md](../../../doc/KEYBOARD_LISTENERS.md#native-api). |
| [`fvm_input_dispatch_keyboard`](../../../include/flashvm/keyboard.h#L82) | Publish a snapshot then deliver one transition synchronously; [KEYBOARD_LISTENERS.md](../../../doc/KEYBOARD_LISTENERS.md#native-api). |
| [`fvm_input_get_keyboard`](../../../include/flashvm/keyboard.h#L63) | Read already published input without allocation; [KEYBOARD.md](../../../doc/KEYBOARD.md#snapshot-api). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | Initialize the ignored callback result; [API_INDEX.md](../../../doc/API_INDEX.md#values-and-borrowed-views). |
| [`fvm_input_remove_key_listener`](../../../include/flashvm/keyboard.h#L72) | Remove by identity before releasing borrowed host userdata; [KEYBOARD_LISTENERS.md](../../../doc/KEYBOARD_LISTENERS.md#native-api). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | Drop the temporary construction root; [GARBAGE_COLLECTION.md](../../../doc/GARBAGE_COLLECTION.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release remaining registrations and managed objects; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the runtime after its context; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |

## Explicit clip selection / Явный выбор клипа

After unregistering the listener, create a native MovieClip, immediately select
it as a strong context root, bind the same two callbacks and deliver another tap.
The selected clip receives both calls with itself as `this`. Querying focus returns
a borrowed pointer, then null clearing drops the selection root before cleanup.
No automatic script eligibility or OS selection policy is exercised.

После снятия слушателя пример создаёт клип, удерживает его выбором хоста,
назначает те же callbacks, доставляет ещё одно нажатие/отпускание и снимает выбор.
Проверка выбора возвращает заимствованный указатель. Автоматического выбора
мышью/Tab и реализации Selection здесь нет.

| Function | Purpose and contract / Назначение и контракт |
| --- | --- |
| [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L36) | Create a native scene receiver; [KEYBOARD_FOCUS.md](../../../doc/KEYBOARD_FOCUS.md#example-and-verification). |
| [`fvm_input_set_keyboard_focus`](../../../include/flashvm/keyboard.h#L75) | Replace/clear a strongly rooted native clip selection without sending events; [KEYBOARD_FOCUS.md](../../../doc/KEYBOARD_FOCUS.md#fvm_input_set_keyboard_focus), [RU](../../../doc/KEYBOARD_FOCUS.ru.md#fvm_input_set_keyboard_focus). |
| [`fvm_input_get_keyboard_focus`](../../../include/flashvm/keyboard.h#L78) | Borrow the selection, lazily clearing detached clips; [KEYBOARD_FOCUS.md](../../../doc/KEYBOARD_FOCUS.md#fvm_input_get_keyboard_focus), [RU](../../../doc/KEYBOARD_FOCUS.ru.md#fvm_input_get_keyboard_focus). |

## Limits / Ограничения

This demonstrates Key listeners and explicit host clip selection. Automatic
clip/button selection, text/IME composition and cross-domain security remain open. Key registration
and physical keyboard state are host-local, not transferred with serialized graphs.
Native ordinary callbacks retain the normal GC barrier; script callbacks use the
existing interpreter/JIT/GC rules. Neither this example nor the SDL FIFO is
thread-safe. Original-Flash mutation/array-identity conformance remains open.


Since v0.618, registration and delivery share the AVM1 work budget. Preparation
failure preserves valid state; later failure preserves committed effects.
[EN contract](../../../doc/INPUT_EVENT_BUDGET.md) / [RU](../../../doc/INPUT_EVENT_BUDGET.ru.md).

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
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:70](../../../include/flashvm/keyboard.h#L70) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:82](../../../include/flashvm/keyboard.h#L82) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:63](../../../include/flashvm/keyboard.h#L63) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:78](../../../include/flashvm/keyboard.h#L78) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:72](../../../include/flashvm/keyboard.h#L72) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:75](../../../include/flashvm/keyboard.h#L75) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:36](../../../include/flashvm/movieclip.h#L36) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:95](../../../include/flashvm/keyboard.h#L95) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
