# delete() именованного канала

[English](STYX_RPC_NAMED_DELETE.md) · [rpc.chan](STYX_RPC_NAMED_CHANNEL.ru.md) · [Код](../src/styxrpc/session_named_channel.cpp)

`port.delete()` закрывает участие объекта и возвращает `undefined`. Метод
принимает вызов без аргументов. Его можно вызвать повторно. Канал живёт до
закрытия последнего участника. Если после этого путь свободен, следующий
`rpc.chan(path)` создаёт новый канал.
