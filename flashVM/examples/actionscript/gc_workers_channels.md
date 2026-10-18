# gc_workers_channels.as

Связывает именованную worker-функцию, отложенный запуск, передачу сообщения по общему каналу и join. После завершения убирает ссылку на worker и вызывает GC; демонстрирует координацию ресурсов между изолированными VM.

## Запуск и особенности

Исходник: [gc_workers_channels.as](gc_workers_channels.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `thread.run_suspended` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).
- `start` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).
- `join` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).
- `channel.create` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).
- `send` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).
- `receive` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).
- `gc.collect` — [ACTIONSCRIPT_EXTENSIONS.md](../../doc/ACTIONSCRIPT_EXTENSIONS.md).

[Описание в общем каталоге](../README.md#example-actionscript-gc_workers_channels-as).
