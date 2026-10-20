# Selection: MovieClip eligibility / Допустимость выбора клипа

Runnable C89 example comparing rejection of a plain MovieClip, acceptance after
`focusEnabled=true`, counted target-path output, synchronous MovieClip callbacks, Selection observer registration and clearing with undefined.
Useful for an embedding host that wants the script-level eligibility policy,
rather than the lower-level unconditional host override. No window, SDL device,
permissions or SWF compiler is required. Each nonempty C line is commented;
errors use status returns rather than assertions.

Исполняемый C89-пример: сначала обычный клип отклоняется, затем разрешение
focusEnabled позволяет его выбрать. Пример получает путь и снимает выбор через
undefined, получая onSetFocus/onKillFocus. Он показывает правила скриптового Selection; для низкоуровневого
безусловного выбора см. соседний [пример слушателей](../keyboard_listeners/README.md).

## Build and run / Сборка и запуск

From a detached FlashVM component:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_selection_main
./build/flashvm_examples_embedding_selection_main
```

Expected / ожидается:

```text
default accepted=0
set other-null=1
observer old-null=1 new-null=0
enabled accepted=1
focus=_level0
kill other-null=1
observer old-null=0 new-null=1
cleared=1
observer removed=1
```

## Implementation and ownership / Реализация и владение

Install the standard library, then explicitly install Selection. Create/pin a
native clip before further allocation. The first setFocus returns false normally:
this is eligibility rejection, not a VM error. Set the Boolean focusEnabled,
select again, then getFocus returns a counted native path without invoking a
replaceable toString. Print the borrowed path before another allocating API call.
Clearing removes the selection root; the construction pin remains until cleanup.
Context destruction also releases partial setup after any error.

Установка Selection явная, после стандартных объектов. Pin защищает клип ещё
до успешного выбора. Boolean false от setFocus — штатный отказ выбора, а не
ошибка VM. getFocus возвращает заимствованную counted-строку; печать идёт до
следующего выделяющего вызова. Сброс снимает корень выбора, затем освобождается
pin и уничтожаются контекст/runtime. Владение строкой/объектами хосту не передаётся.

The observer is pinned during setup, registered strongly by identity, and explicitly
removed before its construction pin is released. Its onSetFocus receives old/new
objects (or null) after per-clip notifications. Each callback is synchronous and
keeps the ordinary native collector barrier. [Observer contract](../../../doc/SELECTION.md#selection-listeners).

Наблюдатель защищён construction pin, зарегистрирован и явно удалён перед
освобождением pin. Его onSetFocus получает старый/новый объект или null после
уведомлений клипов. Обработчики синхронные, нативный GC-барьер сохраняется.

## Demonstrated APIs / Используемые функции

| Function / API | Purpose and contract / Назначение и руководство |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned defaults; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquire the runtime before its context; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquire independent globals and focus; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | Prepare MovieClip constructors/prototypes; [SELECTION.md](../../../doc/SELECTION.md#installation). |
| [`fvm_standard_selection_install`](../../../include/flashvm/keyboard.h#L101) | Atomically publish only the supported Selection methods; [SELECTION.md](../../../doc/SELECTION.md#installation), [RU](../../../doc/SELECTION.ru.md#установка). |
| [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L69) | Create a borrowed native scene; [KEYBOARD_FOCUS.md](../../../doc/KEYBOARD_FOCUS.md#example-and-verification). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | Retain the clip before eligibility is configured; [GARBAGE_COLLECTION.md](../../../doc/GARBAGE_COLLECTION.md). |
| [`fvm_get_global`](../../../include/flashvm/value.h#L85) | Borrow the installed namespace; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | Initialize values and request clearing without manufacturing a null pointer; [SELECTION.md](../../../doc/SELECTION.md#setfocus). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | Invoke real script-visible methods with receiver/argument roots; [SELECTION.md](../../../doc/SELECTION.md#setfocus). |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | Register synchronous native focus handlers with borrowed static userdata; callbacks retain the ordinary GC barrier; [SELECTION.md](../../../doc/SELECTION.md#focus-notifications). |
| `Selection.addListener` | Retain an observer by identity once; synchronous, no OS input; [SELECTION.md](../../../doc/SELECTION.md#selection-listeners). |
| `Selection.removeListener` | Drop a registration and return a Boolean; an active recipient snapshot remains rooted; [SELECTION.md](../../../doc/SELECTION.md#selection-listeners). |
| [`fvm_object_create`](../../../include/flashvm/value.h#L62) | Create the context-owned observer before acquiring its construction pin; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| `Selection.setFocus` | Apply MovieClip eligibility or clear; [SELECTION.md](../../../doc/SELECTION.md#setfocus). |
| `Selection.getFocus` | Obtain a borrowed counted target path or null; [SELECTION.md](../../../doc/SELECTION.md#getfocus). |
| [`fvm_value_boolean`](../../../include/flashvm/value.h#L44) | Build the explicit focusEnabled flag; [API_INDEX.md](../../../doc/API_INDEX.md#values-and-borrowed-views). |
| [`fvm_object_set`](../../../include/flashvm/value.h#L74) | Publish the script eligibility property; [SELECTION.md](../../../doc/SELECTION.md#setfocus). |
| [`fvm_value_string_size`](../../../include/flashvm/value.h#L51) | Print counted path bytes without relying on a terminating NUL; [API_INDEX.md](../../../doc/API_INDEX.md#values-and-borrowed-views). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | Release the construction root before context cleanup; [GARBAGE_COLLECTION.md](../../../doc/GARBAGE_COLLECTION.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Drop selection and remaining managed objects on every path; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the final native owner; [EMBEDDING_API.md](../../../doc/EMBEDDING_API.md). |

## Limits / Ограничения

MovieClip-only host/SWF6–8 subset. No TextField caret/selection, Button,
focus rectangles, automatic
mouse/Tab navigation or security-origin policy. This example passes an object;
activation-relative paths are demonstrated by the shipped interactive SWF and
runtime fixtures. Ordinary native callbacks retain their collector barrier.
Selection and its focus root are host-local, not serialized or synchronized.

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
| `fvm_get_global` | function | [include/flashvm/value.h:85](../../../include/flashvm/value.h#L85) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:69](../../../include/flashvm/movieclip.h#L69) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../../../include/flashvm/value.h#L74) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../../../include/flashvm/keyboard.h#L101) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
| `fvm_value_string_size` | function | [include/flashvm/value.h:51](../../../include/flashvm/value.h#L51) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
