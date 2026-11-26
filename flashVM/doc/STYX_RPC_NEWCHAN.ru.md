# rpc.newchan

[English](STYX_RPC_NEWCHAN.md) · [Штатная сессия](STYX_RPC_DEFAULT.ru.md) · [Реализация](../src/styxrpc/session_channel.c)

`rpc.newchan(address)` зарезервирован для дополнительной сессии с другим
узлом. `address` — строка адреса, например в форме Inferno `tcp!host!port`.
Окончательная адресация пока открыта. Текущая реализация проверяет, что
строка непустая, без NUL, и возвращает `undefined`.
Обращения к транспорту в этой версии нет.
