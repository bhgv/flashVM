# error.as

Показывает создание Error, throw, проверку instanceof в catch и выполнение finally. Полезен для понимания обработки ошибки на уровне AS1/AS2; исполняемая проверка соответствующего байткода находится в tests/actionscript_error.cpp.

## Запуск и особенности

Исходник: [error.as](error.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `Error` — [STANDARD_ERROR.md](../../doc/STANDARD_ERROR.md).
- `Error.toString` — [STANDARD_ERROR.md](../../doc/STANDARD_ERROR.md).
- `instanceof` — [STANDARD_ERROR.md](../../doc/STANDARD_ERROR.md).
- `throw / catch / finally` — [STANDARD_ERROR.md](../../doc/STANDARD_ERROR.md).

[Описание в общем каталоге](../README.md#example-actionscript-error-as).
