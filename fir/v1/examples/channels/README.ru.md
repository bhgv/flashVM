# Каналы

[English](README.md).

`limbo.chan_create` создаёт неограниченную очередь, `limbo.chan_send %ch,
%answer` копирует сообщение в неё, `limbo.chan_receive %ch` получает сообщение,
`limbo.chan_close %ch` закрывает канал. Эффекты не удаляются оптимизатором.
Перед исполнением хост вызывает `fvm_extensions_install(context)`.
Это привязка к существующему расширению, не новая реализация каналов.
[API расширения](../../../../flashVM/doc/ACTIONSCRIPT_EXTENSIONS.ru.md),
[C API каналов](../../../../flashVM/doc/CHANNELS.ru.md).

```sh
build/fir1/fir1 check fir/v1/examples/channels/main.fir
build/fir1/fir1 opt fir/v1/examples/channels/main.fir build/fir1/channels.fir
build/fir1/fir1 swf build/fir1/channels.fir build/fir1/channels.swf
```

Ожидаемый trace: `42`.
[CLI](../../compiler/README.ru.md), [FIR](../../doc/FORMAT.ru.md),
[tests](../../doc/TESTING.ru.md). `python fir/v1/view_docs.py`.
