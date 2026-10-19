# Primary-pointer listeners / Слушатели указателя

Runnable strict-C89 host tutorial: register one receiver, deliver down/move/up,
query each published position and remove the registration. Suitable for a host
mapping mouse, stylus or its chosen primary touch contact into Flash events.
No window, input permission, SWF compiler or physical mouse is required.

Исполняемый пример C89: регистрация одного объекта, начало/движение/завершение
контакта, чтение опубликованных координат, удаление подписки и очистка владельцев.
Хост может передавать касания или перо; физическая мышь не требуется.

## Build and run / Сборка и запуск

From the detached FlashVM component:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_mouse_listeners_main
./build/flashvm_examples_embedding_mouse_listeners_main
```

Expected output / ожидаемый вывод:

```text
event 1: 10,20
event 2: 30,40
event 3: 30,40
delivered=3 removed=1
```

## Implementation and ownership / Реализация и владение

All VM operations use the owning thread. The construction pin protects incomplete
setup; registration independently retains the listener graph. Native userdata is
a borrowed stack counter that stays alive until removal/context destruction.
Callbacks execute synchronously and retain the ordinary native GC barrier.
Position is already published when a handler queries it. Cleanup removes the
registration, releases the pin, destroys the context, then its runtime, including
partial setup after an error. No caller owns or frees a managed object directly.

Все операции выполняются в потоке VM. Pin защищает сборку объекта, регистрация
независимо удерживает его граф. Счётчик userdata заимствован из стека main до
удаления/уничтожения контекста. Обработчики синхронные, с обычным native-барьером
GC. Очистка учитывает частичный успех и сохраняет исходную ошибку.

## Demonstrated APIs / Используемые функции

| Function | Purpose, ownership and contract |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned configuration; [embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquire the runtime before its context; [embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquire independent VM state; [embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L18) | Install ordinary constructors, without opting into Mouse; [Mouse](../../../doc/MOUSE.md#installation). |
| [`fvm_standard_mouse_install`](../../../include/flashvm/mouse.h#L47) | Atomically publish the explicit script namespace; no OS access or callback; [Mouse](../../../doc/MOUSE.md#installation). |
| [`fvm_object_create`](../../../include/flashvm/value.h#L62) | Create a borrowed context-owned receiver before pinning; [embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | Acquire the temporary construction root; [GC](../../../doc/GARBAGE_COLLECTION.md). |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | Register native handlers with borrowed code/userdata and no capture root; [delivery](../../../doc/MOUSE.md#delivery). |
| [`fvm_input_add_mouse_listener`](../../../include/flashvm/mouse.h#L19) | Retain an identity once, without callbacks or devices; [native contract](../../../doc/MOUSE.md#native-host-contract). |
| [`fvm_input_dispatch_mouse`](../../../include/flashvm/mouse.h#L26) | Synchronously deliver one contact transition using finite world pixels; [delivery](../../../doc/MOUSE.md#delivery). |
| [`fvm_input_get_pointer`](../../../include/flashvm/input.h#L75) | Copy callback-visible coordinates without allocation; [native contract](../../../doc/MOUSE.md#native-host-contract). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | Initialize the ignored event return value; [delivery](../../../doc/MOUSE.md#delivery). |
| [`fvm_input_remove_mouse_listener`](../../../include/flashvm/mouse.h#L21) | Release a registration without allocation; an active snapshot stays rooted; [native contract](../../../doc/MOUSE.md#native-host-contract). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | Release the construction pin after the final borrowed access; [GC](../../../doc/GARBAGE_COLLECTION.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release remaining registrations and managed objects; [embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the outer owner after context cleanup; [embedding](../../../doc/EMBEDDING_API.md). |

## Limits / Ограничения

This tutorial supplies explicit primary-contact transitions, not raw multitouch
recognition. No cursor hide/show, wheel, Button/ClipActions, automatic focus,
security-origin policy, native OS/device or original-player acceptance is claimed.
События здесь задаёт хост; пример не распознаёт сырые касания и не проверяет
аппаратное устройство. [EN contract](../../../doc/MOUSE.md), [RU](../../../doc/MOUSE.ru.md).

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
| `fvm_input_add_mouse_listener` | function | [include/flashvm/mouse.h:19](../../../include/flashvm/mouse.h#L19) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:26](../../../include/flashvm/mouse.h#L26) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../../../include/flashvm/input.h#L75) |
| `fvm_input_remove_mouse_listener` | function | [include/flashvm/mouse.h:21](../../../include/flashvm/mouse.h#L21) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../../../include/flashvm/standard.h#L18) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:47](../../../include/flashvm/mouse.h#L47) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
