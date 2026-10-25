# Hybrid heartbeat policy

An executable virtual-time demonstration, without sockets/network traffic: UDP
idle checks every 30 seconds, separate TCP checks every 300 seconds, and an early
TCP check requested by authenticated UDP advice. Periods are illustrative, not
NAT timeout recommendations.

```sh
cmake -S flashVM -B build-heartbeat -DFVM_BUILD_EXAMPLES=ON
cmake --build build-heartbeat --target flashvm_examples_embedding_hybrid_heartbeat_main
./build-heartbeat/flashvm_examples_embedding_hybrid_heartbeat_main
```

Requires a C89 compiler and CMake; use `-S .` inside the component. This target
links only flashvm_srecord, without VM/SDL. All storage is on the stack; no heap
or cleanup is required. Expect separate UDP/TCP decisions, TCP receive-silence
remaining after send, an explicit receive transition, and an encoded hint. A real
host reports only successful I/O, authenticates before accept, serializes state
and enforces aggregate quotas/jitter. Authentication, STUN, broadcast/multicast,
NAT traversal and network listeners are not implemented by the tutorial.

| API | Contract and ownership |
| --- | --- |
| [`fvm_rheartbeat_accept`](../../../include/flashvm/rheartbeat.h#L104) | Accept already authenticated advice after epoch, sequence and permission checks. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |
| [`fvm_rheartbeat_activity`](../../../include/flashvm/rheartbeat.h#L96) | Report actual successful send/receive on exactly one path; UDP never refreshes TCP. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |
| [`fvm_rheartbeat_clear`](../../../include/flashvm/rheartbeat.h#L111) | Host declines reconnect advice; no liveness or network action is fabricated. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |
| [`fvm_rheartbeat_due`](../../../include/flashvm/rheartbeat.h#L89) | Advance virtual monotonic time and suggest actions without acknowledging I/O. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |
| [`fvm_rheartbeat_hint_write`](../../../include/flashvm/rheartbeat.h#L124) | Encode compact canonical fields and CRC into caller storage; no authentication is added. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |
| [`fvm_rheartbeat_init`](../../../include/flashvm/rheartbeat.h#L82) | Copy caller-owned policy, establish the authenticated epoch and start independent ages. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |
| [`fvm_rheartbeat_next`](../../../include/flashvm/rheartbeat.h#L118) | Generate one outgoing nonwrapping hint identity; retain it for retry. [Contract](../../../doc/HYBRID_HEARTBEAT.md). |

[Source](main.c), [guide](../../../doc/HYBRID_HEARTBEAT.md), [catalog](../../README.md#example-embedding-hybrid_heartbeat).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rheartbeat_accept` | function | [include/flashvm/rheartbeat.h:104](../../../include/flashvm/rheartbeat.h#L104) |
| `fvm_rheartbeat_activity` | function | [include/flashvm/rheartbeat.h:96](../../../include/flashvm/rheartbeat.h#L96) |
| `fvm_rheartbeat_clear` | function | [include/flashvm/rheartbeat.h:111](../../../include/flashvm/rheartbeat.h#L111) |
| `fvm_rheartbeat_due` | function | [include/flashvm/rheartbeat.h:89](../../../include/flashvm/rheartbeat.h#L89) |
| `fvm_rheartbeat_hint_write` | function | [include/flashvm/rheartbeat.h:124](../../../include/flashvm/rheartbeat.h#L124) |
| `fvm_rheartbeat_init` | function | [include/flashvm/rheartbeat.h:82](../../../include/flashvm/rheartbeat.h#L82) |
| `fvm_rheartbeat_next` | function | [include/flashvm/rheartbeat.h:118](../../../include/flashvm/rheartbeat.h#L118) |
<!-- source-links:end -->
