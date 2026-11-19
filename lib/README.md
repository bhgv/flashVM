# Independent communication libraries

Copyright (c) 2026 bhgv

[Русский](README.ru.md)

These are two separate, redistributable source projects. Neither project builds,
loads or imports DisVM, FlashVM, Limbo, SWF, a GUI toolkit, or a sibling directory.
Copy either directory to another checkout and configure it there.

| Project | Purpose | Smallest configuration | Hosted configuration |
| --- | --- | --- | --- |
| [Styx](styx/README.md) | Original Inferno 9P2000 codecs, native client/server, replaceable physical carrier | C99 codec, standard byte/string operations, no OS/threads/allocator calls | Linux64, libc/libm/pthread; examples optionally use Python3 |
| [libunibus](libunibus/README.md) | Existing binary S-record codecs, nested client namespaces, sequence/CRC5 and per-tag queues | C89 allocation-free wire functions | C99/pthread queues; optional POSIX gateway example |

The optional example connecting the two projects uses their wire interfaces.
It is not a link dependency: the Unibus gateway does not link libstyx; the Styx
server does not link Unibus. The same file-server models can be used as raw 9P
or inside explicitly agreed nested S-records.

## Documentation

- [Styx English guide](styx/README.md), [Russian guide](styx/README.ru.md), [examples](styx/examples/README.md).
- [libunibus English guide](libunibus/README.md), [Russian guide](libunibus/README.ru.md), [examples](libunibus/examples/README.md).
- Each copied directory owns `view_docs.py`; run `python3 view_docs.py --no-browser --port 8910` inside it.
- Reference pages follow a dictionary layout: syntax, parameters, description,
  return value, errors, availability, examples, related entries and source links.

## Source relationship

`SOURCES.jsonl` records canonical repository paths, checksums and explicit export
transformations. These are external-project source distributions, not implicit
includes into the engines. The checked-in protocol implementations retain their
existing public names so an extraction does not silently fork the wire format.

`python3 lib/export.py --check` checks the extraction against the canonical
implementations. It is a workspace maintenance tool, not a package build dependency.
Inferno license notices are preserved; the workspace's MIT license does not
relicense imported Inferno components.

## Acceptance

The packages have their own CMake/CTest entry points. Workspace interoperability
can additionally be checked with:

```sh
python3 lib/tests/interoperability.py build/standalone-styx build/standalone-unibus
```

This runs five file-server models through independently linked projects, with two
clients reusing the same internal tags/fids. It supplements, rather than replaces,
each package's independent acceptance.

[Acceptance and limits](VALIDATION.md)

[Maintenance scope and remaining inherited work](MAINTENANCE.md)

## Source archives

[Create and verify independent source distributions](DISTRIBUTION.md). Each package
owns its sources, licenses, examples and offline help; exact Git snapshots produce
reproducible archives with per-file manifests and SHA-256 receipts.
