# styxclientremovecancel

[English](styxclientremovecancel.md) · [Указатель API](../index.ru.md) · [Удаление с обычным сроком ожидания](styxclientremove.ru.md)

```c
#include <styx/client/client.h>

int styxclientremovecancel(StyxFid *fid, StyxCancel *cancel, StyxTransfer *result);
```

Хостовая сборка C99, цель `Styx::client`.

| Параметр | Значение |
| --- | --- |
| `fid` | Свой fid удаляемого узла. Передайте его функции и больше не используйте. `NULL` означает локальный отказ. |
| `cancel` | Необязательный заимствованный одноразовый токен мультиплексного клиента, живой до возврата. `NULL` сохраняет обычный срок ожидания. |
| `result` | Необязательный выходной `StyxTransfer`. Фиксирует подтверждение, отмену и возможное изменение. Число байт равно нулю. |

Функция отправляет не больше одного `Tremove`. Возвращает `1` только при `Rremove`. Подтверждённый `Rerror` поглощает удалённый fid по правилам 9P без дополнительного clunk. Если обмен закончился без подтверждения, клиент ставит независимое освобождение fid в очередь, пока соединение работоспособно. Отправленный запрос без ответа мог удалить узел и выставляет `uncertain`. Очистка выполняет clunk прежнего fid без повтора удаления. Предварительная отмена тоже поглощает fid.

```c
int remove_report(StyxClient *session, const char *path, StyxCancel *stop,
    StyxTransfer *out)
{
    return styxclientremovecancel(styxclientwalk(session, path), stop, out);
}
```

Если требуется другая открытая ссылка на узел, пройдите к нему отдельным fid. В примере проход использует обычный срок ожидания, токен управляет самим удалением. См. [отменяемое создание](styxclientcreatecancel.ru.md), [заголовок](../../client/client.h) и [реализацию](../../client/clunk.c).
