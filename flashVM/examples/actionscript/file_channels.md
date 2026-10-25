# file_channels.as

Сохраняет циклический граф объектов в файловый канал, закрывает его и читает граф обратно. Различает receive и tryReceive на EOF и описывает режим добавления; расширения устанавливает приложение через [fvm_extensions_install](../../include/flashvm/extensions.h#L19).

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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_extensions_install` | function | [include/flashvm/extensions.h:19](../../include/flashvm/extensions.h#L19) |
<!-- source-links:end -->
