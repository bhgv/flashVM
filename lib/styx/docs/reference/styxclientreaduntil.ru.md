# styxclientreaduntil

[English](styxclientreaduntil.md) · [Оглавление API](../index.ru.md)

Прочитать один фрагмент данных Styx на открытом fid с необязательным колбэком отмены.

## Синтаксис

```c
#include <styx/client/client.h>

void styxclientreaduntil(StyxFid *fid, unsigned long long offset, void *bytes,
    size_t size, StyxClientStop stop, void *state, StyxTransfer *result);
```

Подключите `Styx::client`. Функция доступна в нативной hosted-сборке.

## Параметры

| Параметр | Контракт |
| --- | --- |
| `fid` | Заимствованный fid, открытый для чтения. Согласуйте операции данных и закрытие на этом fid. |
| `offset` | Смещение в байтах от 0 до `2^63 - 1`. |
| `bytes`, `size` | Буфер для записи длиной `size` байтов. При нулевой длине допустим NULL. Согласованная единица I/O может ограничить размер запроса. |
| `stop`, `state` | Необязательный [колбэк](../../client/readable_parts/client-h/part-002.inc#L335) и заимствованный контекст. Для отмены верните ненулевое значение. Колбэк работает в вызывающем потоке, быстро завершается и не обращается к fid операции. |
| `result` | Буфер результата [StyxTransfer](../../client/file.h#L20). При NULL операция пропускается. |

## Завершение и отмена

Удерживайте fid, буфер и контекст колбэка до возврата. С NULL вместо
колбэка функция работает как [styxclientread](styxclientread.ru.md).
Колбэк проверяется перед отправкой и во время ожидания ответа на
мультиплексоре. Последовательная сессия проверяет его при входе.

Отмена до отправки устанавливает `cancelled`, оставляя `count` и
`uncertain` нулевыми. Мультиплексор отправляет для переданного
запроса один `Tflush` по его тегу. Если `Rread` приходит раньше
`Rflush`, подтверждённые длина и байты сохраняются. Если отмена
победила после попытки отправки, `uncertain` устанавливается,
а `count` остаётся нулём. При наличии колбэка `cancelled` означает
запрошенную через него остановку. Обычные таймауты и ошибки транспорта
тоже завершают передачу с ошибкой.

При успехе установлен `result->success`, а `result->count`
показывает длину скопированных данных. При неопределённом исходе
дальнейшая передача на fid блокируется, с тем же исключением для
чтения с нуля у открытого на чтение каталога, что и в
[styxclientread](styxclientread.ru.md). Fid остаётся у вызывающего.
Ожидание `Rflush` ограничено сроком сессии.

## Пример

В проводном тесте `stop` проверяет флаг в `operation` в том же
потоке, который вызвал чтение. Сервер посылает однобайтовый ответ
перед `Rflush`, поэтому вызов получает подтверждённый байт `'r'`.

```c
StyxTransfer result;
char byte = 0;
styxclientreaduntil(operation->raw, 0, &byte, 1, stop, operation, &result);
```

[Проводной тест](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/tests/channel_stop_wire.c#L191)

## См. также

- [Объявление](../../client/readable_parts/client-h/part-002.inc#L348) · [Реализация](../../client/io.c#L140)
- [styxclientwriteuntil](styxclientwriteuntil.ru.md) · [styxclientreadcancel](styxclientreadcancel.ru.md)
