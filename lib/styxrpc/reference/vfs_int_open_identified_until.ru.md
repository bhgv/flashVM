# styxrpc_vfs_int_open_identified_until

[English](vfs_int_open_identified_until.md) · [Объявление](../include/styxrpc/vfs_client.h#L240) · [Реализация](../src/vfs_int.c#L11)

Открыть смонтированную переменную Int32 и получить идентификатор открытого файла.

## Синтаксис

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_int_open_identified_until(struct StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out, uint64_t *identity, StyxRpcVfsStop stop, void *state);
```

## Доступность

Хостовый SDK C, библиотека `StyxRpc::launch`. Требуется подключённый клиент Styx и поддержка Int32-файлов `.var` со стороны владельца. Примеры проверены на Linux64 c DisVM.

## Параметры

| Имя | Описание |
| --- | --- |
| `session` | Заимствованный подключённый клиент Styx выбранного дерева. |
| `path` | Относительный путь длиной от 1 до 1024 байт с завершающим NUL. Файл расположен прямо в каталоге `.var`. Пустые компоненты, `.` и `..` недопустимы. |
| `writable` | 0 для чтения, 1 для чтения и записи. |
| `out` | Обязательный выход для собственного fid. |
| `identity` | Необязательный выход для полного беззнакового 64-битного `Qid.path` открытого fid. |
| `stop` | Необязательный колбэк. Выполняется в вызывающем потоке, ненулевой результат запрашивает остановку. NULL оставляет обычное ожидание. |
| `state` | Заимствованный контекст колбэка. Действует до возврата, NULL допустим по контракту колбэка. |

## Результат

| Исход | Условие |
| --- | --- |
| `STYXRPC_IO_OK` | Выдан собственный fid. Закройте его после работы. |
| `STYXRPC_IO_NOT_OPEN` | Недопустимый аргумент, отказ обхода, проверки типа до открытия или открытия. |
| `STYXRPC_IO_FAILED` | Ошибка stat открытого fid, неверный тип или размер. |
| `STYXRPC_IO_CANCELLED` | Отмена прервала допуск. Выходы сохраняются, временный fid освобождается. |

## Поведение и время жизни

После открытия SDK запрашивает stat того же fid и проверяет, что это обычный четырёхбайтовый файл. При успехе функция выдаёт fid и необязательный идентификатор вместе. Сохраните идентификатор при создании описателя этой публикации. При любом отказе выходы сохраняются. Временный fid освобождается, clunk может завершиться после возврата.

Колбэк должен быстро возвращать управление и избегать повторного входа в ту же сессию. Мультиплексированный клиент может отправить `Tflush` по тегу ожидающего запроса. Последовательный клиент проверяет флаг между обменами.

## Пример

```c
Use `qid` and `file` only after success. Keep the session alive through the final close.
```

Используйте `qid` и `file` после успешного открытия. Сохраняйте подключение до закрытия файла.

## См. также

[`styxrpc_module_ref_open_remote_until`](module_ref_open_remote_until.ru.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.ru.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.ru.md) · [`styxrpc_vfs_close`](vfs_close.ru.md) · [Парный пример FlashVM и DisVM](../examples/disvm/INT32_REFERENCES.ru.md)
