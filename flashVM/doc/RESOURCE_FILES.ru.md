# Переносимые файлы ресурсов

C89 API `rfs.h` представляет ресурсы диспетчера каталогами и файлами. Его можно
вызывать прямо из IDE, плеера и C/C++ без консоли, VM, сокетов, потоков и OS mount.
Он входит в flashvm_srecord и основной SDK. [EN](RESOURCE_FILES.md),
[очереди](RESOURCE_EVENTS.ru.md), [диспетчер](RESOURCE_DISPATCH.ru.md),
[пример](../examples/embedding/resource_files/README.ru.md).

## Дерево и идентичность

Корень содержит видимые сессии ресурсы с именами `slot.generation`. В каждом:
`path` — исходный путь регистрации без завершающего NUL; `ops` — каталог
разрешённых числовых ID операций; `events` — поток при наличии SUBSCRIBE.
Например, `/1.1/path`, `/1.1/ops/7`, `/1.1/events`. В настоящем приложении имена
нужно получать перечислением, а не предполагать порядок регистрации.
Начальные нули, знаки, косая черта внутри компонента и обращения к OS FS
не допускаются. `..` переводит в родительский каталог; родитель корня — корень.

Это начальное дерево идентичностей, ещё не именованное дерево сцены/виджетов,
пространство имён Inferno, mount или полноценный 9P сервер. Старый дескриптор
не заменяется новым ресурсом с тем же исходным путём. Qid состоит из поколения
ресурса и номера слота/неизменяемой позиции операции в серверной таблице;
фильтрация прав и разные сессии не меняют идентичность. Персистентности после
перезапуска нет. Нативные структуры нельзя передавать в сеть целиком.

## API и владение

Все вызовы неблокирующие. Хост последовательно вызывает API одного FS;
диспетчер может отдельно использовать свои mutex hooks. Управляемых указателей
и корней GC нет. Проверки прав линеаризуются на захватах mutex диспетчера;
отзыв прав не прерывает уже исполняемый callback. Ошибки сохраняют выходы,
кроме отдельно оговорённого. Входные/выходные буферы не должны перекрываться
друг с другом или внутренней памятью FS.

