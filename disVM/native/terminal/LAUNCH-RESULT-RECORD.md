# styxlaunchpublicationresultrecord

[Русский](LAUNCH-RESULT-RECORD.ru.md) · [API](launch_publication.h) · [Publication](LAUNCH-PUBLICATION.md) · [TCP example](../../examples/vfs-run/TUTORIAL.md) · [Reverse Styx delivery](LAUNCH-DELIVER.md)

`int styxlaunchpublicationresultrecord(StyxLaunchPublication *publication, int wait, const FVM_SOrdinal *ordinal, unsigned long tag, void *bytes, size_t capacity, size_t *used);`

Link `styx_terminal`. Pass a live publication, the next ordinal for the
recipient's reply tag, a nonzero 32-bit tag, a caller-owned output buffer and
capacity in bytes, and a writable output length. With `wait = 0`, an unfinished
command returns pending without a blocking waitpid. With nonzero `wait`, the call may wait for the child.
The publication serializes process status observation.

A result of 1 provides an ordered packet containing one S-record. Its payload
is the decimal process exit status followed by newline. A result of 0 means
no completion is available. A result of -1 indicates invalid arguments, a wait
failure, or an encoding error, including insufficient capacity. The caller
assigns the ordinal and publishes the resulting packet on its authorized lane.

The TCP fixture reserves reply tag 17, starts it at ordinal zero, and exposes
it to the recipient as `/launch-exit`. The client reads the result alongside
ordinary Unibus records. The example also checks the separate result file.
