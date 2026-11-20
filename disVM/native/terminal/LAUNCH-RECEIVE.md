# styxterminallaunchreceive

[Русский](LAUNCH-RECEIVE.ru.md) · [API](launch_receive.h) · [Caller](../../runtime/VFS-RUN.md)

Link `styx_terminal` and use `launch_receive.h`. For each authorized one-shot
Styx file, initialize a `StyxLaunchReceiver` with its fixed S-record tag,
owner start callback, callback state, and `used = 0`. Call
`styxterminallaunchreceive(&receiver, offset, bytes, size)` from the file's
serialized write handler. Offset and size are byte counts. The function borrows
the frame and receiver during the call.

A complete matching frame with an empty payload invokes the owner callback
once. A 1 return acknowledges admission. A 0 return indicates invalid input,
a spent receiver, or callback refusal. Invalid input leaves the receiver
available. Invoking the callback spends it even if the callback returns 0.
The host supplies access control, serialization and callback lifetime. The
callback selects the local command and schedules its execution.
