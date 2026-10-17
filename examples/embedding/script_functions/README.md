# Calling a script-defined ActionScript function

The example in `example.as` is ActionScript 1-compatible syntax (and valid in ActionScript 2). It defines a zero-argument function and stores its result in a global variable. `tests/avm1_functions.c` executes equivalent AVM1 bytecode through the embedding API and asserts the result.

To run the C test, configure with `FVM_BUILD_TESTS=ON`, build, then run `ctest --output-on-failure` in the build directory.

**Limitations:** this example intentionally avoids parameters, locals, and function-object properties, which are not yet implemented. `trace` and general Flash Player built-ins are not assumed to be present.

<!-- example-catalog-api:start -->
## Назначение и особенности

Исходник иллюстрирует объявление и вызов AVM1-функции и получение результата через глобальную переменную. README направляет к отдельному байткодному тесту; текстовый AS-парсер в этот пример не входит.


## Демонстрируемый script API

- `answer` — [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md).

Это AS-эскиз/исходник; перечень имён не утверждает наличие отдельного C main
или полноту реализации каждого вызова.

[Описание в общем каталоге](../../README.md#example-embedding-script_functions).
<!-- example-catalog-api:end -->
