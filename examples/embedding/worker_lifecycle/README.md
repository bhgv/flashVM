# Suspended worker / Отложенный запуск

C89 example (`main.c`) creates a suspended OS thread, verifies its state,
starts it, joins and checks its result (42). The worker executes in a separate
FlashVM context. Build as part of the project and run `flashvm_worker_lifecycle`.

Пример на C89 создаёт ожидающий поток ОС, проверяет состояние, запускает,
ожидает завершения и проверяет результат 42. Контекст FlashVM отдельный.

No `.as` example is executable yet: the ActionScript `thread` object is
specified but not implemented. Объект ActionScript `thread` пока не реализован.
