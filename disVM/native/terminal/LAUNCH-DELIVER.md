# styxlaunchpublicationdeliver

[Русский](LAUNCH-DELIVER.ru.md) · [API](launch_publication.h) · [TCP example](../../examples/vfs-run/TUTORIAL.md) · [Receiving inbox](UNIBUS-INBOX.md)

`DVM_LaunchDelivery styxlaunchpublicationdeliver(StyxLaunchPublication *publication, int wait, DVM_Provider *peer_unibus, unsigned long tag);`

Link `styx_terminal`. Supply a live publication, a retained writable single-file
provider for the recipient's `.unibus`, and a nonzero 32-bit reply tag. The
function borrows these references for the call. With nonzero `wait` it may wait
for the child. With zero `wait` it checks for completion without blocking in
`waitpid`. Status observation is serialized.

The result is delivered, not sent, or uncertain. A completed child yields one
S-record containing its decimal exit status followed by newline, written to a
fresh fid. A pending child, invalid input or refusal yields not sent. Transport
uncertainty is preserved. The caller controls when this operation runs and
does not automatically retry a possibly delivered record.

The TCP fixture publishes `.unibus` on both sides of one bidirectional
carrier. The caller writes the command to the owner's file on logical channel
0. The owner writes its result to the caller's file on logical channel 1.
The fixture also checks the existing ordered-stream and result-file interfaces.
