# Source distributions

Copyright (c) 2026 bhgv

[Русский](DISTRIBUTION.ru.md) · [Libraries](README.md) · [Acceptance](VALIDATION.md)

## Purpose and contents

Each archive is a complete independent source project: C implementation, headers,
CMake files, licenses, EN/RU reference/tutorial pages, examples and the offline
viewer. Styx and libunibus remain separate archives with separate build options;
neither needs the other or either VM. They are source releases, not precompiled
binaries or a promise of acceptance on every hardware target.

The repository maintenance tool [package.py](package.py) reads exact committed
Git blobs. It never includes working-tree edits, untracked build products or local
SDK files. File modes are reduced to ordinary 0644/0755, owner names/IDs are empty
or zero, and archive timestamps use the selected commit time. Gzip carries no
host filename. Repeated execution with the same commit and compression toolchain
produces identical archive bytes, regardless of output directory or local Git
archive filters/settings. There is no network access.

Each archive contains a generated `RELEASE.json`: format version 1, package name,
full repository commit, and the sorted list of source paths, executable modes
and SHA-256 hashes. It describes all source files, excluding itself. The separate
`SHA256SUMS` covers both complete compressed archives. Hashes detect corruption;
they do not authenticate a publisher. Original license notices remain intact.

## Create both archives

Run from the repository root with Git and Python 3.9 or later:

```sh
python3 lib/package.py --revision HEAD --output build/library-distributions
cd build/library-distributions
sha256sum -c SHA256SUMS
```

`--revision` accepts a committed Git revision, defaults to `HEAD`, and is resolved
once for both packages. Uncommitted library changes must be committed before they
can appear in a release. `--output` is required and selects the destination
relative to the current working directory (an absolute path is also accepted).
Existing archives with the same names and `SHA256SUMS` are replaced. Other files
are left in place; use a new directory to avoid retaining unrelated old releases.

Filenames are `styx-<12-hex-commit>.tar.gz` and
`libunibus-<12-hex-commit>.tar.gz`. Each has just its own `styx/` or `libunibus/`
root. Nonregular Git entries, links and a tracked `RELEASE.json` collision are
rejected. A package archive is replaced only after successful compression; the
two archives and checksum list are not one atomic filesystem transaction. On
any failed invocation, fix the cause and rerun before distributing the set.

## Use an archive in another project

Copy the chosen archive and its checksum receipt to your project machine, verify
the receipt, then unpack it. The archive does not require this repository or Git
to configure/build. For example, after unpacking Styx:

```sh
cmake -S styx -B styx-build -DSTYX_HOSTED=OFF -DBUILD_TESTING=ON
cmake --build styx-build
ctest --test-dir styx-build --output-on-failure
```

For the smallest libunibus configuration:

```sh
cmake -S libunibus -B unibus-build -DUNIBUS_QUEUES=OFF -DBUILD_TESTING=ON
cmake --build unibus-build
ctest --test-dir unibus-build --output-on-failure
```

The [Styx README](styx/README.md) and [libunibus README](libunibus/README.md)
describe hosted client/server/queue configurations, installation and external
CMake consumers. Run `python3 view_docs.py` inside either unpacked project to
browse its own documentation. The minimal C libraries do not require Python;
hosted Styx configuration and some examples/tests do.

## Acceptance of the actual deliverables

```sh
python3 lib/tests/package_contract.py
python3 lib/tests/distribution.py /absolute/path/to/cmake --revision HEAD
```

The first check uses a disposable Git fixture to cover dirty/untracked isolation,
exact binary bytes and executable modes, export-ignore/local archive settings,
and rejection of links/reserved paths.

The [distribution acceptance](tests/distribution.py) creates each archive twice,
requires identical bytes, checks each manifest/hash/mode, and unpacks only verified
regular files. It builds all four hosted/minimal profiles outside the repository
with an unavailable C++ compiler. It then installs and relocates each SDK, builds
external C and C++98 consumers, checks installed docs/viewers, and runs five
Styx-server models through an independently linked Unibus gateway. C++ consumers
test C declarations; neither implementation becomes C++.

Temporary extraction/build trees are removed on exit. The persistent diagnostic
log is `build/library-distribution-acceptance.log`; failures propagate as a nonzero
exit. The full harness needs Linux amd64, the compiler/toolchain, Git, Python,
and `ctest` next to the supplied `cmake`. This is host acceptance, not a physical
SPI/CAN/radio or MCU certification. See [maintenance limits](MAINTENANCE.md).
