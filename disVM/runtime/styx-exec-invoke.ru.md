# StyxexecHandler.invoke

[English](styx-exec-invoke.md) · [Руководство](STYX-EXEC-PUBLISH.ru.md) · [Интерфейс](../module/styxexec.m#L8) · [Реализация](../../lib/styxrpc/tests/flashvm/publish_handler.b)

```limbo
invoke(path: string, bytes: array of byte): array of byte
```

Реализуйте метод в модуле, передаваемом Node.dispatch. Он получает абсолютный путь публикации и принадлежащие запросу байты в выбранном потоке Limbo. Верните массив не длиннее входа либо nil для успешного пустого ответа. Исключение отклоняет вызов.

```limbo
return bytes;
```
