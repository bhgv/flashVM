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
through the native-cell parser and ordinary typed linker. The exact
provider carries local adapter admission. The
constructor trusts the host's input provenance. Remote metadata, wrapper
providers and filenames cannot confer admission. Ordinary remote code retains
the execution-provider launch requirement.

The adapter opens its configured Styx route paths at invocation under the
instance's namespace. Discovering owner routes for arbitrary remote files
remains a separate operation.

Each successful typed load of an adapter attaches an independent copy of the
active namespace's bindings to its module instance before returning to the
caller. The copy retains the mounted providers. A later caller bind or unmount
affects subsequent loads. Existing instances keep their selected bindings.
The providers and their backing files remain live, so this does not preserve
a closed session or freeze file contents.

A namespace change during adapter loading can make the load fail. Retry the load
against the new bindings if desired.

To keep the original file visible to reads, use [dvm_module_publish](MODULE-PUBLISH.md).
