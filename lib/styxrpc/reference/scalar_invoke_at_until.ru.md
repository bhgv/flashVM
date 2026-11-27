# styxrpc_scalar_invoke_at_until

[English](scalar_invoke_at_until.md) · [Объявление](../include/styxrpc/scalar_client.h#L47) ·
[Реализация](../src/scalar_client.c#L9)

```c
#include <styxrpc/scalar_client.h>

StyxRpcOutcome styxrpc_scalar_invoke_at_until(struct StyxFid *directory,
    struct StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result,
    StyxRpcCallStop stop, void *state);
```

Используйте хостовую цель `StyxRpc::launch`. Заимствуйте точный
неоткрытый каталог `directory` или передайте NULL для обычного
поиска по пути, как у [`styxrpc_scalar_invoke_at`](scalar_invoke_at.ru.md).

Передайте точную компилированную сигнатуру и один знаковый аргумент.
При NULL в directory передайте подключённую сессию и относительный путь
к модулю. Положительное `wait_ms` ограничивает ожидание ответа в
миллисекундах, максимум `STYXRPC_SCALAR_WAIT_MAX_MS`. При успехе
`result` получает скаляр.

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
StyxRpcOutcome outcome = styxrpc_scalar_invoke_at_until(
    directory, session, "lib/counter.dis", "next",
    STYXRPC_INT_SIGNATURE, 41, 1000, &result,
    stop_requested, task);
```

Здесь `stop_requested(task)` читает флаг вызывающего.
[Парный тест](../tests/flashvm/mixed_pair.py) запускается с
`--cancel-dis-call` и проверяет отмену и дальнейшие вызовы в той же
сессии Styx.
