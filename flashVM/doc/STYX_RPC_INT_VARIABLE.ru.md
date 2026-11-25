# styxrpc.intVariable

[English](STYX_RPC_INT_VARIABLE.md) · [Сессия](STYX_RPC_SESSION.ru.md) · [Реализация](../src/styxrpc/session_modules.c)

```actionscript
var counter = styxrpc.intVariable("/mounted/peer.dis/.var/count", true);
counter.value = 77;
var confirmed = counter.value;
```

Передайте точный путь ячейки Int32 в `.var` с необязательной начальной
косой чертой. Без второго параметра или с `false` доступ только на чтение.
Полученный объект владеет открытым fid. Чтение `value` запрашивает текущее
значение у владельца. Прокси с разрешением записи принимает точные целые
числа от −2³¹ до 2³¹−1. Ошибка доступа вызывает исключение. При потере
подтверждения запись может сообщить о неопределённом результате. Чтение и
запись выполняются у владельца фильма и могут ждать I/O. Сборщик мусора
откладывает закрытие fid. Обычные файловые операции скрывают `.var`. См.
[publishObject](STYX_RPC_PUBLISH_OBJECT.ru.md).
