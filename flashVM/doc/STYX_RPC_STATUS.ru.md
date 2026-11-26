# vfs.status()

[English](STYX_RPC_STATUS.md) · [Сессия](STYX_RPC_DEFAULT.ru.md) · [Реализация](../src/styxrpc/session_launch.c)

В парном плеере вызовите `vfs.status()` или `os.status()` без аргументов.
Оба вызова возвращают новый снимок с булевым полем `connected` и строкой
`channel`. Значение `"open"` означает, что локальный клиент Styx считает
сессию подключённой. После обнаружения отключения возвращается `"closed"`.
Вызов обходится без I/O. Пока клиент не обнаружил обрыв, снимок может
по-прежнему показывать открытый канал.

```actionscript
var session = vfs.status();
if (session.connected) {
    var input = os.open("/notes.txt");
    input.close();
}
```

Для проверки очереди сообщений есть [`vfs.ready()`](STYX_RPC_CHAN_READY.ru.md),
для квитанции запуска — [`vfs.status(token)`](STYX_RPC_SESSION.ru.md).
