# Union traversal and module-instance VFS / Обход юнионов и VFS экземпляра модуля

The native VFS now guards active provider searches, including ordinary local
unions. A repeated branch is reported as `DVM_VFS_CYCLE`; a union can continue
with another branch. History belongs to one operation's native owner and is
removed on return. There are no shared node flags, suspended guests, global
visited sets, recursive tree snapshots or per-machine environment caches.

Защита относится ко всем provider-backed обходам, а не только к удалённым
вызовам. Повтор активного состояния останавливает циклическую ветвь. Остальные
ветви проверяются дальше. Если подходящая ветвь не найдена, доказанный цикл
не превращается в «файла нет». Отказ в доступе сам по себе не является циклом.

## Identity and active history / Сущность и активная история

[`walk.h`](../host/traversal/walk.h) defines the shared native implementation.
Each active entry compares all of these fields:

| Field | Meaning |
| --- | --- |
| `entity.domain[4]` | Four unsigned 32-bit limbs identifying a VM instance or a host-configured logical domain |
| `entity.object[4]` | Four unsigned 32-bit limbs identifying this exact provider projection/export endpoint/held object |
| `node_high`, `node_low` | Unsigned 64-bit node identity where known, including held remote Qids |
| `operation` | Named metadata, open, access, capture, launch discovery, directory listing or held metadata |
| `remaining` | Canonical root-relative remaining search spelling |
| `detail` | Access/open mode relevant to that operation |

Bind occurrence ID, stack position, nickname, permissions, timestamps and load
are not identity. Moving a layer must not conceal a cycle. Changing permissions
must not create a supposedly new entity; the actual operation still decides
access. Separate grants/projections remain distinct even when they use one
machine or spell the same path.

Opened native handles get separate identities: replacing a name does not make
old and new held objects identical. Remote held metadata/listing additionally
uses the acknowledged Qid path. Named lookups use the fixed exported endpoint
and path; this describes a search, not a snapshot of a mutable file tree.

The owner keeps eight entries inline and grows to a finite 128-entry limit.
A depth limit or allocation failure is an error, not proof of a cycle. Matching
push/pop scopes are required; completed branches can revisit an entity, and
parallel native threads have independent histories. Byte transfers over an
already opened non-directory resource do not become namespace traversals.

[`provider.h`](vfs/provider.h) also exposes `dvm_provider_walk_identity` for a
trusted adapter to assign a shared semantic identity before a factory is
published. It requires exclusive ownership and no previous callback. This is
comparison metadata, never authentication or an access grant.

The hosted VM configures its descriptive domain at startup using native entropy,
before publishing providers. An embedding SDK user calls `walk_domain` before
constructing remotely traversed providers. A zero domain supports local searches
but cannot be exported; the codec refuses it rather than transmit process-local
pointer identities. No native pointer or C structure layout crosses the wire.

## Explicit Styx transport profile / Явный профиль транспорта Styx

The existing raw Styx constructors and original Inferno codecs are unchanged.
For history-aware links, both endpoints explicitly select:

- `styxclientwalkmountphy(phy, msize, user, name, timeout, slots, tag)`;
- `styxphywalkinit(server, ops, phy, permissions, needfile, slots, root)`.

The profile works over an already granted reliable ordered `RemotePhy` stream;
it does not require TCP or UDP and creates no listener/authentication policy.
It can be layered on an already admitted bidirectional session. It does not
turn an ordinary single-session connection into a bidirectional one.

Each bounded transport envelope contains a little-endian total length and two
ordinary S-records. Tag0 describes the direction-local Styx/history profile,
selected nonzero data tag and copied history; a reply acknowledges the tag and
reports the fixed virtual export endpoint. The nonzero-tag record contains the
complete unchanged original 9P message. This profile fixes its tag for the
connection; it does not implement live Unibus client-tag reassignment, a second
Unibus broker or access to a client's transport-level tags.

