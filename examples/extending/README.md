# Extending FlashVM from C89

`main.c` registers the `makePoint` native callback with the current C API.
The callback validates two numeric arguments, creates a VM object,
sets its `x` and `y` properties and returns it to the C host.
Comments document the ownership and registration behavior.

Expected output:

    makePoint => (12, 34)

Build from the repository root:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

Executable: `flashvm_example_extending` (when `FVM_BUILD_EXAMPLES=ON`).
This is a **statically registered** extension, not a loadable `.so` / `.dll`.
ActionScript-to-native calls and native Flash display objects are not yet available.

Read [Extending API](../../doc/EXTENDING_API.md).
