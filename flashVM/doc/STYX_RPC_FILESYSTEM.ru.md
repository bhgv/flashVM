# Дерево Styx как файловая система FlashVM

[English](STYX_RPC_FILESYSTEM.md) · [Связь FlashVM и DisVM](STYX_RPC.ru.md) · [C SDK](../../lib/styxrpc/reference/vfs_open.ru.md) · [Живые переменные VFS](STYX_RPC_LIVE_VARIABLES.ru.md)

Хост выбирает уже смонтированное подключение Styx. Подключите `flashvm_styxrpc` и передайте его в [fvm_styxrpc_filesystem_mount](STYX_RPC_FILESYSTEM_MOUNT.ru.md), например с точкой `/dis`. В том же контексте установите [файловые операции](FILESYSTEM_INSTALL.ru.md) под именем `fs`. Сценарий пользуется теми же операциями для локального файла и файла на Styx.

Для парной работы монтируйте в `/` в свежем контексте. Корень исключителен. Все пути сценария и обычные файловые каналы используют эту сессию до уничтожения контекста. Обычное представление исключает `.prog` из списков и отклоняет этот компонент на любой глубине. Типизированная привязка внешней переменной открывает выданный путь ячейки.

```actionscript
var rows = fs.list("/dis");
var info = fs.stat("/dis/data/sample");
var input = fs.open("/dis/data/sample", "r");
var bytes = input.read(64);
input.close();
```

`stat(path)` возвращает `[lengthHigh, lengthLow, mode, directory]`. Две беззнаковые части длины по 32 бита сохраняют точный размер. `list(path[, skip, limit])` возвращает массив до 32 дочерних записей. Каждая запись начинается с имени, затем содержит те же поля, что и `stat`. Для страницы укажите одновременно число пропущенных записей и предел от 1 до 32. Порядок задаёт файловый сервер, а страницы открывают каталог независимо.

`open(path[, mode])` по умолчанию открывает на чтение. На дереве Styx доступны `r`, `wo` для записи в существующий файл, `rwo` с общим курсором и `rw` с чтением от начала и дописыванием. Дописывание измеряет конец открытого файла перед записью. Режимы создания и усечения `w` и `wx`, переименование и удаление на Styx пока отсутствуют. Файл-каталог открывается через `list`.

Дескриптор поддерживает `read(count)`, `write(bytes)`, `seek(offset[, origin])` и `close()`. Массив ввода и вывода содержит до 4096 байтов, включая нули. Чтение на EOF даёт пустой Array. Успешная запись возвращает число байтов. `seek` использует начало (0), текущую позицию (1) или конец (2); режим `rw` использует отдельные позиции и отклоняет seek. Ошибка записи может оставить подтверждённый префикс. Мост прекращает запись на этом дескрипторе, если не подтверждён весь запрос.

`channel.create("file", path, mode)` и [fvm_channel_open_file_in](CHANNEL_OPEN_FILE_IN.ru.md) разрешают путь в том же контексте и сохраняют файловый протокол каналов. Удалённый файл держит отдельный `fid` от открытия до закрытия. [Снятие монтирования](FILESYSTEM_UNMOUNT.ru.md) меняет поиск новых путей; открытые файлы продолжают обращаться к своим fid. Внутри выбранной точки ошибка возвращается из её файловой системы. Для пути под точкой используйте непустые компоненты без `.` и `..`.

## Внешние переменные

[Привязка свойства](STYX_RPC_INT_VARIABLE_BIND.ru.md) соединяет имя в объекте ActionScript с точным файлом нативной переменной `int` в `.var`. Например, после привязки можно выполнить `trace(external.count)` и `external.count = 65539`. Значение хранится у владельца, а свойство читает или заменяет его через `libstyxrpc`. Для переменной только для чтения попытка присваивания вызывает исключение. Связь свойства с fid удерживается сборщиком мусора. [Контракт нативной ячейки](../../disVM/runtime/VFS-INT-VARIABLE.ru.md)

[Проверка файловых операций](../tests/styxrpc/vfs.c) работает с отдельным C-устройством. [Проверка переменной](../../disVM/native/terminal/tests/variable/guest.py) запускает нативную DisVM в интерпретаторе и JIT. В каждом режиме FlashVM видит обновление Limbo, записывает новое значение и оставляет соседнюю переменную только для чтения без изменений.
Для сквозной проверки передайте собранную DisVM, компилятор Limbo, SDK терминала и тестовый хост FlashVM:

```sh
python3 disVM/native/terminal/tests/variable/guest.py \
  build-disvm/disvm build-disvm/bootstrap/limbo-bootstrap \
  build-terminal-sdk build-flashvm/flashvm_tests_styxrpc_host
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_open_file_in` | function | [include/flashvm/channel.h:154](../include/flashvm/channel.h#L154) |
| `fvm_styxrpc_filesystem_mount` | function | [include/flashvm/styxrpc_filesystem.h:22](../include/flashvm/styxrpc_filesystem.h#L22) |
<!-- source-links:end -->
