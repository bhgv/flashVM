# styxrpc_argument_scope_invoke_at_until

[English](argument_scope_invoke_at_until.md) · [Объявление](../include/styxrpc/channel_arguments.h#L79) ·
[Реализация](../src/channel_client.c#L149)

```c
#include <styxrpc/channel_arguments.h>

StyxRpcOutcome styxrpc_argument_scope_invoke_at_until(struct StyxFid *directory,
    StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity,
    StyxRpcLeasedReply *result,
    StyxRpcCallStop stop, void *state);
```

Используйте хостовую цель `StyxRpc::launch`. Заимствуйте точный
неоткрытый каталог `directory` или передайте NULL для обычного
поиска по пути, как у [`styxrpc_argument_scope_invoke_at`](argument_scope_invoke_at.ru.md).

Передайте проверенную цель с PID вызывающего и сроком ожидания.
Заимствуйте `size` байтов закодированных аргументов и буфер ответа
на `capacity` байтов. Функция поглощает переданную область аргументов
при любом исходе. При NULL действует обычный канальный контракт,
вызывающий сам управляет полномочиями аргументов. После отмены
временная область отзывается. Подтверждённые передачи очереди
сохраняются. При успехе `result` заимствует буфер и может владеть
[арендой каналов](channel_lease_release.ru.md) до освобождения.

Необязательный колбэк `stop(state)` выполняется синхронно в
вызывающем потоке. Он проверяет флаг остановки и возвращает ненулевое
значение для отмены. Функция проверяет флаг до открытия, до записи и
между фрагментами запроса и опросами ответа. NULL оставляет обычное
блокирующее ожидание. Удерживайте state до возврата. Fid вызова
принадлежит SDK. Начатый обмен и clunk используют сроки сессии.
Полностью полученный ответ сохраняет обычный исход.

| Исход | Значение |
| --- | --- |
| `STYXRPC_CALL_CANCELLED` | Колбэк остановил вызов до записи запроса. |
| `STYXRPC_CALL_CANCELLED_UNCERTAIN` | Колбэк остановил вызов после записи. У другой стороны могли произойти изменения. |

При отмене SDK закрывает fid вызова и сохраняет типизированный выход.
Запрос не повторяется. Действуют и обычные исходы успеха и отказа.
Каталог модуля остаётся заимствованным. Для параллельных вызовов
используйте разные буферы и слоты вывода.

```c
StyxRpcOutcome outcome = styxrpc_argument_scope_invoke_at_until(
    directory, scope, &target, arguments, size, reply, sizeof(reply), &result,
    stop_requested, task);
```

Здесь `stop_requested(task)` читает флаг вызывающего.
[Парный тест](../tests/flashvm/mixed_pair.py) запускается с
`--cancel-dis-call` и проверяет отмену и дальнейшие вызовы в той же
сессии Styx.
