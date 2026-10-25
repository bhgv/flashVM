# Original function interfaces: processes, threads and LAN

The C89 tutorial in `examples/embedding/function_rpc/` uses the same native
function in three arrangements. The original function and userdata remain in
the source VM. The receiving VM holds a callable interface. All calls use the
broker's one TCP listening port and carry routing/call IDs.

Build a native socket-enabled example:

```sh
cmake -S . -B build-rpc -DFVM_DEFS_FILE=cmake/profiles/minimal.defs
cmake --build build-rpc --target flashvm_examples_embedding_function_rpc_main
```

On Windows, use the generated `.exe` and its configuration directory when the
selected generator creates one; arguments are the same.

## Two independent FlashVM runtimes on different threads

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main threads
```

The source runtime hands its context exclusively to the broker service thread.
[`fvm_worker_run`](../include/flashvm/worker.h#L25) creates a second independent runtime and context on a worker
thread. Only a primitive URI string is copied into the worker. It imports the
interface and invokes the original over TCP, even in one process. A VM object,
function address or userdata pointer is never passed to the recipient VM.
After joining the worker, the host reclaims the source context and checks its
original call counter. Output includes `two independent runtime threads via TCP
passed` and `original calls=4`.

## Two independent processes on one computer

Start the source in terminal A:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main serve
```

Copy its first output line (the complete `fvm://...` URI). In terminal B:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main call "URI_FROM_TERMINAL_A"
```

The client verifies result 42, the three-byte `C\0E` exception, unsupported and
ordinary error returns. After the client finishes, press Enter in terminal A.
The source reclaims its heap, checks four original userdata calls and shuts down.
No native callback serialization codec is registered in either process.

## Two computers connected by LAN

Build the example on both computers. If computer A has LAN address
`192.168.1.10`, run on A:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main serve 0.0.0.0 192.168.1.10 9210
```

`0.0.0.0` selects local listening interfaces; `192.168.1.10` is the address
published inside the URI, reachable by computer B. To listen only on the LAN
interface, use `192.168.1.10` for both arguments. A reachable DNS name is also
accepted as the advertised host. TCP port 9210 must be reachable from B to A.
The printed URI is a bearer capability: give it only to the intended client.

On computer B, copy A's complete URI into:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main call "URI_PRINTED_BY_COMPUTER_A"
```

The same four results are checked. Then press Enter on A to reclaim the source
context and stop. Replace the sample LAN address/port with the actual values;
never advertise `127.0.0.1` or `0.0.0.0` to another computer. All original
functions share the selected listening port; clients use ordinary outbound TCP
connections, not additional listening ports. Port 0 requests automatic port
selection; the URI contains the actual selected port.

Local automated checks cover independent VM threads, independent processes,
wildcard/interface binds, IPv4/DNS advertised hosts and malformed ports. They do
not certify execution on two physical computers or on foreign operating systems.
See [ownership and limitations](REMOTE_FUNCTIONS.md); cross-authority callback
cycles, persistent socket multiplexing and unrestricted concurrent heap access
remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_worker_run` | function | [include/flashvm/worker.h:25](../include/flashvm/worker.h#L25) |
<!-- source-links:end -->
