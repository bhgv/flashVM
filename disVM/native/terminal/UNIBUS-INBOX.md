# styxunibusinbox

[Русский](UNIBUS-INBOX.ru.md) · [API](unibus_inbox.h) · [Reverse sender](LAUNCH-DELIVER.md) · [Example](../../examples/vfs-run/TUTORIAL.md)

`DVM_Provider *styxunibusinbox(unsigned long tag);`

Link `styx_terminal`. Pass the nonzero 32-bit tag allocated for the incoming
reply. Returns an owned single-file provider reference, or NULL on invalid
input or allocation failure. Use ordinary provider release to retire it.
Open fids retain independent lifetime references.

Publish write-only access to the peer as its authorized `.unibus` route,
and read-only access in the receiving Limbo namespace. The provider accepts
one complete S-record of up to 64 encoded bytes. Wrong tags, incomplete
frames and repeat writes are refused. Supported opens are `r` and `wo`.
Each read open has its own sequential cursor. Before a record arrives a
nonempty read reports pending as an I/O error. After arrival reads yield the
wire bytes and then EOF. The provider does not wait for a future record.
Concurrent publication is serialized and cancellation can refuse admission.

The TCP example maps the local inbox as `/reverse-exit`. The owner writes
to the caller's `.unibus` on the reverse Styx channel. Limbo reads the same
record from its read-only alias through `Sys->read`.
