# styxcancelcreatepoll

[English](styxcancelcreatepoll.md) · [API Styx](../index.ru.md) · [Явный токен остановки](styxcancelcreate.ru.md)

```c
#include <styx/client/client.h>
StyxCancel *styxcancelcreatepoll(StyxClientStop stop, void *state);
```

Хостовая сборка C99, цель `Styx::client`.

Создать свой токен для синхронной операции на мультиплексоре. Вызывающий может
передать короткий колбэк `stop(state)`, который возвращает ненулевое значение,
когда нужно остановить ожидание. `NULL` создаёт обычный токен. Удерживайте
заимствованный `state` до завершения операции. Колбэк вызывает ожидающий поток
вне замков клиента. При обходе из нескольких фрагментов один логический токен
действует на каждом фрагменте, а остановка сохраняется. Одна допущенная
логическая операция расходует одноразовый допуск. Свою ссылку освобождайте
через `styxcancelfree`.

Функция возвращает собственный токен либо `NULL` при нехватке памяти.
Конструктор не выполняет сетевой обмен и не начинает опрос.

```c
StyxCancel *token = styxcancelcreatepoll(stop, state);
if (token)
{
    StyxTransfer reply;
    StyxFid *fid = styxclientwalkcancel(client, "dir/file", token, &reply);
    styxclientclunklater(fid);
    styxcancelfree(token);
}
```

[Заголовок](../../client/client.h) · [Реализация](../../client/cancel.c)
