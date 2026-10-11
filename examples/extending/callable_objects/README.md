# Callable objects (C89)

This example creates a native callable bound to one interpreter context, stores it
in an ordinary object property, retrieves it, and invokes it with two numbers.
The callback receives the interpreter context and a Lua-inspired argument stack.
It must clear the argument stack and push at most one result before returning.

Build from the repository root with CMake and enable examples. The `main.c`
source is also exercised by the regression test `flashvm_callable`.

**Limitations:** callable objects are currently native C callbacks only; the
AVM1 bytecode interpreter does not yet call these object values. The context
owns the callback registration; `userdata` lifetime is the host's responsibility.
Do not access one context concurrently from multiple threads.
