# Worker + channel / Поток + канал

A worker runs in a separate VM context. It receives an owned channel handle
through `userdata`, blocks until a number arrives, then returns number + 1.
The caller joins the worker before releasing its retained channel handle.

Рабочий поток запускается с отдельным контекстом VM. Через `userdata` ему
передаётся дескриптор канала. Он ожидает число и возвращает число + 1.
Вызывающая сторона ожидает завершения потока до освобождения дескриптора.

This is a native C89 example, **not** an ActionScript `thread.run` implementation.
Это пример C89, **не** реализация `thread.run` в ActionScript.
