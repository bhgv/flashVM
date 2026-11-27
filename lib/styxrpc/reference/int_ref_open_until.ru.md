# styxrpc_int_ref_open_until

[English](int_ref_open_until.md) · [Объявление](../include/styxrpc/variable_ref.h#L42) · [Реализация](../src/variable_ref_client.c#L14)

Открыть выданную ссылку на временную Int32-переменную в исходной сессии процесса.

## Синтаксис

```c
#include <styxrpc/variable_ref.h>

StyxRpcIoOutcome styxrpc_int_ref_open_until(struct StyxClient *issuer,
    const StyxRpcIntRef *ref, StyxRpcVfsFile **out, StyxRpcVfsStop stop, void *state);
```

## Доступность

Хостовый SDK C, библиотека `StyxRpc::launch`. Требуется подключённый клиент Styx и поддержка Int32-файлов `.var` со стороны владельца. Примеры проверены на Linux64 c DisVM.

## Параметры

| Имя | Описание |
| --- | --- |
| `issuer` | Заимствованный подключённый клиент Styx дерева процессов исходного владельца. |
| `ref` | Заимствованный декодированный описатель. PID не ниже `STYXRPC_INT_REF_FIRST_PID` (0x80000000). `writable` равен 0 для чтения или 1 для чтения и записи. |
| `out` | Обязательный выход для собственного fid. |
| `stop` | Необязательный колбэк. Выполняется в вызывающем потоке, ненулевой результат запрашивает остановку. NULL оставляет обычное ожидание. |
| `state` | Заимствованный контекст колбэка. Действует до возврата, NULL допустим по контракту колбэка. |

## Результат

| Исход | Условие |
| --- | --- |
| `STYXRPC_IO_OK` | Выдан собственный fid. Закройте его после работы. |
| `STYXRPC_IO_NOT_OPEN` | Недопустимый аргумент, отказ обхода или открытия, отозванная ссылка либо несовпадение PID в Qid. |
| `STYXRPC_IO_FAILED` | Ошибка stat открытого fid, неверный тип или размер. |
| `STYXRPC_IO_CANCELLED` | Отмена прервала допуск. Выходы сохраняются, временный fid освобождается. |

## Поведение и время жизни

SDK открывает `.prog/<pid>/.var/ref` в режиме описателя и проверяет обычный четырёхбайтовый файл. Старшие 32 бита `Qid.path` открытого fid должны совпасть с выданным PID. Отзыв делает открытые fid недействительными, а новая выдача получает новый PID. При любом отказе выходы сохраняются. Временный fid освобождается, clunk может завершиться после возврата.

Колбэк должен быстро возвращать управление и избегать повторного входа в ту же сессию. Мультиплексированный клиент может отправить `Tflush` по тегу ожидающего запроса. Последовательный клиент проверяет флаг между обменами.

## Пример

```c
Here `received` is a descriptor decoded on the original issuer's connection. Close `file` if the open succeeded.
```

Здесь `received` — описатель, декодированный на подключении к исходному владельцу. Закройте `file` после успешного открытия.

## См. также

[Ссылки на Int32](int_variable_refs.ru.md) · [`styxrpc_vfs_int_open_identified_until`](vfs_int_open_identified_until.ru.md) · [`styxrpc_module_ref_open_remote_until`](module_ref_open_remote_until.ru.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.ru.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.ru.md) · [`styxrpc_vfs_close`](vfs_close.ru.md) · [Парный пример FlashVM и DisVM](../examples/disvm/INT32_REFERENCES.ru.md)
