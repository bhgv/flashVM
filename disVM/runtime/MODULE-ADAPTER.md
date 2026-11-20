# dvm_module_adapter

[Русский](MODULE-ADAPTER.ru.md) · [Header](vfs/module_adapter.h) · [Publication example](../examples/remote-scalar/STYX.md)

Include `vfs/module_adapter.h` in a native DisVM host. Call
`dvm_module_adapter(bytes, size)` with trusted local adapter bytecode and its
length in bytes, greater than zero and less than 8 MiB. It copies the input
and returns an owned provider, or NULL for invalid arguments or allocation
failure. The call allocates native storage and performs no I/O.

Mount the returned provider as an exact file and bind it to the application's
normal load name. Mounting and binding retain the file. Release the original
provider reference after mounting. Each read open gets an independent
cursor. The file permits read and execute access and denies write access.

Normal Dis `load` resolves the namespace once and passes the selected adapter
through the native-cell parser and ordinary typed linker. Only the concrete
provider returned by this constructor has local adapter admission. The
constructor trusts the host's input provenance. Remote metadata, wrapper
providers and filenames cannot confer admission. Ordinary remote code retains
the execution-provider launch requirement.

The adapter opens its configured Styx route paths at invocation under the
active namespace. Route discovery and a per-instance route pin are separate
from publication of the adapter image.