This envelope is an explicit extension, not a new standard 9P field, opcode or
Twalk tree transfer. Raw peers do not receive it automatically. Malformed,
unsupported or changed endpoint assignments fail the profiled connection;
there is no silent fallback to interpreting them as raw 9P.

History is captured on the actual native RPC caller before broker queue
publication. The native server imports owned scalar copies before original
libstyx dispatch. Its bounded fid metadata follows acknowledged attach/walk,
partial walk, open, clunk and version reset; it does not replace authoritative
libstyx fids. Before enqueuing a remote lookup, a native VFS mount probes the
acknowledged endpoint/path against active history. A return to the same blocked
server can therefore be refused before another request is queued onto it.

`styxphycycleerror(server)` lets the current synchronous server callback mark an
**actual cycle result**. The original reply remains Rerror; the profile's control
record adds a typed cycle flag. An unrelated skipped cycle must not relabel a
later permission denial. The client propagates that annotation to the original
RPC caller, preserving union continuation without parsing arbitrary error text.

S-record CRC behavior remains exactly that of the shared FlashVM codec, including
short checksums for short records. It is error detection, not peer authentication;
this adapter neither changes its checksum algorithm nor guarantees rejection of
every possible mutation. Scalar-history validation, quotas and ordinary provider
permissions remain separate checks.

Raw-only remote peers cannot carry this active history. General asynchronous
nested RPC continuations are also not implemented here: the server profile
covers its synchronous namespace callbacks and original deferred byte-read/
flush lifecycle. Different-state recursion into an already occupied serial
server still needs an independently serviced route or an explicit RPC-cycle
handling policy; matching search-state detection is not arbitrary deadlock
resolution.

## Provider-side module environment / Окружение модуля у провайдера

`DVM_LAUNCH_OWNER_FIRST` is a new explicit preparation policy:

1. The client grants a filtered live VFS root, represented remotely by Styx.
2. The provider prepares one fresh namespace for this module instance: its own
   permitted root above that caller's granted root.
3. Ordinary absence falls through to the lower live provider. No stack scan,
   machine deduplication or client/provider role reversal is performed.
4. The prepared environment is attached to the actual local Modlink before
   publication using `dvm_module_link_prepared` or `dvm_module_environment_bind`.
5. Calls reuse it. The nearest bound module in the owner's live call frames
   supplies the VFS to Sys/Vfsctl operations, builtins and ordinary dependencies.
6. A spawned owner inherits the effective namespace. `FORKNS`/`NEWNS` use that
   current environment; per-process detached variants do not alter another
   module instance's prepared namespace.
7. Final Modlink retirement, including Fast/cycle-collector paths, removes its
   weak registry key and queues preallocated native cleanup on the closer.

Spawn retains the currently selected module binding as well as the base group.
This matters when the child starts in the same bound module: its first frame
must select the parent's effective group, including earlier `FORKNS` edits,
rather than return to the module's original prepared group. Parent and child
share that group until either explicitly forks its own namespace. Only the
active binding is inherited; unrelated module override records are not copied.
Construction stages this native reference before publishing the child. Allocation
failure or a later launch-context retain failure releases all staged ownership.
The constructor inspects only its own active parent's frames, never another
running owner's register set.

При `spawn` сохраняется выбранное окружение текущего модуля, включая изменения
после `FORKNS`. Родитель и ребёнок разделяют одну группу, пока один из них явно
не отделит свою. Это действует и для вложенного `spawn`. Ошибка создания потока
не публикует ребёнка и освобождает уже удержанные ссылки. `/prog` не изменён.

Loads in this new policy follow the actual configured stack, including explicit
later reorders. The module import base remains the normal import base. Legacy
explicit `DVM_LAUNCH_TERMINAL_FIRST` environments keep their established policy;
there is no silent change to older callers.

