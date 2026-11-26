# fvm_styxrpc_session_bind

[English](STYX_RPC_SESSION.md) · [Заголовок](../include/flashvm/styxrpc_session.h) · [Исходник](../src/styxrpc/session.c) · [Закрытие](STYX_RPC_SESSION_CLOSE.ru.md)

Подключите `flashvm_styxrpc` и `flashvm/styxrpc_session.h`.

```c
FVM_Result fvm_styxrpc_session_bind(FVM_Context *context, FVM_StyxRpcSession *binding,
    struct StyxClient *client, const StyxRpcProgram *program, FVM_Value *object);
```

В потоке владельца контекста передайте обнулённую привязку в постоянной памяти
хоста и смонтированную сессию. Нулевой `program` разрешает работу с файлами
и вызовы именованных экспортов. Для именованного запуска укажите описатель с ненулевым PID во
встречном дереве этой сессии. Необязательный родительский PID и маска скрытия
дескрипторов копируются. Хост публикует `.var`, `.fd` и `.vfs` перед
обслуживанием соединения. Эти колбэки могут использовать данные хоста в
отдельном потоке диспетчера. Объекты FlashVM остаются в потоке владельца.

При успехе привязка владеет токенами запуска, а `object` получает новый
объект AS. Удержите его GC-корнем или сохраните до следующего выделения
объектов. Привязка однократная и заимствует клиент. Неверный вход даёт
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), остальные ошибки возвращают API выделения и
регистрации FlashVM. При отказе выход сохраняется.

| Вызов AS | Результат и условия |
| --- | --- |
| `launch(name, startup)` | Положительный токен при подтверждённой доставке, −1 при отказе до отправки, 0 при неопределённом исходе. Байты строки `startup` передаются с длиной, включая NUL. Предел полного кадра именованного сервиса — 64 байта с контекстом и обрамлением. |
| `status(token)` | 1 — ожидание, 2 — успешное завершение, 3 — ошибка программы, 0 — неопределённость. Подтверждённый конечный результат хранится до закрытия. |
| `close(token)` | True после отзыва точного токена и clunk. Для ожидающей задачи запрашивается отмена. |
| `stat(path)` | `[status, lengthHigh, lengthLow, mode, directory]`. Длина состоит из двух беззнаковых 32-битных частей. При ошибке метаданные равны нулю или false. |
| `read(path, offset, count)` | `[status, bytes]`. Плотный Array с подтверждённым префиксом до 2048 байтов. EOF даёт успешный короткий результат. |
| `write(path, offset, bytes)` | `[status, confirmedCount]`. Принимает до 2048 собственных индексных свойств со значениями 0–255. Геттеры могут выполняться до отправки. Успешный результат может означать короткую запись. |
| [`callInt(module, method, argument)`](STYX_RPC_SESSION_CALL_INT.ru.md) | Проверенный результат `fn(int): int`. |
| [`loadModule(path, ...methods)`](STYX_RPC_LOAD_MODULE.ru.md) | Проверить и загрузить методы. |
| [`intVariable(path, writable)`](STYX_RPC_INT_VARIABLE.ru.md) | Открыть прокси живой переменной. |
| [`openExchange(name)`](STYX_RPC_OPEN_EXCHANGE.ru.md) | Открыть обмен разными типами значений и живыми ссылками. |
| [`newnode(path, permissions, callback)`](STYX_RPC_NEWNODE.ru.md) | Опубликовать файл с колбэком в парном плеере. |
| [`openMessages(root, ...types)`](STYX_RPC_OPEN_MESSAGES.ru.md) | Открыть именованный порт UniBus. |

Одновременно открываются до восьми запусков. Положительные целые токены
точно представимы в AS и не переиспользуются в пределах привязки.
Неизвестный либо закрытый токен и неверные параметры дают ошибку аргумента.
Скрипт не может изменить заданный хостом PID, родителя или политику FD.

Пути относительно точки монтирования не могут содержать пустые компоненты,
`.` или `..`. Смещения — точные целые от 0 до 2⁵³−1. Статусы файловых
операций: 0 — подтверждённая передача, 1 — отказ до работающего доступа к
данным, 2 — подтверждённая ошибка, 3 — неопределённость. Подтверждённый
префикс сохраняет смысл при последующем сбое. Ошибка выделения в VM после
операции не разрешает её повтор. Запуск и файловые операции синхронны и
используют сроки сессии. Используйте поток владельца, который может ждать.

Привязка должна жить, пока доступны её методы. Закройте её до освобождения
сессии. См. [проверку запуска](../tests/styxrpc/launch.c) и
[проверку VFS](../tests/styxrpc/vfs.c).

Для обычных путей и открытых файлов используйте [монтирование файловой системы](STYX_RPC_FILESYSTEM.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_session_bind` | function | [include/flashvm/styxrpc_session.h:36](../include/flashvm/styxrpc_session.h#L36) |
| `FVM_StyxRpcSession` | struct | [include/flashvm/styxrpc_session.h:24](../include/flashvm/styxrpc_session.h#L24) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
