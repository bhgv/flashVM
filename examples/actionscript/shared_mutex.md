# shared_mutex.as

Передаёт worker-потоку объект с mutex и каналом. Рабочая функция захватывает общий ресурс, отправляет сообщение и освобождает его; показывает различие общей идентичности ресурса и копирования обычного графа.

## Запуск и особенности

Исходник: [shared_mutex.as](shared_mutex.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `mutex.new` — [MUTEX_CHECKPOINT.md](../../doc/MUTEX_CHECKPOINT.md).
- `set` — [MUTEX_CHECKPOINT.md](../../doc/MUTEX_CHECKPOINT.md).
- `reset` — [MUTEX_CHECKPOINT.md](../../doc/MUTEX_CHECKPOINT.md).
- `thread.run` — [MUTEX_CHECKPOINT.md](../../doc/MUTEX_CHECKPOINT.md).
- `join` — [MUTEX_CHECKPOINT.md](../../doc/MUTEX_CHECKPOINT.md).
- `channel.create` — [MUTEX_CHECKPOINT.md](../../doc/MUTEX_CHECKPOINT.md).

[Описание в общем каталоге](../README.md#example-actionscript-shared_mutex-as).
