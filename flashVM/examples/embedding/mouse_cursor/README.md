# Cursor visibility request / Запрос видимости курсора

[main.c](main.c) is a complete C89 tutorial with each line commented. It creates
an independent context, checks initial visible request, requests hide twice,
restores visible and reports the previous states, then calls script-visible hide/show. It opens no window or device.

Русский: исполняемый пример проверяет запрос видимости контекста, а не настоящий
курсор ОС. Начальное значение — видим; повторное скрытие возвращает прежний
скрытый запрос. Хост применяет прочитанное значение к собственному окну отдельно.
Сенсорному или headless хосту создавать физическую мышь/курсор не требуется.

## Build and result / Сборка и результат

From the FlashVM component directory, any ordinary profile is sufficient:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_mouse_cursor_main -j2
./build/flashvm_examples_embedding_mouse_cursor_main
```

Expected: `previous=1,0,0 requested=1`, followed by `script previous=1,0`, exit 0. Failures are reported to stderr
with their actual status and exit 1. No SDK, graphical backend or permission is
required. This does not certify a physical cursor or native OS display.

## Ownership and API / Владение и API

All functions execute on the context owner thread outside collector/release/
teardown. Stack config/outputs belong to main; context borrows runtime lifetime.
No managed object escapes, no GC pin is needed. Requests allocate nothing, invoke
no script/host callback, and leave position, keys, focus, subscriptions and queued
events unchanged. Invalid arguments preserve both request and output. Request
persists until explicit replacement or context destruction, including movie loads.
See [full EN contract](../../../doc/MOUSE.md#cursor-visibility-request) and
[RU](../../../doc/MOUSE.ru.md#запрос-видимости-курсора).

| API | Parameters, result and ownership |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Writable config: initialize supported ABI defaults; acquire no owner. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Initialized config, runtime output: acquire outer owner; release after its contexts. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Live runtime, context output: independent requested visibility initially 1. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_input_get_cursor_visible`](../../../include/flashvm/mouse.h#L41) | Live context, mandatory writable int output: copy requested 0/1, not OS state; invalid call preserves output; [FVM_OK](../../../include/flashvm/flashvm.h#L42)/FVM_INVALID_ARGUMENT only. [Contract](../../../doc/MOUSE.md#cursor-visibility-request). |
| [`fvm_input_set_cursor_visible`](../../../include/flashvm/mouse.h#L46) | Context, visible=0/1, mandatory writable previous int: set request and return old 0/1; invalid call preserves state/output, no allocation/OS callback. [Contract](../../../doc/MOUSE.md#cursor-visibility-request). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Acquired context: destroy only after no callback uses it; invalidate its state. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Acquired runtime: release after context cleanup, including partial setup failure. [Embedding](../../../doc/EMBEDDING_API.md). |

Русский: таблица описывает каждую функцию отдельно. В этом примере нет скриптов
или объектов GC: достаточно освободить context, затем runtime. printf/fprintf
печатают только собственные скаляры. Для оконного применения запросов нужен
отдельный backend; успех VM не выдаётся за успех SDK/аппаратного курсора.
## Script-visible calls / Скриптовые вызовы (v0.609)

The example also installs standard objects and Mouse, borrows the globally rooted
namespace and calls hide/show with no arguments. Both return Number with the
previous requested state (1 then 0); no native cursor SDK is invoked. The namespace
is rooted by its global binding throughout these stateless calls. No managed
result escapes, so no additional pin is required. The [script contract](../../../doc/MOUSE.md#script-hide-and-show)
explains SWF5 cursor support, SWF6 listener guards and ignored arguments.

| API | Parameters and behavior |
| --- | --- |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L18) | Live context: install ordinary objects; real allocation errors propagate. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_standard_mouse_install`](../../../include/flashvm/mouse.h#L50) | Standard-installed context: atomically install cursor/listener subset, preserve conflicts; version gates remain explicit. [Mouse](../../../doc/MOUSE.md#script-hide-and-show). |
| [`fvm_get_global`](../../../include/flashvm/value.h#L85) | Context, name Mouse, writable value output: borrow namespace retained by global binding, no host ownership transfer. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | Context, borrowed receiver, hide/show name, count=0, NULL args, writable out: ordinary rooted native method lookup, return Number previous request; real lookup/callback errors propagate. [Mouse](../../../doc/MOUSE.md#script-hide-and-show). |

Русский: второй этап примера вызывает те же методы, что доступны фильму. Возврат
проверяется как Number, затем копируется в скаляр. Преобразования аргументов,
курсор ОС и графический backend не требуются; таблица содержит отдельные контракты
добавленных функций. Обычный поиск метода может выделять память, сам переход — нет.

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
| `fvm_input_get_cursor_visible` | function | [include/flashvm/mouse.h:41](../../../include/flashvm/mouse.h#L41) |
| `fvm_input_set_cursor_visible` | function | [include/flashvm/mouse.h:46](../../../include/flashvm/mouse.h#L46) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../../../include/flashvm/flashvm.h#L45) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../../../include/flashvm/flashvm.h#L42) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../../../include/flashvm/standard.h#L18) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:50](../../../include/flashvm/mouse.h#L50) |
<!-- source-links:end -->
