# Interface snapshots / Зафиксированные прототипы интерфейсов — v0.284

C89 `fvm_exec_implements` installs interface prototype references in loaded SWF7–8.
It reads own stored prototype data without getters, ignores primitive/invalid
interface entries and does not require callable class objects. The first list is
retained, including an empty list produced by invalid entries. A zero input count
is a noop. SWF4–6 leave interface state unchanged. Replacing a constructor's
prototype afterwards does not retarget existing interface edges. Host-only calls
keep the previous replaceable constructor-reference policy.

`fvm_exec_interface_count` handles ImplementsOp count conversion independently
of bytecode: loaded objects yield zero without valueOf; primitives wrap to Int32
and clamp to available values. Hosts keep integral bounded numeric validation.
The interpreter adapts class/count/interface operands to these C services. This
count policy follows the pinned current-player reference; historical player
versions that coerced object counts are not certified by these fixtures.

[GC:] One private vector retains either legacy constructor references or snapshot
prototype references. An explicit mode flag distinguishes them and remembers an
installed empty snapshot list. The collector traces every edge in either mode.
All declaration inputs are rooted before allocation. A prepared list is published
only after all allocations succeed, leaving state unchanged on failure.

Graph schema 9 adds one validated interface-edge-mode byte per object. It is
selected only if a graph contains installed snapshot state; other graphs retain
the byte-compatible schema 8 layout. Schemas 1–8 still decode with legacy edges.
Graph identity, shared references, empty-list installation and private edges are
restored together. File and network frame readers accept schema 9. Older binaries
cannot read these new snapshot graphs; their graphs remain readable here. Native
callback IDs and resource-kind identities are unchanged. Canonical private helper
records continue rejecting unexpected interface state.

В SWF7–8 сохраняются сами прототипы интерфейсов на момент объявления, а не ссылки
на изменяемое поле конструктора. Повторное объявление не заменяет первый список,
даже пустой. В SWF4–6 состояние не меняется. Getter prototype не вызывается.
GC, очередь, файл и новый формат графа сохраняют те же объекты и режим списка.
Обычные графы остаются в формате v8; формат v9 нужен только новым снимкам.

Tests cover version gates, accessor suppression, frozen identity, repeated/empty
lists, object/fractional/negative/bounded counts, actual actions, GC, atomic
allocation failures and decoder rollback, graph/checksum validation, shared identity and memory/file
channel transport. Full builds additionally exercise localhost broker transport.
Existing legacy serialization/host inheritance fixtures remain intact.
All nine roadmap stages and complete property/interface/platform conformance stay open.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`action_implements_op` and
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
`set_interfaces` (first installation) / `prototype` (own stored data).

Validation: final Linux AGG/Cairo runs each pass 555 tests without launch failures.
The earlier 554-test runs needed execute-bit restoration and successful reruns
for 6/5 binaries respectively. Nine affected heap/global ASan+UBSan checks pass
(stack/leak coverage excluded). The file/localhost broker paths pass in both full
OS_PORTS profiles; the limited sanitizer profile checks memory/file transport.
All 26 unchanged original SWFs match exactly in each profile.
