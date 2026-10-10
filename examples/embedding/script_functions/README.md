# Calling a script-defined ActionScript function

The example in `example.as` is ActionScript 1-compatible syntax (and valid in ActionScript 2). It defines a zero-argument function and stores its result in a global variable. `tests/avm1_functions.c` executes equivalent AVM1 bytecode through the embedding API and asserts the result.

To run the C test, configure with `FVM_BUILD_TESTS=ON`, build, then run `ctest --output-on-failure` in the build directory.

**Limitations:** this example intentionally avoids parameters, locals, and function-object properties, which are not yet implemented. `trace` and general Flash Player built-ins are not assumed to be present.
