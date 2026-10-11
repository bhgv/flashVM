# Lua-style native callback / Lua-подобный нативный callback

Build this C89 example through CMake or `python3 scripts/check.py`.
It registers `add`, supplies two numbers, and prints `add(20, 22) = 42`.
The same registration is callable by AVM1 ActionCallFunction; the bytecode
integration is covered by `tests/auxlib.c`.

Сборка: CMake или `python3 scripts/check.py`. Пример регистрирует `add`,
передаёт два числа и выводит `add(20, 22) = 42`. Ту же функцию можно вызвать
из AVM1 через ActionCallFunction; это проверено в `tests/auxlib.c`.

See / см. `doc/LUA_STYLE_API.md` and `doc/LUA_STYLE_API.ru.md`.
