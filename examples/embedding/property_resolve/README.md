# Missing methods resolved to native C Functions

[Русский](README.ru.md) · [Source](main.c)

This C89 example loads a minimal SWF6, installs `Object.__resolve`, and calls
an absent `sum` method. The resolver returns the original native C Function;
the VM invokes it with the original receiver and arguments. No `sum` property
is cached. Successful output is `sum = 42`.

From the repository root, with CMake and C/C++ compilers supporting C89/C++98:

```sh
cmake -S . -B build-resolve -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-resolve --config Release --target flashvm_examples_embedding_property_resolve_main
./build-resolve/flashvm_examples_embedding_property_resolve_main
```

Windows generators may place the `.exe` in `Release/`. Foreign OS execution
has not been validated by this Linux check.

The receiver is pinned during construction. Temporary Function pins protect
publication; the resolver's managed capture retains the original C Function.
Userdata alone is not a GC root. Native callbacks retain the normal collection
barrier. Context destruction releases the receiver graph, including pins.

This example runs locally. For original C functions served over one broker port
between independent processes, two runtimes on separate threads, or computers
on a LAN, see [function_rpc](../function_rpc/README.md). For hook semantics and
remaining limitations, see [OBJECT_RESOLVE](../../../doc/OBJECT_RESOLVE.md).
