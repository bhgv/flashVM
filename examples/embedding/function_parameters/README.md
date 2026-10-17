# AVM1 function arguments / Аргументы функций AVM1

English: The corresponding bytecode regression test is `tests/avm1_parameters.c`.
It defines `echo(n)` and invokes it with 42, verifying the global `result`.
Run `ctest --test-dir build -R flashvm_tests_avm1_parameters --output-on-failure`.
This is a bytecode-level test; an ActionScript-to-SWF compiler is not bundled.

Русский: Соответствующий тест байткода — `tests/avm1_parameters.c`.
Он определяет `echo(n)`, вызывает функцию с аргументом 42 и проверяет
глобальную переменную `result`. Компилятор ActionScript в SWF в проект не входит.

<!-- example-catalog-api:start -->
## Назначение и особенности

Исходник echo(n) и ссылка на байткодный тест показывают передачу параметра и запись результата в глобальную переменную. Отдельного main нет; исходный AS компилируется внешним инструментом.


## Демонстрируемый script API

- `echo` — [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md).

Это AS-эскиз/исходник; перечень имён не утверждает наличие отдельного C main
или полноту реализации каждого вызова.

[Описание в общем каталоге](../../README.md#example-embedding-function_parameters).
<!-- example-catalog-api:end -->
