# Keyboard polling / Опрос клавиатуры

Runnable strict-C89 host example: publish two simultaneous held keys, query the
standard `Key` polling bridge, then clear input as on focus loss. No keyboard,
window, background thread or platform permissions are needed. The host supplies
synthetic input explicitly. This is useful when embedding a movie in a GUI,
replaying input or testing a VM independently from a device.

Исполняемый пример C89: публикация двух одновременно удерживаемых клавиш, опрос
`Key` и сброс при потере фокуса. Здесь моделируется ввод хоста; системные клавиши
и события слушателей не захватываются. Каждая непустая строка C прокомментирована.

## Build and run / Сборка и запуск

From the detached FlashVM component directory:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_keyboard_polling_main
./build/flashvm_examples_embedding_keyboard_polling_main
```

Expected output / ожидается:

```text
Key.isDown = true
Key.isDown = true
Key.isDown = false
Key.getCode = 0
```

## Implementation and ownership / Реализация и владение

`main.c` creates runtime/context, installs the standard library and explicitly
installs the supported Key namespace. A zeroed [`FVM_KeyboardState`](../../../include/flashvm/keyboard.h#L50) with exact
`struct_size` receives two held slots. Publication copies it; the host can reuse
its storage immediately. The helper invokes actual native Key methods through
[`fvm_object_call`](../../../include/flashvm/value.h#L126). Key remains rooted by its global binding; this example does
not replace globals or collect while retaining an unrooted object. Cleanup
releases context before runtime on both success and failure. No assertions.

Глобальный `Key` удерживает объект до разрушения контекста. При замене глобального
значения долгоживущий хост должен сам защитить сохраняемый объект от GC.
`reset` не генерирует вымышленных отпусканий клавиш. Установка не выполняется
автоматически основным standard API; SDL-плеер подключает её явно.

## Demonstrated APIs / Используемые функции

| Function | Purpose and contract / Назначение и руководство |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned default configuration; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquire the runtime owner; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquire an isolated context/input owner; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | Install ordinary standard objects before Key; [API_INDEX.md](../../../doc/API_INDEX.md). |
| [`fvm_standard_keyboard_install`](../../../include/flashvm/keyboard.h#L103) | Transactionally publish polling methods without opening an input device; [KEYBOARD.md](../../../doc/KEYBOARD.md#installation). |
| [`fvm_input_set_keyboard`](../../../include/flashvm/keyboard.h#L69) | Validate/copy all held slots, latest codes and toggles, with no event callback; [KEYBOARD.md](../../../doc/KEYBOARD.md#snapshot-api). |
| [`fvm_input_get_keyboard`](../../../include/flashvm/keyboard.h#L71) | Copy current context-local state into exact-layout caller storage; [KEYBOARD.md](../../../doc/KEYBOARD.md#snapshot-api). |
| [`fvm_input_reset_keyboard`](../../../include/flashvm/keyboard.h#L73) | Clear held keys/codes/toggles on focus loss without listener delivery; [KEYBOARD.md](../../../doc/KEYBOARD.md#snapshot-api). |
| [`fvm_get_global`](../../../include/flashvm/value.h#L85) | Borrow the globally rooted namespace; [API_INDEX.md](../../../doc/API_INDEX.md). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | Build the scalar virtual-key argument; [API_INDEX.md](../../../doc/API_INDEX.md#values-and-borrowed-views). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | Invoke each actual Key polling method synchronously; [KEYBOARD.md](../../../doc/KEYBOARD.md#script-methods). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release the VM and its globally rooted Key object; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the runtime after all its contexts; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |

## Limits / Ограничения

This polling example does not register listeners; [the companion example](../keyboard_listeners/README.md)
now covers them. No automatic clip/button key-event delivery, security origin
checks, IME/text composition or original-Flash conformance claim.
The host must marshal native input to the VM-owning thread. The example uses
C calls to the same bridge rather than compiling an ActionScript source file.
[Full EN contract](../../../doc/KEYBOARD.md) / [русский контракт](../../../doc/KEYBOARD.ru.md).

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
| `fvm_get_global` | function | [include/flashvm/value.h:85](../../../include/flashvm/value.h#L85) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../../../include/flashvm/keyboard.h#L71) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:73](../../../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:69](../../../include/flashvm/keyboard.h#L69) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:50](../../../include/flashvm/keyboard.h#L50) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:103](../../../include/flashvm/keyboard.h#L103) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
