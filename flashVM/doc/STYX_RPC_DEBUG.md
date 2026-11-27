# Debug AVM1 in the paired player

[Русский](STYX_RPC_DEBUG.ru.md) · [Paired player](STYX_RPC_PLAYER.md) · [VM debugger API](DEBUG_API.md) · [Playback adapter](../src/styxrpc/playback_debug.c)

The stock paired player can stop inside an interpreted timeline action.
The controlling program uses the existing Styx mount, for example
`/flash/.state` in DisVM. The player attaches the debugger when it receives
`debug`. This pauses timeline ticks and selects the interpreter until the
movie is replaced. Existing embedders can supply their own debugger callback.

Open a fresh write fid at offset zero for each command. Finish it with an
optional newline. Write errors indicate that the command was rejected.

| Command | Effect |
| --- | --- |
| `debug` | Pause and install the breakpoint handler in the uploaded movie. |
| `break <offset> [function]` | Set a breakpoint at the decimal byte offset in the named AVM1 function. Omit the function for a top-level block. |
| `clear <offset> [function]` | Remove that breakpoint. |
| `start` | Start the paused timeline until it encounters an enabled breakpoint. |
| `continue <stop>` | Continue the current activation from the given stop ID. |
| `step <stop>` | Execute from that stop and stop again at the next interpreted action boundary. |

For a fresh movie with actions in its first frame, send `debug`, `break 0`
and `start`. Open a fresh read fid for each snapshot. Wait for
`debug.state=stopped` and obtain `debug.stop`. For the first stop in the
connection send `clear 0` and `step 1`. The next stop has a new ID. Send
`continue` with that ID to let the frame finish. A stale ID fails without
resuming the movie. Break and clear also work while stopped. Function names in
commands are counted byte strings of up to 127 bytes. Line feeds and NUL
are excluded. Same-named functions share their breakpoint table.

At a stop, `state=debug` and the `debug.*` fields give the ID, byte offset,
mapped source and line, and up to four cached innermost frames, each with the
offset, [engine](../include/flashvm/crash.h#L29) and name. Source is empty and
the line is zero if no source map is attached. Nonprintable and non-ASCII
bytes in the displayed labels become `?`. Outside a stop, `debug.state=armed`
indicates that the loaded context has the debugger attached.

At an actual breakpoint the timeline's VM call is still on the stack.
The player services `.state` while holding it and refuses unrelated file
I/O, upload and restart. Connection loss aborts that execution. This
interface stops timed timeline execution. It does not stop a callback
already running inside an ordinary Styx request. Upload and restart outside
a stop replace the context and its breakpoints. Stop IDs do not repeat on
that connection.

Run the `styxrpc-native-debug-*` tests after configuring libstyxrpc with
the paired player executable and DisVM build. They verify the VM's actual
breakpoint location, single action stepping, stale IDs, I/O restriction
and resumed SWF execution on local sockets and TCP.
[Limbo fixture](../../lib/styxrpc/tests/flashvm/native_debug.b).
