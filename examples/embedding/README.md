# Embedding FlashVM from C89

`main.c` demonstrates creating a runtime and context, creating an object,
setting and reading properties, publishing an object as a global, registering
and invoking a native C function, and destroying the context/runtime.
Comments explain ownership and borrowed string lifetime.

Expected output:

    Hello from C host
    nativeSum(10, 20) = 30

Build from the repository root:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

Executable: `flashvm_example_embedding` (when `FVM_BUILD_EXAMPLES=ON`).
This does **not** execute ActionScript or SWF yet.

Read [Embedding API](../../doc/EMBEDDING_API.md).
