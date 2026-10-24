# Selected upstream synchronization, 0.1.1

[Русский](SYNC.ru.md) · [Manifest](UPSTREAM.json) · [Update workflow](../tools/UPSTREAMS.md)

Authoritative repository: https://github.com/bhgv/Inferno-OS-bhgv, **experiment**.
Previous revision: fa2366ca2acaee899da4a8b2e450d5d2629e279b.
Selected revision: 3e3536578b969cf1a460210d8ec667713869ccfc.
FlashVM patch base: 6c6f1c70ae9748372e0419ac5d2ca1a41153fc63 (published 0.1528).

The new commit adds descriptor-index native leaf paths and fixes aggregate slot
access, slice geometry and raw-channel reference ownership. Sources are imported
byte-for-byte from that exact Git commit. No local edits are hidden in upstream/.
No new upstream bug was demonstrated by the checks, so no upstream fix/patch or
push is included in this synchronization.

## Strict source selection

The DisVM manifest owns **109 files** (previously 97). The 12 additions are only
six C/Python regression pairs: array-indices, descriptor-index, heap-bounds,
reference-slots, string-branches and channels. descriptor-index.c is also an input
to the updated native JIT fixture. Existing interpreter/emitter/channel fixture
files are refreshed in place. No whole OS or additional runtime subsystem is
imported. Upstream checkout history stays outside the FlashVM repository.

Draw/Tk/Prefab, applications, fonts/resources, window servers and Keyring service
implementations remain excluded. Retained generated/interface declarations do not
link their implementations. Future executable-host work must select only the
required scheduling, allocation, module registration and I/O boundaries. The
current archive is still not EMU_EXECUTABLE; the upstream 64-bit host/ABI port
remains incomplete. A headless whole-Inferno build is not substituted for DisVM.

The separate temporary compiler under tools/limbo-bootstrap retains **174 files**.
Its selected files are unchanged by the new upstream commit; only its pinned
revision advances to keep provenance aligned. Our own Coco/R compiler remains
the priority. A future incompatible compiler/VM ABI must be detected, not silently
assumed compatible because both snapshots come from the same repository.

## Executed checks, Linux AMD64

- Source-built native bootstrap: 1/1 test.
- DisVM core archive compilation, AMD64 backend: passed.
- Selected DisVM regressions: 15/15; native AMD64 execution in WORD32 and WORD64
  fixture models included.
- Six newly selected regression drivers: passed separately with
  `-fsanitize=undefined -fno-sanitize-recover=all`.
- Both imported manifests verify exact SHA-256 contents and reject extra files.

Fixture models have injected host boundaries. These checks are not execution of
a complete Limbo application in our DisVM, real ARM64/RISC-V execution, full
scheduler/GC acceptance or native Dis/SWF interoperation. Existing compiler/IDE
sources are unchanged by this patch; their previous suite is not rerun here.

## Repeatable update and upstream fixes

Use an explicit local checkout of the owner's experiment branch and the commands
in [UPSTREAMS](../tools/UPSTREAMS.md). Refresh by exact committed revision, review
changes/selection, then build and test. Builds never fetch a moving HEAD.
Confirmed interpreter/JIT defects must first be fixed and tested on a local work
branch of Inferno-OS-bhgv and exported as a separate patch for that repository;
then refresh this manifest from that exact fix commit. Preserve licenses and
upstream source formatting. FlashVM and upstream patches remain separate for
manual application; do not mix cross-repository commits into one git-am series.
