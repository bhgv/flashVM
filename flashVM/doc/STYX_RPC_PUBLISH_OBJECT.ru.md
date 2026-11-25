# styxrpc.publishObject

[English](STYX_RPC_PUBLISH_OBJECT.md) · [Плеер](STYX_RPC_PLAYER.ru.md) · [Реализация](../src/styxrpc/player_services.c)

```actionscript
receiver.total = 41;
styxrpc.publishObject("sample.dis", receiver, ["update", "apply"],
    ["total", "total", "var", "rw"]);
```

Передайте имя файла без пути и живой объект. Образ доступен другой стороне
по `mounted/<имя>`. Передайте от одного до восьми методов и до восьми
переменных Int32 в любом порядке.

| Поля | Назначение |
| --- | --- |
| `[export, member]` | Вызвать колбэк `fn(int): int` у объекта. |
| `[export, member, signature, arguments, result]` | Вызвать managed-колбэк с указанной компилированной сигнатурой Dis и схемами. |
| `[export, member, signature, arguments, result, "graph"]` | Вызвать метод через штатный графовый кодек для вложенных и возвращаемых ссылок. |
| `[name, property, "var", "ro"]` или `[name, property, "var", "rw"]` | Опубликовать живое свойство Int32 в `.var/<name>` с указанным доступом. |

При допуске образа вызов возвращает true. Некорректные описатели и повторная
публикация дают ошибку. Плеер удерживает объект и вызывает методы в потоке
владельца. Каждый фильм может опубликовать один образ. При замене фильма
плеер отзывает его образ. Последующие операции на старых fid дают ошибку.
См. [loadModule](STYX_RPC_LOAD_MODULE.ru.md) и [intVariable](STYX_RPC_INT_VARIABLE.ru.md).
