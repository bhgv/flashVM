# styxrpc_module_ref_open_remote_until

[English](module_ref_open_remote_until.md) · [Объявление](../include/styxrpc/module_ref.h#L55) · [Реализация](../src/variable_ref_client.c#L51)

Открыть Int32 по описателю экспорта модуля в обычной VFS исходного владельца.

## Синтаксис

```c
#include <styxrpc/module_ref.h>

StyxRpcIoOutcome styxrpc_module_ref_open_remote_until(struct StyxClient *issuer,
    const StyxRpcModuleRef *ref, StyxRpcVfsFile **out, StyxRpcVfsStop stop, void *state);
```

## Доступность

Хостовый SDK C, библиотека `StyxRpc::launch`. Требуется подключённый клиент Styx и поддержка Int32-файлов `.var` со стороны владельца. Примеры проверены на Linux64 c DisVM.

## Параметры

| Имя | Описание |
| --- | --- |
| `issuer` | Заимствованный клиент, подключённый к обычной VFS владельца. До вызова выберите подключение по согласованной роли владельца. |
| `ref` | Заимствованный декодированный описатель. Содержит ненулевой 64-битный идентификатор узла, канонический путь `mounted/<image>/.var/<export>` с NUL и режим 0 для чтения или 1 для чтения и записи. |
| `out` | Обязательный выход для собственного fid. |
| `stop` | Необязательный колбэк. Выполняется в вызывающем потоке, ненулевой результат запрашивает остановку. NULL оставляет обычное ожидание. |
| `state` | Заимствованный контекст колбэка. Действует до возврата, NULL допустим по контракту колбэка. |

## Результат

| Исход | Условие |
| --- | --- |
| `STYXRPC_IO_OK` | Выдан собственный fid. Закройте его после работы. |
| `STYXRPC_IO_NOT_OPEN` | Недопустимый аргумент, отказ обхода или открытия, снятый экспорт либо несовпадение Qid. |
| `STYXRPC_IO_FAILED` | Ошибка stat открытого fid, неверный тип или размер. |
| `STYXRPC_IO_CANCELLED` | Отмена прервала допуск. Выходы сохраняются, временный fid освобождается. |

## Поведение и время жизни

SDK открывает файл по пути, проверяет его тип и размер в четыре байта и сравнивает все 64 бита `Qid.path` с `ref->node`. Замена по тому же пути не проходит сравнение. Вызывающий проверяет область подключения при декодировании описателя. При любом отказе выходы сохраняются. Временный fid освобождается, clunk может завершиться после возврата.

Колбэк должен быстро возвращать управление и избегать повторного входа в ту же сессию. Мультиплексированный клиент может отправить `Tflush` по тегу ожидающего запроса. Последовательный клиент проверяет флаг между обменами.

## Пример

```c
Here `received` is a scoped module descriptor. After cancellation, a new open can use the same descriptor to recheck the original identity.
```

Здесь `received` — описатель модуля в согласованной области. После отмены этот же описатель позволяет снова проверить исходный идентификатор.

## См. также

[Протокол передачи ссылок](module_refs.ru.md) · [`styxrpc_vfs_int_open_identified_until`](vfs_int_open_identified_until.ru.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.ru.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.ru.md) · [`styxrpc_vfs_close`](vfs_close.ru.md) · [Парный пример FlashVM и DisVM](../examples/disvm/INT32_REFERENCES.ru.md)
