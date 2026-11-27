# Writable 9P tree from C and Limbo

[Русский](MUTATIONS.ru.md) · [SDK guide](../../README.md) · [Mutation API](../../reference/server_enable_mutations.md)

The [C provider](mutable_device.c) serves a small in-memory tree on an inherited Unix socket. Its file IDs come from stable slots. A directory revision changes when a child appears or is unlinked. Each writer or reader holds its own reference to the slot, so recreating a name never redirects an old open file.

1. Build the hosted Styx and libstyxrpc packages. Configure libstyxrpc with `STYXRPC_EXAMPLES=ON` and `BUILD_TESTING=ON`. Pass `STYXRPC_DISVM_SOURCE` and `STYXRPC_DISVM_BUILD` to include the DisVM cases.
2. Run `ctest --test-dir build-styxrpc -R 'styxrpc-(mutable-tree|disvm-mutations)' --output-on-failure`. The [driver](mutations.py) creates a fresh socket pair per run. It launches the provider on one end and either the C or the DisVM caller on the other.
3. The [C client](mutation_client.c) creates a directory, creates and writes a child, and checks that the server refuses duplicate creation and removal of a nonempty directory. It then unlinks the child, reads the original bytes from a retained fid and removes the empty directory.
4. The [Limbo client](mutations.b) mounts the C tree at `/device`. It calls `Sys.create(path, Sys->OWRITE | Sys->OEXCL, 8r666)` to obtain a writer, checks a read-only parent and a reserved node, and recreates the name after `Sys.remove`. The old and new readers return different data.
5. Each client prints `PASS` after its checks. The provider exits when the session closes and verifies that all its cookies were released. A test failure includes the child process's diagnostic.

Both clients send standard Styx requests. The SDK provides a directory creation helper as well as the file helpers. The mounted Limbo subset covers exclusive file creation and removal.
