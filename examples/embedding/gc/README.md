# C89 embedding: tracing GC

This example creates a native-owned object, pins it, runs a full cycle, unpins it and collects it. The `FVM_Object *` is **borrowed**: pin before holding it across collection safe points, and do not access it after unpinning and collecting. The VM global object is always rooted.

Build via CMake with `FVM_BUILD_EXAMPLES=ON`; run `flashvm_example_gc`.
