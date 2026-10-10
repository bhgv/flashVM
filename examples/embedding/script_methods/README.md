# Named AVM1 methods / Именованные методы AVM1

EN: The C89 regression example in `tests/avm1_script_method.c` builds a small
AVM1 bytecode sequence. It declares `compute(n)`, creates a receiver through the
public object API, sets `base = 35`, and calls `receiver.compute(7)` through
`CallMethod`. The script accesses `this.base` and returns `42`.

RU: Пример на C89 в `tests/avm1_script_method.c` формирует байткод AVM1,
объявляет `compute(n)`, создаёт объект через публичный API, устанавливает
`base = 35` и вызывает `receiver.compute(7)` через `CallMethod`.
Внутри функции `this` указывает на объект-получатель; результат — `42`.

EN: **Current limitation:** the interpreter looks up the method name in the
context's named-script-function registry, not in an object's properties.
It cannot yet represent AVM1 functions as first-class object values, handle
anonymous closures, or implement `DefineFunction2`. This is not Flash 8 parity.

RU: **Ограничение:** поиск имени метода выполняется в реестре именованных
скриптовых функций контекста, а не в свойствах объекта. Функции как значения,
анонимные замыкания и `DefineFunction2` пока не реализованы. Это ещё не
полная совместимость с Flash 8.

Run / Запуск: `ctest --test-dir build -R flashvm_avm1_script_method --output-on-failure`.
