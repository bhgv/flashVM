# styxrpc_managed_invoke_at_until

[English](managed_invoke_at_until.md) · [Объявление](../include/styxrpc/managed_client.h#L44) ·
[Реализация](../src/managed_client.c#L11)

```c
#include <styxrpc/managed_client.h>

StyxRpcOutcome styxrpc_managed_invoke_at_until(struct StyxFid *directory,
    const StyxRpcManagedTarget *target, const void *arguments, size_t size,
    void *reply, size_t capacity, StyxRpcManagedReply *result,
    StyxRpcCallStop stop, void *state);
```

Используйте хостовую цель `StyxRpc::launch`. Заимствуйте точный
неоткрытый каталог `directory` или передайте NULL для обычного
поиска по пути, как у [`styxrpc_managed_invoke_at`](managed_invoke_at.ru.md).

Заимствуйте проверенную цель, закодированные записи аргументов и их
размер `size`. Передайте буфер ответа на `capacity` байтов. Цель
задаёт схему, PID при контекстном вызове и срок ожидания ответа.
При успехе разобранный результат и обновления ссылок заимствуют
буфер ответа. Обновления доступны после полного проверенного ответа.

Необязательный колбэк `stop(state)` выполняется синхронно в
вызывающем потоке. Он проверяет флаг остановки и возвращает ненулевое
значение для отмены. Функция проверяет флаг до открытия, при отправке и
при ожидании данных. На мультиплексоре отправленный запрос получает один
`Tflush` по тегу. Метаданные, подтверждение flush и clunk используют сроки
сессии. Полный ответ до подтверждения flush сохраняет обычный исход.
NULL оставляет обычное блокирующее ожидание. Удерживайте state до
возврата. Fid вызова принадлежит SDK.

| Исход | Значение |
| --- | --- |
| `STYXRPC_CALL_CANCELLED` | Колбэк остановил вызов до записи запроса. |
| `STYXRPC_CALL_CANCELLED_UNCERTAIN` | Колбэк остановил вызов во время или после записи. У другой стороны могли произойти изменения. |

При отмене SDK закрывает fid вызова и сохраняет типизированный выход.
Запрос не повторяется. Действуют и обычные исходы успеха и отказа.
Каталог модуля остаётся заимствованным. Для параллельных вызовов
используйте разные буферы и слоты вывода.

```c
StyxRpcOutcome outcome = styxrpc_managed_invoke_at_until(
    directory, &target, arguments, size, reply, sizeof(reply), &result,
    stop_requested, task);
```

Здесь `stop_requested(task)` читает флаг вызывающего.
[Парный тест](../tests/flashvm/mixed_pair.py) запускается с
`--cancel-dis-call` и проверяет отмену и дальнейшие вызовы в той же
сессии Styx.
