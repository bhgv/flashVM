# Двоичный обмен с разными схемами направлений

[English](README.md) · [Каталог](../../README.md#example-embedding-srecords) · [Контракт](../../../doc/SRECORD.ru.md).

Собираемый C89-пример без VM/SDL показывает передачу metadata/value, приём по одному
байту и обычные fwrite/fread. В A→B тег 3 объявлен ieee754.f64.be, в B→A — boolean.
Это две независимые модели направления. Опциональный файл содержит два их
транскрипта подряд для демонстрации; реальному multiplex transport нужен явный
конверт направления, в формате записи он не подразумевается.

## Build / Сборка

```sh
cmake -S flashVM -B build-srecord -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-srecord --target flashvm_examples_embedding_srecords_main \
  flashvm_srecord_oracle flashvm_srecord_cpp98 flashvm_srecord_fuzz --parallel
./build-srecord/flashvm_examples_embedding_srecords_main
./build-srecord/flashvm_examples_embedding_srecords_main transcript.bin
ctest --test-dir build-srecord -R '^flashvm_srecord_' --output-on-failure
```

Нужны C89/C++98 компиляторы и CMake; Python 3 нужен независимым oracle-тестам,
не C-примеру. Из отделённого flashVM-компонента используйте -S . и свой
абсолютный путь cmake/profiles/minimal.defs. Сохраните общую MIT LICENSE.

## Expected / Ожидается

```text
tag=3 type=ieee754.f64.be bytes=8
tag=3 type=boolean bytes=1
```

Код 0 означает успешную проверку обеих моделей и, если задан файл, его запись/
чтение. Код 1 — ошибка протокола/I/O, 2 — неверные аргументы. Файл явно создаётся/
перезаписывается, содержит NUL и не читается как текст. При ошибке вывода может
остаться неполный файл. Все FILE закрываются. Пути следуют правилам native CRT;
Windows UTF-16 adapter здесь не добавлен.

Float64 1.25 дан точными big-endian байтами 3f f4 00 00 00 00 00 00, без memcpy
native double. Boolean true использует каноническое целое 81. Имена схем в этом
примере объясняют payload; codec не исполняет обработчики схем автоматически.

## Владение и ограничения

Все buffers, tables и metadata принадлежат стеку хоста. Входы/выходы раздельны.
Apply копирует type identity, view заимствует receive до reset, lookup копирует
metadata. [GC:] VM runtime/объекты/pins вообще не создаются. Локальные вызовы
без locks; общий доступ к этим объектам потребовал бы синхронизации хоста.

Это обычный файл двоичных записей, не смонтированная удалённая ФС. Пользовательский
граф со shared references/cycles, dispatcher ресурсов, sockets, права и Inferno
mount/bind пока не реализованы. Принятый план: [namespace](../../../doc/RESOURCE_NAMESPACE.ru.md).

## C API

| API | Назначение и владение |
| --- | --- |
| [`fvm_sbindings_init`](../../../include/flashvm/sbindings.h#L33) | Создать пустую входящую таблицу в slots хоста, без выделений. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_payload_write`](../../../include/flashvm/sbindings.h#L43) | Закодировать список деклараций в отдельный буфер после полной проверки. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_srecord_write`](../../../include/flashvm/srecord.h#L66) | Обрамить metadata/value канонической длиной/тегом/checksum; вход заимствован лишь на время вызова. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_apply`](../../../include/flashvm/sbindings.h#L49) | Проверить тег 0 и атомарно скопировать привязки; конфликт оставляет таблицу прежней. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sstream_init`](../../../include/flashvm/sstream.h#L24) | Связать state с receive-буфером и лимитом; без I/O. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sstream_push`](../../../include/flashvm/sstream.h#L33) | Сохранить фрагмент; только OK публикует view. MORE также сообщает consumed. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_lookup`](../../../include/flashvm/sbindings.h#L37) | Скопировать тип/версию входящего направления, без глобальной трактовки тегов. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sstream_reset`](../../../include/flashvm/sstream.h#L38) | После последнего использования инвалидировать view, сохранить storage, сбросить state. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_srecord_read`](../../../include/flashvm/srecord.h#L72) | Проверить одну запись в байтах fread, вернуть точный consumed; view заимствует вход. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |

## Cross / Другие OS и CPU

```sh
cmake -S flashVM -B build-srecord-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-srecord-target --target flashvm_examples_embedding_srecords_main --parallel
```

Нужны headers/libc/linker именно target. Протокол использует явные байты и
32-битные limbs, не размер unsigned long и не native endian. Cross-link ARM64/
RISC-V64/Windows x64 не означает запуск на устройствах. Нативная проверка ОС,
обрывов, прав ФС и будущих adapters проводится отдельно.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sbindings_apply` | function | [include/flashvm/sbindings.h:49](../../../include/flashvm/sbindings.h#L49) |
| `fvm_sbindings_init` | function | [include/flashvm/sbindings.h:33](../../../include/flashvm/sbindings.h#L33) |
| `fvm_sbindings_lookup` | function | [include/flashvm/sbindings.h:37](../../../include/flashvm/sbindings.h#L37) |
| `fvm_sbindings_payload_write` | function | [include/flashvm/sbindings.h:43](../../../include/flashvm/sbindings.h#L43) |
| `fvm_srecord_read` | function | [include/flashvm/srecord.h:72](../../../include/flashvm/srecord.h#L72) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../../../include/flashvm/srecord.h#L66) |
| `fvm_sstream_init` | function | [include/flashvm/sstream.h:24](../../../include/flashvm/sstream.h#L24) |
| `fvm_sstream_push` | function | [include/flashvm/sstream.h:33](../../../include/flashvm/sstream.h#L33) |
| `fvm_sstream_reset` | function | [include/flashvm/sstream.h:38](../../../include/flashvm/sstream.h#L38) |
<!-- source-links:end -->
