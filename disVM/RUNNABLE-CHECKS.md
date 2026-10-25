# Runnable Dis checks

Build and install with `bash build-workspace.sh --projects disvm --jobs 4`.
The workspace builds its pinned native Limbo bootstrap automatically; neither an
external `emu` nor administrator installation is required for this build.

On native Linux AMD64, run `out/disvm/disvm-check --all`. The installed binaries
execute the retained production Dis interpreter/JIT instruction tests for WORD32
and WORD64 operands. They check arithmetic, aliases, floating-point environment
and native code publication. Python generates and compiles these programs during
build; running the installed programs does not require Python.

These are diagnostic frontends with test allocator/scheduler fixtures, not a
complete module loader or a general Dis application host. `--help` explains this
boundary; passing a `.dis` program is rejected. Other hosts can build the core
without these AMD64 programs; their existence is not a portability claim.

`examples/console-check/main.b` is a real, fully commented Limbo program: a spawned
producer sends two values over a blocking channel and actual `$Sys` prints 42.
The build installs `console-check.dis` and its debug information when the native
Limbo bootstrap is enabled. Executing this module still requires an initialized
Dis host with the actual Sys builtin. The diagnostic frontend does not substitute
empty Sys functions and does not pretend to launch the IDE.

See [module store](MODULE-STORE.md) for the separate runnable native example that
loads compiled module bytes into the versioned store without executing them.
