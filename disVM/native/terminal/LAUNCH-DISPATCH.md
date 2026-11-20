# styxlaunchpublicationdispatch

[Русский](LAUNCH-DISPATCH.ru.md) · [API](launch_publication.h) · [Publication](LAUNCH-PUBLICATION.md) · [TCP example](../../examples/vfs-run/TUTORIAL.md)

`int styxlaunchpublicationdispatch(StyxLaunchPublication *publication, uint64_t offset, const void *bytes, size_t size);`

Link `styx_terminal`. Call from a Styx write handler before handing bytes to
the existing Unibus record stream. Borrow an initialized publication, the byte
offset within this fid, and the write buffer with its size in bytes. Reserve
the publication's launch tag independently of ordinary Unibus records.

A complete S-record in the first write of a fresh fid can select the launch
route. Return 1 to acknowledge its admission, or -1 to reject a matching request.
Return 0 leaves the write for ordinary stream processing. Later write fragments
remain with the stream. Launch delivery uses one complete write, so the
transport's write limit applies. Admission is serialized by the publication.
The owner authorizes access to the Styx endpoint and keeps its publication alive.

In the TCP example, `runargs("/routed.dis", payload)` sends through the mounted
`.unibus`. After owner `init`, the client reads status from `/run-result` and
continues ordinary Unibus traffic through the same `.unibus` node.
