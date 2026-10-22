# embedding/standard_text

Регистрирует шрифт, создаёт TextField и обращается к его свойствам и методам через обычные объекты VM. Извлекает векторный путь; AS-файл отдельно показывает редактирование текста.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_standard_text_main --parallel
./build-examples/flashvm_examples_embedding_standard_text_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_standard_text_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке описано действие каждой
функции и дан переход к руководству; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Назначение и документация |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned configuration defaults before runtime creation; no VM ownership is transferred. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_construct`](../../../include/flashvm/value.h#L158) | Invoke TextFormat construction on the context owner; returned format is VM-owned and must be rooted before later GC-capable calls. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Create an independent VM context owned by the caller; keep its runtime alive and destroy the context first. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release the context and managed fields/formats after all borrows end; exported native paths remain independent. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | Synchronously invoke setNewTextFormat, replaceText or getTextFormat with borrowed rooted receiver/arguments; inspect status and copy results before reuse. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_object_get`](../../../include/flashvm/value.h#L72) | Read field text, autoSize or dimensions through native accessors on the context owner; result views are borrowed and may be overwritten. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_object_set`](../../../include/flashvm/value.h#L74) | Apply field text, formatting, wrap policy or autoSize; automatic geometry is measured before publication and failed writes preserve field state. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | Release an owned exported path after use; null is accepted and VM destruction does not release this independent path. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Create caller-owned runtime state from borrowed initialized config; keep it alive until every context is destroyed. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release runtime resources after context destruction; do not retain runtime/context borrows afterwards. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_set_global`](../../../include/flashvm/value.h#L87) | Publish a copied value as a context global so the VM traces the managed format/field; borrowed strings are not host-owned objects. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_standard_text_install`](../../../include/flashvm/standard_text.h#L17) | Install the bounded TextFormat/TextField library on the owner context; check status before using constructors or fields. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_standard_text_register_font`](../../../include/flashvm/standard_text.h#L21) | Copy the supplied embedded font bytes into the context under Geometry; caller retains original fixture storage and duplicate conflicts are errors. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_textfield_create`](../../../include/flashvm/standard_text.h#L27) | Create a VM-owned native field with explicit font and outer dimensions; root it through the caption global before later GC-capable operations. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_textfield_path`](../../../include/flashvm/standard_text.h#L34) | Copy glyph/underline outlines into an independently owned native path; failures clear output and the caller destroys a successful path after VM teardown. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_value_boolean`](../../../include/flashvm/value.h#L44) | Write a normalized local Boolean value without allocation; true/false select left/none for autoSize. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | Write a local numeric value without allocation for dimensions, sizes or UTF-16 edit indices; field setters validate their own bounds. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_value_string`](../../../include/flashvm/value.h#L46) | Create a counted VM-compatible string value from the supplied zero-terminated text; check status before assigning it to a field. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | Initialize or reset a local value without allocation before explicitly constructing an object argument. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |

[Описание в общем каталоге](../../README.md#example-embedding-standard_text).
<!-- example-catalog-api:end -->

## Automatic size / Автоматический размер (v0.850)

```sh
cmake -S . -B build-text -DFVM_DEFS_FILE=cmake/profiles/text-jit.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-text --target flashvm_examples_embedding_standard_text_main
./build-text/flashvm_examples_embedding_standard_text_main --auto-size
```

The optional C89 path checks immediate left fitting (52x48), text growth (width
76) and fixed-width wrapping (30x140) with the repository font. It exports owned
paths before destroying the context/runtime; no window or physical device is used.
Unknown arguments fail before acquisition; NO_FONTS reports its prerequisite.
The AS companion is source for a host-created caption, not a compiled SWF or
MovieClip display integration. Contracts, supported anchors/dimension restrictions
and GC/thread ownership: [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md).

Режим C89 показывает немедленный размер 52x48, рост ширины до 76 и переносы
при фиксированной ширине 30 с высотой 140. Контур экспортируется до уничтожения
контекста/runtime и освобождается отдельно; окна и устройства не используются.
AS-файл — исходник для caption, созданного приложением, а не собранный SWF.

## Parent-space anchors / Опоры относительно родителя (v0.857)

Run the same binary with `--anchors`. After the existing geometry checks it
resets a box to x=10, width=100 and fits AV at 40px with each mode. Expected:
left x=10, center x=34, right x=58; all widths are 52. Only the scalar placement
changes; the exported independent path stays field-local. The host applies x/y
once. Existing owner-thread, GC, font prerequisites and API ownership above apply.

Запуск с `--anchors` после прежних проверок сбрасывает поле в x=10, width=100
и подбирает размер AV при 40px. Результат left/center/right: x=10/34/58,
ширина всегда 52. Независимый контур остаётся локальным; хост применяет x/y один
раз. Требования к шрифтам, потоку контекста, GC и владению API описаны выше.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_construct` | function | [include/flashvm/value.h:158](../../../include/flashvm/value.h#L158) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../../../include/flashvm/value.h#L74) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../../../include/flashvm/value.h#L87) |
| `fvm_standard_text_install` | function | [include/flashvm/standard_text.h:17](../../../include/flashvm/standard_text.h#L17) |
| `fvm_standard_text_register_font` | function | [include/flashvm/standard_text.h:21](../../../include/flashvm/standard_text.h#L21) |
| `fvm_textfield_create` | function | [include/flashvm/standard_text.h:27](../../../include/flashvm/standard_text.h#L27) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:34](../../../include/flashvm/standard_text.h#L34) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