The registry is keyed by the exact local module instance, not by path, machine
ID or signature, and owns no managed reference back to the module. Namespace
reference counts only track ordinary native owners. Explicit pctl variants and
spawned children can independently retain their own namespaces after the
original instance ends. Existing opened FDs remain pinned to their resources.
No code resets FD0/1/2 or reconstructs them from namespace names: shell pipes,
redirection, current stream offsets and explicit terminal capabilities survive.
A FlashVM display context is optional for console modules.

Preparation may perform native provider I/O before managed admission. Attachment
and typed linking perform no remote I/O or foreign guest execution. Original
Inferno interface checks and linker entry frames/PCs are preserved.

## Proven scope and remaining integration / Проверенное и дальнейшая интеграция

Acceptance includes actual local and two-server A→B→A loops, distinct mount
aliases of one endpoint, healthy lower-layer continuation, explicit denial
preservation, replacement held objects, parallel history isolation, copied remote
history, bounded depth, allocation-refusal cleanup, fragmented PHY traffic,
acknowledged heartbeat, deferred reads, cancellation and disconnect.

Real native Dis applications run in both interpreter and JIT, checking dependency
lookup, module-bound builtin I/O, FORKNS, two independent module instances and
redirected stdin sharing. These tests attach prepared environments through the
public native API to actual original-linker results; they do not mock execution.
The module fixture now covers 82 positive interpreter/JIT executions and four
negative controls which omit the inherited binding or misdirect event-scope replacement. Additional
cases verify post-spawn shared edits, independent parent/child `FORKNS`, a
grandchild, real allocation refusal and rollback after a later context-retain
failure. Each accepted child is explicitly joined; the omission controls catch
the exact namespace error and still require the native ownership drain.

The native loader/cache fixture uses the agreed caller-relative rule: prime a
provider image through an explicit alias, then load the same virtual name from
the terminal owner's own layer and check its transitive dependency. Removing
that layer selects the provider image and its dependency without changing the
already loaded terminal instance. This is local native-source lookup coverage,
not evidence of remote execution or generic `.m` marshalling.

**This is not yet a transparent cross-machine Limbo module-call implementation.**
The ordinary terminal-side `load` still does not automatically create a remote
Modlink proxy or marshal arbitrary `.m` arguments/results/channels/FDs. A trusted
owner must already select the actual provider runtime and load its own local
implementation there. The new per-instance environment is the execution-side
building block for that next integration; it does not download provider code to
the terminal or relocate dependencies/devices. Native event-device selection and
pctl now use independent per-process/binding inboxes; already opened FD scopes
are preserved. First scope selection initializes observers, rather than a hook
on every mcall/return; see [system event scopes](SYSTEM-EVENTS.md).

Полностью прозрачный межмашинный `load`/вызов через `.m` остаётся отдельной
интеграцией. Здесь реализованы окружение конкретного исполняемого экземпляра,
его время жизни и общий обход юнионов, включая передачу истории по явному
профилю Styx. Проверки VFS выполняются на живых ресурсах; снимок дерева не строится.

## Validation checkpoint / Контрольная проверка

Linux amd64, 2026-11-19: all ten new `vfs-walk-*` cases pass in normal and
ASan/UBSan builds. The complete instrumented SDK run passes **750/750**;
16 affected runtime checks pass in both builds, including 58 actual interpreter/
JIT provider-module scenarios. Platform selection and deterministic hosted source
regeneration also pass. Public traversal/client/server declarations link from a
C++98 consumer. Changed handwritten source files remain at most 500 lines.

Parallel normal SDK runs exposed intermittent existing transport-fixture failures:
`vfs-mount-deep` reported a partial relay header in one run, and grant checkpoints
for variants7/11 occasionally timed out. The deep and variant7 cases each passed
ten consecutive isolated reruns; this does not establish that parallel transport
acceptance is free of timing failures. No retry was added to the tests and no
failing checkpoint was weakened. These observations remain a separate stability
follow-up rather than being described as a clean parallel normal-suite result.
