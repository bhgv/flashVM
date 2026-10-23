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
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | Install ordinary Object/Array and related builtins on the owner context for `--tabs`; may allocate and returns errors before the text tutorial starts. No device acquisition; repeated intact installation is idempotent. [Contract](../../../doc/STANDARD_OBJECTS.md). [Declaration](../../../include/flashvm/standard.h). |
| [`fvm_standard_text_install`](../../../include/flashvm/standard_text.h#L21) | Install the bounded TextFormat/TextField library on the owner context; check status before using constructors or fields. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_standard_text_register_font`](../../../include/flashvm/standard_text.h#L25) | Copy the supplied embedded font bytes into the context under Geometry; caller retains original fixture storage and duplicate conflicts are errors. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_standard_text_register_font_variant`](../../../include/flashvm/standard_text.h#L35) | Borrow context/family, style bits, counted font bytes and face index; synchronously validate/copy a real face into one slot. Context owns the copy. Owner-thread only, no GC/teardown calls; duplicate/unknown flags are INVALID_ARGUMENT, missing face use is NOT_FOUND, budget/profile is UNSUPPORTED, allocation is OUT_OF_MEMORY. No system lookup or synthetic fallback. [EN](../../../doc/TEXT_FONT_VARIANTS.md) / [RU](../../../doc/TEXT_FONT_VARIANTS.ru.md). |
| [`fvm_textfield_create`](../../../include/flashvm/standard_text.h#L41) | Create a VM-owned native field with explicit font and outer dimensions; root it through the caption global before later GC-capable operations. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
| [`fvm_textfield_path`](../../../include/flashvm/standard_text.h#L75) | Copy glyph/underline outlines into an independently owned native path; failures clear output and the caller destroys a successful path after VM teardown. [EN](../../../doc/TEXT_AUTOSIZE.md) / [RU](../../../doc/TEXT_AUTOSIZE.ru.md). |
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

## Real faces / Настоящие начертания (v0.875)

Run `--font-variants` to register three additional original CC0 faces, then select
bold/italic through the ordinary TextFormat object. At 40px, AV advances are
48.00/56.00/52.00/60.00 for regular/bold/italic/bold-italic. The original regular
face remains the default. Context-owned copies are released with the VM; the
final combined-face path is independently owned and destroyed separately.
The existing text-jit build/profile and owner-thread/GC requirements apply.
No SWF loader/display-list integration or physical-device acceptance is implied.

`--font-variants` регистрирует три дополнительных исходных CC0-лица и выбирает
bold/italic через TextFormat. При 40px AV даёт 48.00/56.00/52.00/60.00 для
regular/bold/italic/bold-italic. Regular остаётся default. Копии принадлежат
контексту, итоговый контур combined-face независим и освобождается отдельно.
Требования text-jit, потока-владельца и GC описаны выше. Это не проверка загрузки
шрифтов SWF, display list или реальных устройств.

## Paragraph inset mode / Поля абзаца

Run `flashvm_examples_embedding_standard_text_main --insets` with FONTS.
The same owner context applies leftMargin=10, rightMargin=12, blockIndent=8.
A 124px outer field leaves 90px of wrapping space: expected output is
`insets wrap: advance=72.00 height=94.00`, then `insets fit: width=166.00`.
The checked C89 source uses existing property/call APIs and ordinary errors;
the format stays globally rooted, and the exported path is separately owned.
NO_FONTS reports the existing unsupported profile. No window/device is involved.

Режим `--insets` применяет поля 10/12 и отступ блока 8 в том же потоке-владельце.
При ширине 124 перенос использует 90 пикселей. Ожидаются advance=72, height=94,
затем автоматическая ширина без переноса 166. Формат сохраняется глобальным
корнем, путь принадлежит вызывающему отдельно. Это не проверка устройства.
Контракт значений, ошибок, владения и ограничений: [EN](../../../doc/TEXT_INSETS.md) /
[RU](../../../doc/TEXT_INSETS.ru.md).

## First-line mode / Отступ первой строки

Run `flashvm_examples_embedding_standard_text_main --indent` in the FONTS profile.
It adds indent=18 to the existing left/right/block values 10/12/8. Expected:
`indent wrap: advance=90.00 height=94.00`, then `indent fit: width=184.00`.
The same property/call APIs borrow the globally rooted managed format; candidate
style and geometry publish only after success. Existing independent path export
and owner-context cleanup still run. NO_FONTS reports the existing unsupported
profile. No device or original Flash acceptance is claimed.

Режим добавляет отступ 18 к полям 10/12/8. Ожидаются advance=90, height=94,
затем ширина без переноса 184. Формат удерживается глобальным корнем; геометрия
публикуется после проверки, путь принадлежит вызывающему отдельно.
[Аргументы, ошибки и ограничения EN](../../../doc/TEXT_INDENT.md) /
[RU](../../../doc/TEXT_INDENT.ru.md).

## Managed tab table / Управляемая таблица tabStops

Run `flashvm_examples_embedding_standard_text_main --tabs` with FONTS.
The mode explicitly installs ordinary builtins, creates and globally roots Array
[40,80], applies a copied future style and assigns tabbed text. Expected:
`tabs fit: advance=104.00 width=108.00`, then
`tabs wrap: width=54.00 height=140.00`. Existing independent path export and owner
cleanup still run. NO_FONTS reports its existing profile limit; no device is opened.

Режим устанавливает Array, удерживает исходный массив глобальным корнем и
копирует позиции 40/80 в будущий стиль. Ожидаются ширина текста 104, автоширина
108, затем высота 140 при ширине поля 54. Путь принадлежит вызывающему отдельно.
[Контракт EN](../../../doc/TEXT_TABS.md) / [RU](../../../doc/TEXT_TABS.ru.md).

## Default tabs / Стандартная табуляция

Run `flashvm_examples_embedding_standard_text_main --default-tabs` with FONTS.
An empty managed tabStops Array selects four declared average widths, not a stored
custom table. Future formatting, full text assignment and autoSize produce
`default-tabs fit: advance=199.00 width=203.00`; at outer width 54 the five-line
result is `default-tabs wrap: width=54.00 height=232.00`. The ordinary path export
and context teardown retain their ownership rules. Missing average metrics,
bidi tabs and wrapped justify fail explicitly; reference/device parity remains open.
Режим сохраняет пустой Array в будущем стиле, измеряет default и переносит
широкие tab на отдельные строки. Контракт: [EN](../../../doc/TEXT_DEFAULT_TABS.md) /
[RU](../../../doc/TEXT_DEFAULT_TABS.ru.md). Все функции уже описаны в таблице API выше.

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
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_standard_text_install` | function | [include/flashvm/standard_text.h:21](../../../include/flashvm/standard_text.h#L21) |
| `fvm_standard_text_register_font` | function | [include/flashvm/standard_text.h:25](../../../include/flashvm/standard_text.h#L25) |
| `fvm_standard_text_register_font_variant` | function | [include/flashvm/standard_text.h:35](../../../include/flashvm/standard_text.h#L35) |
| `fvm_textfield_create` | function | [include/flashvm/standard_text.h:41](../../../include/flashvm/standard_text.h#L41) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:75](../../../include/flashvm/standard_text.h#L75) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
