# string.as

Показывает String.length, substring, indexOf, charCodeAt и fromCharCode на смешанной латинице, кириллице и emoji. Позволяет отличить число кодовых единиц UTF-16 от числа видимых символов; это исходник для внешнего AVM1-компилятора.

## Запуск и особенности

Исходник: [string.as](string.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `String` — [STANDARD_STRING.md](../../doc/STANDARD_STRING.md).
- `String.length` — [STANDARD_STRING.md](../../doc/STANDARD_STRING.md).
- `substring` — [STANDARD_STRING.md](../../doc/STANDARD_STRING.md).
- `indexOf` — [STANDARD_STRING.md](../../doc/STANDARD_STRING.md).
- `charCodeAt` — [STANDARD_STRING.md](../../doc/STANDARD_STRING.md).
- `String.fromCharCode` — [STANDARD_STRING.md](../../doc/STANDARD_STRING.md).

[Описание в общем каталоге](../README.md#example-actionscript-string-as).
