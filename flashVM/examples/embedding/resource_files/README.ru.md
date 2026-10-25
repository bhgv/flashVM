# Файлы ресурсов

Собираемый C89 пример очереди через файловый C API без VM, SDL, сети и mount.
Показывает частичное чтение ответа по одному байту и явное владение.

```sh
cmake -S flashVM -B build-files -DFVM_BUILD_EXAMPLES=ON
cmake --build build-files --target flashvm_examples_embedding_resource_files_main
./build-files/flashvm_examples_embedding_resource_files_main
```

Внутри компонента используйте `cmake -S .`. Нужны C89 и CMake; цель
линкуется только с flashvm_srecord. Результат: `resource files: hello`.
FS заимствует диспетчер, файл принадлежит примеру. Все вызовы последовательны;
callback исполняется только pump. При ошибках закрывается даже queued запрос.
Файл закрывается до FS, FS до диспетчера. `1.1` верно только для свежего примера;
в приложении имена нужно перечислять. Один вызов на open; для следующего — reopen.
Это пример native echo, не реализация удалённого MovieClip или OS mount.

| API | Владение / исполнение |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Создаёт принадлежащий хосту native диспетчер без mutex hooks. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_register`](../../../include/flashvm/rdispatch.h#L115) | Копирует регистрацию, заимствует callback userdata до unregister/destroy. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Owner выдаёт явное READ право сессии. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rfs_create`](../../../include/flashvm/rfs.h#L55) | Создаёт FS, заимствует диспетчер и исключительные ID сессии. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rfs_walk`](../../../include/flashvm/rfs.h#L64) | Копирует проверенный дочерний узел без обращения к OS FS. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rfs_open`](../../../include/flashvm/rfs.h#L70) | Открывает один дескриптор операции с READ|WRITE. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rfs_write`](../../../include/flashvm/rfs.h#L91) | Копирует один запрос в очередь без исполнения callback. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rdispatch_pump`](../../../include/flashvm/rdispatch.h#L145) | Owner явно исполняет не больше одного queued callback. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rfs_read`](../../../include/flashvm/rfs.h#L83) | Копирует байт потока, удерживая остаток native ответа. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_sbindings_init`](../../../include/flashvm/sbindings.h#L37) | Заимствует один слот native RX привязки этого файла. [doc/SRECORD.ru.md](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_apply`](../../../include/flashvm/sbindings.h#L59) | Проверяет CRC объявления перед изменением RX таблицы. [doc/SRECORD.ru.md](../../../doc/SRECORD.ru.md). |
| [`fvm_rreply_record_read`](../../../include/flashvm/rwire.h#L48) | Проверяет типизированный ответ и копирует counted bytes. [doc/SRECORD.ru.md](../../../doc/SRECORD.ru.md). |
| [`fvm_rfs_close`](../../../include/flashvm/rfs.h#L99) | Отменяет queued работу или освобождает открытый дескриптор. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rfs_destroy`](../../../include/flashvm/rfs.h#L104) | Освобождает FS после закрытия файлов; заимствование заканчивается на успехе. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Освобождает остановленный диспетчер, отказывает при незабранных jobs. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |

[main.c](main.c) · [catalog](../../README.md#example-embedding-resource_files)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:145](../../../include/flashvm/rdispatch.h#L145) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rfs_close` | function | [include/flashvm/rfs.h:99](../../../include/flashvm/rfs.h#L99) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:55](../../../include/flashvm/rfs.h#L55) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:104](../../../include/flashvm/rfs.h#L104) |
| `fvm_rfs_open` | function | [include/flashvm/rfs.h:70](../../../include/flashvm/rfs.h#L70) |
| `fvm_rfs_read` | function | [include/flashvm/rfs.h:83](../../../include/flashvm/rfs.h#L83) |
| `fvm_rfs_walk` | function | [include/flashvm/rfs.h:64](../../../include/flashvm/rfs.h#L64) |
| `fvm_rfs_write` | function | [include/flashvm/rfs.h:91](../../../include/flashvm/rfs.h#L91) |
| `fvm_rreply_record_read` | function | [include/flashvm/rwire.h:48](../../../include/flashvm/rwire.h#L48) |
| `fvm_sbindings_apply` | function | [include/flashvm/sbindings.h:59](../../../include/flashvm/sbindings.h#L59) |
| `fvm_sbindings_init` | function | [include/flashvm/sbindings.h:37](../../../include/flashvm/sbindings.h#L37) |
<!-- source-links:end -->
