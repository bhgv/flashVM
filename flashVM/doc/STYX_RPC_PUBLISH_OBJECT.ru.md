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
| `[export, member, signature, arguments, result, "channel", slots]` | Вызвать фабрику с аргументами по значению и одним типизированным канальным результатом, например `"(i)"` и `"(c[i])"`. Колбэк может вернуть первое значение очереди. `slots` задаёт целую ёмкость от 0 (рандеву) до 8. |
| `[name, property, "var", "ro"]` или `[name, property, "var", "rw"]` | Опубликовать живое свойство Int32 в `.var/<name>` с указанным доступом. |

При допуске образа вызов возвращает true. Некорректные описатели и повторная
публикация дают ошибку. Плеер удерживает объект и вызывает методы в потоке
владельца. Каждый фильм может опубликовать один образ. При замене фильма
плеер отзывает его образ. Последующие операции на старых fid дают ошибку.
См. [loadModule](STYX_RPC_LOAD_MODULE.ru.md) и [intVariable](STYX_RPC_INT_VARIABLE.ru.md).

Фабрики каналов используют тот же [реестр аренд](../../lib/styxrpc/reference/module_channels_init.ru.md),
что и публикации на C. У фильма может одновременно быть до 16 аренд
семейств возвращённых каналов. Обычный типизированный `load` в Limbo
позволяет вызвать фабрику и использовать канал. Аренду удерживает fid
вызова. Закрытие fid или смена фильма отзывает её. Для импорта канального
метода из AS пока нужен встречный PID, предоставленный хостом.

Парный [тест](../../lib/styxrpc/tests/flashvm/mixed_pair.py) с ключом
`--module-channel` собирает [провайдер AS](../../lib/styxrpc/tests/flashvm/module-channel_pair.as)
и [клиент Limbo](../../lib/styxrpc/tests/flashvm/module-channel_pair.b)
средствами проекта и проверяет две независимые очереди, скалярный вызов и
живую переменную объекта.
