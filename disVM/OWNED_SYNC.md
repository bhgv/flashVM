# Owned operations and Inferno synchronization

[Русский](OWNED_SYNC.ru.md) · [Guarded owner tutorial](../common/owned/README.md) ·
[Transactional table splice](../common/table/SPLICE.md)

The selected interpreter and bootstrap snapshots are pinned to Inferno experiment
`152ab5ae4f315d1479d3a508796a8b56d5048c66`. This includes bounded priority-architecture
CALL/MCALL/RET and FRAME/MFRAME handling, captured frame publication, descriptor
map preflights and checked optimizer use/def byte ranges. Imported sources remain
byte-exact, including their license notices. The frame/control guest matrices
exercise AMD64 and ARM64; physical ARM64 execution remains unverified.

The list overlap-copy oracle correction was developed in the owning Inferno
repository and published there first. It admits complete eight-byte source loads
within the requested span, alongside byte tails. It still rejects out-of-span
reads and any payload access on a refused copy. The old byte-only oracle rejected
legal production copy kernels; no production memory guard was relaxed.

FlashVM-only additions are separate: a POSIX operation-token recursive guard,
a guarded native scalar table owner, and a transactional dense-prefix replacement
API. Native-thread tests protect complete read-modify-write transactions through
nested callbacks. An executable tutorial demonstrates self insertion and explicit
owner lifetime. Allocation-failure injection checks splice rollback and storage
accounting; sparse-key collisions are rejected without destroying dictionary data.

These additions do not yet provide per-object locking inside DisVM array/string
storage, remote operation lifetime integration, scheduler-aware native waits,
Limbo/FIR resized slice assignment, or regexp slices/capture replacement. Plain
Fht clients still serialize their own access. Existing native Dis array views
continue sharing storage under their established rules. Do not treat a host guard
library as completed VM integration or a new language opcode.