| Функция | Контракт |
| --- | --- |
| [`fvm_rfs_create`](../include/flashvm/rfs.h#L49) | Диспетчер, аутентифицированная ненулевая Uint32 сессия, выходной указатель. Одно фиксированное native выделение; NOMEM сохраняет выход. Диспетчер заимствуется до destroy. Пространство request ID этой сессии исключительно принадлежит этому FS, включая прямые вызовы диспетчера. Права и аутентификация не создаются. |
| [`fvm_rfs_stat`](../include/flashvm/rfs.h#L51) | FS, канонический node, выходная копия entry. Повторно проверяет поколение/права. UNKNOWN для отсутствующего/невидимого ресурса, DENIED для запрещённого дочернего узла, INVALID для некорректного node. Заимствованных объектов в выходе нет. |
| [`fvm_rfs_walk`](../include/flashvm/rfs.h#L54) | FS, node родительского каталога, один NUL-terminated компонент, выход. Сначала проверяет родителя; поддерживает `..`, имена ресурсов и разрешённых детей. Проход через файл INVALID. Нет выделения, I/O и VM вызовов. |
| [`fvm_rfs_open`](../include/flashvm/rfs.h#L58) | FS, node, точный mode, выходной дескриптор. Каталоги/PATH/EVENTS требуют READ, CALL — READ|WRITE. Один из [FVM_RFS_FILES](../include/flashvm/rfs.h#L5) слотов; EVENTS дополнительно занимает независимую подписку. Неверный mode DENIED, исчерпание LIMIT. При ошибке дескриптор не опубликован. |
| [`fvm_rfs_readdir`](../include/flashvm/rfs.h#L62) | FS, открытый каталог, индекс от нуля, выход entry. OK даёт одну копию; PENDING означает конец. Изменение любого видимого ресурса/прав/фильтра операций даёт CONFLICT до переоткрытия, включая посторонние видимые ресурсы; отозванная запись не утекает из старого снимка. |
| [`fvm_rfs_read`](../include/flashvm/rfs.h#L67) | FS, дескриптор, Uint32 offset, непустой выходной буфер и счётчик. PATH допускает произвольное смещение и EOF. CALL/EVENTS требуют точное следующее смещение. PENDING — ответа/события ещё нет. Частичная запись хранится в дескрипторе, ошибки сохраняют буфер/счётчик. Нулевая ёмкость INVALID; каталоги читаются readdir. |
| [`fvm_rfs_write`](../include/flashvm/rfs.h#L73) | FS, CALL дескриптор, offset ноль, counted payload, выходной счётчик. Ровно один запрос на open, включая пустой; не больше [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9). Копирует и ставит в очередь с возрастающим Uint32 ID. OK не означает исполнение. Повтор CONFLICT, исчерпание ID LIMIT. Ошибка post сохраняет состояние файла/счётчик. |
| [`fvm_rfs_close`](../include/flashvm/rfs.h#L79) | FS и дескриптор. Отменяет ещё не начатый запрос, забирает удержанный ответ, закрывает только свою подписку. Исполненные эффекты остаются. Исполняемый callback даёт BUSY, файл остаётся открытым для повтора. Старый дескриптор UNKNOWN. Уже удалённые watch/request допустимы при очистке. Сессию/VM не уничтожает. |
| [`fvm_rfs_destroy`](../include/flashvm/rfs.h#L82) | Указатель на FS после остановки пользователей. Открытый файл BUSY. Успех освобождает только FS и обнуляет указатель; диспетчер, права и регистрации принадлежат хосту. |
| [`fvm_rdispatch_operation_index`](../include/flashvm/revents.h#L48) | Диспетчер, сессия, поколение ресурса, ID операции, выходной индекс. Под mutex проверяет право и возвращает неизменяемую позицию серверной таблицы от нуля. UNKNOWN ресурс/операция, DENIED право. Очередь не меняет; индекс не является правом и не индексирует фильтрованный discovery. |
| [`fvm_rdispatch_watch_check`](../include/flashvm/revents.h#L69) | Диспетчер, сессия, watch. Проверяет поколение/владельца, не забирая событие. Отозванная/пересозданная подписка UNKNOWN, другая сессия DENIED. Защищает остаток частично прочитанного события. |

## Вызовы, события и очистка

Запись передаёт сырые байты параметров операции. Затем владелец явно вызывает
pump. Read даёт PENDING до terminal reply. Ответ начинается с tag-zero объявления
[FVM_RFS_TAG](../include/flashvm/rfs.h#L17) как `flashvm.resource.reply`, затем идёт одна S-запись с CRC. Читать
можно частями с накопительным offset; после ответа EOF. Для следующего вызова
файл переоткрывается. Когда ответ скопирован в файл, dispatcher take подтверждает
его ID и освобождает очередь. Close до этого отменяет queued или забирает done;
после этого отбрасывает непрочитанные байты. Гарантии exactly-once после take нет.

Каждый EVENTS open имеет свою очередь. В начале — tag-zero объявление
`flashvm.resource.event`, далее S-записи; read может делить их на части.
Отсутствие события означает PENDING, а не EOF. Потери `missed` независимы.
Даже оставшиеся в буфере байты требуют прежнего поколения watch: revoke/regrant
не восстанавливает старый поток. Перед исчерпанием Uint32 offset возвращается
LIMIT без изъятия следующего события; close/open создаёт новый поток. Все числа
в записях используют [S-кодирование](SRECORD.ru.md), не native layout.

Каталоги содержат согласованный снимок. После CONFLICT закрыть, переоткрыть и
начать перечисление заново. Закрыть все файлы, остановить пользователей, удалить
FS, затем при необходимости отключить сессию на owner и уничтожить диспетчер.
Пока FS существует, диспетчер заимствован и должен оставаться живым.
Лимиты: [FVM_RFS_FILES](../include/flashvm/rfs.h#L5) дескрипторов, [FVM_RFS_BUFFER](../include/flashvm/rfs.h#L7) байт в каждом, общий предел
подписок диспетчера [FVM_R_WATCHES](../include/flashvm/revents.h#L5). После create выделений памяти нет.

## Сборка и проверка

```sh
cmake -S flashVM -B build-files -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build-files --target flashvm_rfs_contract flashvm_examples_embedding_resource_files_main
ctest --test-dir build-files -R '^flashvm_rfs_' --output-on-failure
```

C89 тест проверяет фрагменты ответа, независимые события, revoke/regrant,
стабильные qid при фильтрации сессий, устаревшие каталоги/поколения, отмену queued,
BUSY при running close, пустой payload, исчерпание слотов/ID, отказ allocation и
атомарность выходов. Это native протокольная проверка, не запуск на чужой OS,
полная совместимость Flash, сетевой listener или mount.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `FVM_R_WATCHES` | macro / define | [include/flashvm/revents.h:5](../include/flashvm/revents.h#L5) |
| `fvm_rdispatch_operation_index` | function | [include/flashvm/revents.h:48](../include/flashvm/revents.h#L48) |
| `fvm_rdispatch_watch_check` | function | [include/flashvm/revents.h:69](../include/flashvm/revents.h#L69) |
| `FVM_RFS_BUFFER` | macro / define | [include/flashvm/rfs.h:7](../include/flashvm/rfs.h#L7) |
| `fvm_rfs_close` | function | [include/flashvm/rfs.h:79](../include/flashvm/rfs.h#L79) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:49](../include/flashvm/rfs.h#L49) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:82](../include/flashvm/rfs.h#L82) |
| `FVM_RFS_FILES` | macro / define | [include/flashvm/rfs.h:5](../include/flashvm/rfs.h#L5) |
| `fvm_rfs_open` | function | [include/flashvm/rfs.h:58](../include/flashvm/rfs.h#L58) |
| `fvm_rfs_read` | function | [include/flashvm/rfs.h:67](../include/flashvm/rfs.h#L67) |
| `fvm_rfs_readdir` | function | [include/flashvm/rfs.h:62](../include/flashvm/rfs.h#L62) |
| `fvm_rfs_stat` | function | [include/flashvm/rfs.h:51](../include/flashvm/rfs.h#L51) |
| `FVM_RFS_TAG` | macro / define | [include/flashvm/rfs.h:17](../include/flashvm/rfs.h#L17) |
| `fvm_rfs_walk` | function | [include/flashvm/rfs.h:54](../include/flashvm/rfs.h#L54) |
| `fvm_rfs_write` | function | [include/flashvm/rfs.h:73](../include/flashvm/rfs.h#L73) |
<!-- source-links:end -->
