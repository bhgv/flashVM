# parsing.as

Сравнивает parseInt с автоматическим и явным основанием и parseFloat с числовым префиксом. Примеры 0777, шестнадцатеричного 3E8 и строки с единицами показывают, почему основание и границы числового префикса важны.

## Запуск и особенности

Исходник: [parsing.as](parsing.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `parseInt` — [GLOBAL_PARSING.md](../../doc/GLOBAL_PARSING.md).
- `parseFloat` — [GLOBAL_PARSING.md](../../doc/GLOBAL_PARSING.md).
- `String` — [GLOBAL_PARSING.md](../../doc/GLOBAL_PARSING.md).

[Описание в общем каталоге](../README.md#example-actionscript-parsing-as).
