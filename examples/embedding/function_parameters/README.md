# AVM1 function arguments / Аргументы функций AVM1

English: The corresponding bytecode regression test is `tests/avm1_parameters.c`.
It defines `echo(n)` and invokes it with 42, verifying the global `result`.
Run `ctest --test-dir build -R flashvm_avm1_parameters --output-on-failure`.
This is a bytecode-level test; an ActionScript-to-SWF compiler is not bundled.

Русский: Соответствующий тест байткода — `tests/avm1_parameters.c`.
Он определяет `echo(n)`, вызывает функцию с аргументом 42 и проверяет
глобальную переменную `result`. Компилятор ActionScript в SWF в проект не входит.
