# resource_exceptions.as

Использует try/catch/finally для захвата и гарантированного освобождения mutex. В catch рассматривает структурированные поля ресурсной ошибки, включая транспорт и путь; подходит как шаблон очистки после отказа операции.

## Запуск и особенности

Исходник: [resource_exceptions.as](resource_exceptions.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `mutex.new` — [NETWORK_FAULT_CONTEXT.md](../../doc/NETWORK_FAULT_CONTEXT.md).
- `set` — [NETWORK_FAULT_CONTEXT.md](../../doc/NETWORK_FAULT_CONTEXT.md).
- `reset` — [NETWORK_FAULT_CONTEXT.md](../../doc/NETWORK_FAULT_CONTEXT.md).
- `failure.code` — [NETWORK_FAULT_CONTEXT.md](../../doc/NETWORK_FAULT_CONTEXT.md).
- `failure.message` — [NETWORK_FAULT_CONTEXT.md](../../doc/NETWORK_FAULT_CONTEXT.md).
- `failure.context` — [NETWORK_FAULT_CONTEXT.md](../../doc/NETWORK_FAULT_CONTEXT.md).

[Описание в общем каталоге](../README.md#example-actionscript-resource_exceptions-as).
