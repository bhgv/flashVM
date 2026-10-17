# file_channels.as

Сохраняет циклический граф объектов в файловый канал, закрывает его и читает граф обратно. Различает receive и tryReceive на EOF и описывает режим добавления; расширения устанавливает приложение через fvm_extensions_install.

## Запуск и особенности

Исходник: [file_channels.as](file_channels.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `channel.create` — [FILE_CHANNELS.md](../../doc/FILE_CHANNELS.md).
- `send` — [FILE_CHANNELS.md](../../doc/FILE_CHANNELS.md).
- `receive` — [FILE_CHANNELS.md](../../doc/FILE_CHANNELS.md).
- `tryReceive` — [FILE_CHANNELS.md](../../doc/FILE_CHANNELS.md).
- `close` — [FILE_CHANNELS.md](../../doc/FILE_CHANNELS.md).

[Описание в общем каталоге](../README.md#example-actionscript-file_channels-as).
