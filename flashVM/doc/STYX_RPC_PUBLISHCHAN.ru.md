# rpc.publishchan

[English](STYX_RPC_PUBLISHCHAN.md) · [Штатная сессия](STYX_RPC_DEFAULT.ru.md) · [Реализация](../src/styxrpc/session_channel.c)

`rpc.publishchan(address, callback)` резервирует синтаксис приёма нового
соединения. `address` — непустая строка без NUL, например
`tcp!port`. `callback` предназначен для подготовки VFS принятого
соединения.

Сейчас вызов возвращает `undefined` после проверки строки и объекта
колбэка. Соединения, сокеты и корни VFS при этом не создаются. Колбэк
не запускается и не удерживается.
