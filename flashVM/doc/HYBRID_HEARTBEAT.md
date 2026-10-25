# Hybrid UDP/TCP policy

The C89 `rheartbeat.h` module lets a host choose a frequent UDP idle check, a
separate slower TCP check and rate-limited TCP/reconnect advice arriving over an
authenticated UDP path. It is implemented policy plus a compact hint codec, not
a STUN server, socket adapter or network-authentication implementation.
[Executable policy tutorial](../examples/embedding/hybrid_heartbeat/README.md),
[resource observers](RESOURCE_EVENTS.md), [debugger](RESOURCE_DEBUGGER.md).

## Cost and distribution

The suggested split is useful, but a TCP heartbeat on an established connection
does not repeat its handshake. UDP's smaller header does not alone establish
lower total CPU/radio/battery cost. Frequency, acknowledgements, authentication,
NAT behavior and wakeups matter. RFC 8085 section 3.5 specifically explains why
less frequent TCP keepalives can use fewer resources than UDP keepalives.
[RFC 8085](https://www.rfc-editor.org/rfc/rfc8085.html#section-3.5).

LAN broadcast/multicast is a possible adapter for discovery/group notifications;
it is not general Internet delivery through NAT. Across hosts on different
networks use authenticated unicast fan-out/relay, with per-recipient resource
rights. One resource owner serializes mutating debugger commands; independent
observers do not need competing access to one consuming queue. The event API
provides application fan-out, not socket multicast or an elected distributed
controller. Group traffic also needs aggregate pacing/congestion control and
bounded feedback. [RFC 8085 section 4](https://www.rfc-editor.org/rfc/rfc8085.html#section-4).

UDP activity never keeps a separate TCP NAT mapping alive. A TCP probe must
traverse that TCP connection. STUN's returned address belongs to its observed
transport/connection, not an arbitrary future TCP connection. The host may use
UDP advice to schedule a TCP check or establish a new configured TCP session.
[RFC 8489 section 6.3.1.1](https://www.rfc-editor.org/rfc/rfc8489.html#section-6.3.1.1).
Application checks here are distinct from the OS TCP SO_KEEPALIVE facility.
[RFC 9293 section 3.8.4](https://www.rfc-editor.org/rfc/rfc9293.html#section-3.8.4).

## Native API

All policy state/configuration is caller-owned and contains no managed pointers.
Initialize every policy member, including exact struct_size. Calls on one state
must be serialized by its host; keep outputs disjoint from live state; there are no internal locks, allocation, clocks,
sockets, waits or VM calls. Error returns preserve state and outputs. The C89
module is also part of the standalone flashvm_srecord library.

| Function | Arguments, outcome and lifetime |
| --- | --- |
| [`fvm_rheartbeat_init`](../include/flashvm/rheartbeat.h#L82) | State output, copied policy, nonzero Uint32 authenticated epoch and current monotonic Uint32 milliseconds. Exact ABI sizes, transport/hint masks and timer bounds are checked. Hints require both paths and positive request_gap_ms. Zero periodic/deadline timer disables it; there are no universal NAT defaults. Reinitialization resets replay state: use a new authenticated epoch on reconnect, not a reused live epoch. |
| [`fvm_rheartbeat_due`](../include/flashvm/rheartbeat.h#L89) | State, now_ms and action output. Advances elapsed ages and suggests SEND_UDP/SEND_TCP, independent UDP_STALE/TCP_STALE and pending RECONNECT advice. Does not mark any transmission successful. Repeated poll may return the same unsent action. Stale is suspicion, not proof of peer death or permission to roll back commands. |
| [`fvm_rheartbeat_activity`](../include/flashvm/rheartbeat.h#L96) | State, exactly UDP or TCP, received=0/1 and now_ms. Call only after actual successful path activity. Send resets idle; receive also resets silence. UDP never refreshes TCP counters. TCP activity serves a pending probe request but leaves reconnect advice for an explicit host decision. |
| [`fvm_rheartbeat_accept`](../include/flashvm/rheartbeat.h#L104) | State, copied decoded hint and now_ms, only after transport authentication/authorization. Wrong epoch or disallowed flags DENIED, repeated/out-of-order sequence CONFLICT, malformed input/clock INVALID. On OK records UDP receipt, advances its sequence and coalesces advice. No VM/debug command or packet destination is accepted. |
| [`fvm_rheartbeat_clear`](../include/flashvm/rheartbeat.h#L111) | State, nonempty hint mask and now_ms. Host acknowledges/declines advice and restarts the request throttle. It does not refresh actual path idle/receive counters. No network action is performed. |
| [`fvm_rheartbeat_next`](../include/flashvm/rheartbeat.h#L118) | State, outgoing hint flags and hint output. Generates a strictly increasing Uint32 sequence in this epoch; exhaustion LIMIT without changes. Local incoming permissions do not grant permissions on the peer. Retain this hint for an I/O retry instead of generating a new identity. |
| [`fvm_rheartbeat_hint_write`](../include/flashvm/rheartbeat.h#L124) | Nonzero tag, hint, output/capacity and used. Encodes canonical unsigned S integers `version,epoch,sequence,flags` in a checked record. Invalid fields S_INVALID; capacity S_LIMIT. Output/used unchanged on error. This does not add a MAC/authentication tag. |
| [`fvm_rheartbeat_hint_read`](../include/flashvm/rheartbeat.h#L131) | Incoming binding table, counted bytes, hint output and consumed. Requires schema flashvm.transport.hint/version 1, verifies CRC and exact four canonical unsigned fields. Incomplete S_MORE, unknown binding S_UNKNOWN, invalid schema/fields S_INVALID; framing errors propagate. Outputs change only on S_OK. Decoding alone does not authorize accept. |

[`FVM_RHeartbeatPolicy`](../include/flashvm/rheartbeat.h#L28), [`FVM_RHeartbeat`](../include/flashvm/rheartbeat.h#L61), [`FVM_RHeartbeatHint`](../include/flashvm/rheartbeat.h#L49) are native records,
not wire layouts. Timers use milliseconds and range 0..[FVM_RH_MAX_INTERVAL](../include/flashvm/rheartbeat.h#L18).
The monotonic clock wraps at Uint32; ordinary wrap works, rollback or an ambiguous
gap greater than 2^31-1 ms is INVALID. Poll at least within that half-range (about
24.8 days). Ages saturate, so a long silence cannot become fresh after wrap.

Regular traffic postpones idle probes. Received traffic alone refreshes that
path's liveness evidence. A requested TCP check waits for request_gap_ms since
the last handled advice/TCP activity, and sufficient TCP idle time. Reconnect
advice shares this throttle; the host may decline it. Permissions default to
deny when zero-initialized. A new valid hint cannot bypass the throttle.

## Transport requirements and limits

Authenticate unicast ingress before accepting a hint. CRC, epochs and increasing
sequence numbers are not credentials. Use an appropriate authenticated transport
or established-session MAC/replay scheme; do not turn this parser into a public
unauthenticated reconnect amplifier. Epoch negotiation, key distribution,
encryption, jitter, congestion/rate control across peers and observed IP/port
reporting belong to a real transport adapter. No sender-supplied address/command
is executed by this module. STUN wire compatibility is not claimed.

A host polls from its event loop, sends only within its quotas, reports successful
I/O and keeps command/event delivery independent of heartbeat loss. Failed sends
must not call activity. Recovery must reject stale epochs and explicitly revoke
old debugger/resource sessions; heartbeat timeout cannot undo committed effects.
The tutorial's 30-second UDP/300-second TCP periods are illustrative configuration,
not measured NAT guarantees or battery recommendations. No real UDP/TCP listener,
broadcast/multicast transport or Internet traversal is added in this revision.

## Build and verification

```sh
cmake -S flashVM -B build-heartbeat -DFVM_BUILD_EXAMPLES=ON
cmake --build build-heartbeat --target flashvm_rheartbeat_contract flashvm_srecord_oracle flashvm_examples_embedding_hybrid_heartbeat_main
ctest --test-dir build-heartbeat -R '^flashvm_rheartbeat_' --output-on-failure
```

The contract checks slower periodic TCP, early requested TCP, independent receive
evidence, coalescing/throttle, deny/replay/epoch handling, atomic errors, sequence
exhaustion, clock wrap and saturating ages. An independent Python oracle checks
hint bytes at integer boundaries, CRC and malformed/prefix cases. These are
policy/codec tests without sleeps or network acceptance. Cross compile checks
must not be described as execution on the foreign target.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_RH_MAX_INTERVAL` | macro / define | [include/flashvm/rheartbeat.h:18](../include/flashvm/rheartbeat.h#L18) |
| `FVM_RHeartbeat` | struct | [include/flashvm/rheartbeat.h:61](../include/flashvm/rheartbeat.h#L61) |
| `fvm_rheartbeat_accept` | function | [include/flashvm/rheartbeat.h:104](../include/flashvm/rheartbeat.h#L104) |
| `fvm_rheartbeat_activity` | function | [include/flashvm/rheartbeat.h:96](../include/flashvm/rheartbeat.h#L96) |
| `fvm_rheartbeat_clear` | function | [include/flashvm/rheartbeat.h:111](../include/flashvm/rheartbeat.h#L111) |
| `fvm_rheartbeat_due` | function | [include/flashvm/rheartbeat.h:89](../include/flashvm/rheartbeat.h#L89) |
| `fvm_rheartbeat_hint_read` | function | [include/flashvm/rheartbeat.h:131](../include/flashvm/rheartbeat.h#L131) |
| `fvm_rheartbeat_hint_write` | function | [include/flashvm/rheartbeat.h:124](../include/flashvm/rheartbeat.h#L124) |
| `fvm_rheartbeat_init` | function | [include/flashvm/rheartbeat.h:82](../include/flashvm/rheartbeat.h#L82) |
| `fvm_rheartbeat_next` | function | [include/flashvm/rheartbeat.h:118](../include/flashvm/rheartbeat.h#L118) |
| `FVM_RHeartbeatHint` | struct | [include/flashvm/rheartbeat.h:49](../include/flashvm/rheartbeat.h#L49) |
| `FVM_RHeartbeatPolicy` | struct | [include/flashvm/rheartbeat.h:28](../include/flashvm/rheartbeat.h#L28) |
<!-- source-links:end -->
