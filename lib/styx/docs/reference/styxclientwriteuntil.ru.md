# styxclientwriteuntil

[English](styxclientwriteuntil.md) · [Оглавление API](../index.ru.md)

Записать один фрагмент данных Styx с необязательным колбэком отмены.

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientwriteuntil(StyxFid *fid, unsigned long long offset, const void *bytes,
    size_t size, StyxClientStop stop, void *state, StyxTransfer *result);
```

Подключите `Styx::client`. Функция доступна в нативной hosted-сборке.

| Параметр | Контракт |
| --- | --- |
| `fid` | Заимствованный fid, открытый для записи. Согласуйте операции и закрытие на этом fid. |
| `offset` | Смещение в байтах от 0 до `2^63 - 1`. |
| `bytes`, `size` | Входной буфер с `size` доступными для чтения байтами. При нулевой длине допустим NULL. Согласованная единица I/O может ограничить размер запроса. |
| `stop`, `state` | Необязательный [колбэк](../../client/readable_parts/client-h/part-002.inc#L335) и его заимствованный контекст. Для отмены верните ненулевое значение. |
| `result` | Буфер результата [StyxTransfer](../../client/file.h#L20). При NULL операция пропускается. |

## Завершение и отмена

Удерживайте все аргументы до возврата. Колбэк работает в вызывающем
потоке по тем же правилам ожидания и сроков, что и
[styxclientreaduntil](styxclientreaduntil.ru.md). Последовательная
сессия проверяет его только при входе. NULL вместо колбэка даёт
поведение [styxclientwrite](styxclientwrite.ru.md).

Если `Rwrite` приходит раньше `Rflush`, в `result->count` возвращается
длина подтверждённого префикса. При отмене до попытки отправки
`cancelled` установлен, длина и неопределённость нулевые. Если
запись попытались отправить и ответ не пришёл, данные могли измениться.
Клиент устанавливает `uncertain`, блокирует дальнейшую передачу на
этом fid и оставляет его вызывающему. Закройте fid через
[styxclientclunk](styxclientclunk.ru.md).

## Пример

Передайте открытый файл, буфер запроса и колбэк, который читает флаг
отмены задачи. Продвигайте смещение на возвращённую длину при успехе.

```c
StyxTransfer transfer;
styxclientwriteuntil(fid, offset, request, size, stop, task, &transfer);
if (transfer.success) {
    offset += transfer.count;
}
```

[Вызов из SDK](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/src/channel_exchange.c)

## См. также

- [Объявление](../../client/readable_parts/client-h/part-002.inc#L358) · [Реализация](../../client/io.c#L148)
- [styxclientreaduntil](styxclientreaduntil.ru.md) · [styxclientwritecancel](styxclientwritecancel.ru.md)
