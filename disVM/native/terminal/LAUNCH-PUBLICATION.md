# Styx launch publication

[Русский](LAUNCH-PUBLICATION.ru.md) · [API](launch_publication.h) · [Execution](LAUNCH-PROCESS.md) · [Unibus dispatch](LAUNCH-DISPATCH.md)

`styxlaunchpublicationinit` initializes caller-owned storage for an authorized,
one-shot command. Supply a nonzero S-record tag, the owner's working directory
and an argument vector with an absolute executable. Leave the publication at
a stable address and retain the configuration through submission. The return
value is 1 on success or 0 on invalid input or initialization failure.

`styxlaunchpublicationfile` returns an owned single-file provider reference.
Select 0 for the write-only submit file or 1 for the read-only result file.
The host publishes these capabilities using its normal VFS or Styx server.
Submit accepts one valid request frame. The result file streams the decimal
exit status followed by a newline, then EOF. Reading before submission fails.
Reading after admission can wait for the child. Each open has an independent
read position. The result metadata uses a zero length for this stream.

`styxlaunchpublicationfinish` retires the publication after all providers
and opens have been released and can wait for the child. It returns 1 and
writes the exit status when available, 0 if no child started, or -1 on failure.
Call it once for successfully initialized storage. The owner controls which
clients receive the submit and result capabilities.
