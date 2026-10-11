# Native worker / Нативный рабочий поток

EN: `main.c` demonstrates starting a C callback in a separate OS thread with a fresh FlashVM runtime and context. The input string is copied before thread creation. `join` waits for completion; the returned value is borrowed from the worker handle until `destroy`. Build via CMake target `flashvm_worker` (test) or compile `main.c` against FlashVM.

RU: `main.c` показывает запуск C-функции в отдельном системном потоке с новым Runtime и Context. Входная строка копируется перед запуском. `join` ожидает завершения, результат принадлежит дескриптору потока до вызова `destroy`. Собирайте цель `flashvm_worker` или `main.c` с библиотекой FlashVM.

This is not yet the ActionScript `thread.run` API. / Это ещё не реализация `thread.run` для ActionScript.
