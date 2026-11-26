# Штатная сессия RPC в парном FlashVM

[English](STYX_RPC_DEFAULT.md) · [Файлы с колбэками](STYX_RPC_NEWNODE.ru.md) · [Клиент Limbo](../../disVM/runtime/STYX-CHANNEL.ru.md)

Парный плеер передаёт фильму уже подключённую двунаправленную сессию
как объект `rpc`. Доступ к файловой стороне и публикация узлов идут
через эту же сессию. Для обмена сообщениями используйте
[`rpc.ready()`](STYX_RPC_CHAN_READY.ru.md), [`rpc.recv()`](STYX_RPC_CHAN_RECV.ru.md)
и [`rpc.send(value)`](STYX_RPC_CHAN_SEND.ru.md) прямо на объекте.

Для совместимости со стороной Limbo плеер публикует скрытый корень
`/.rpc` в сессии. Один открытый fid обслуживает эту очередь до clunk.
`rpc.send` со стороны плеера возвращает true после постановки в очередь
либо false, пока порт закрыт или исходящая очередь заполнена.
Доступ к этой очереди выполняется в потоке владельца фильма.

[`openExchange(name)`](STYX_RPC_OPEN_EXCHANGE.ru.md) открывает именованный
порт сообщений на уже присоединённой сессии. Имена для отдельных соединений
описаны в [newchan](STYX_RPC_NEWCHAN.ru.md) и [publishchan](STYX_RPC_PUBLISHCHAN.ru.md).
