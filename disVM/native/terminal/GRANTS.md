# Captured channel grants / Корни выданных каналов

The native Styx terminal adapter opens endpoints inside the descriptor's actual
grant. Previously it validated a provider but walked endpoint names from the
client's attach root. A narrowed root could therefore select unrelated ambient
nodes with identical names. Typed-announced resource fetches had the same issue.
Both paths now retain and use the captured grant root.

RU: Каналы и объявленные ресурсы открываются относительно выданного корня.
Совпадение имени вне него не даёт доступа к этому имени. Обычный Styx-сервер
по-прежнему решает, разрешено ли конкретное открытие или операция.

## Native adapter boundary / Граница native-адаптера

[`styxmountgrantfid`](../vfs/mount_grant.h) borrows a pinned provider/client and
returns an owned unopened root fid, or NULL. Before accessing native state it
requires one of this SDK's exact Styx mount factories, a bindable fixed dot and
the exact client used to create the mount. The requested access must fit the
mount's immutable local ceiling. A directory additionally requires EXEC/search.

The terminal constructor requires READ and WRITE because it opens both directions.
The local ceiling does not grant remote access: the server's ordinary `Twalk`
and `Topen` remain authoritative, including permission changes before open.
No new permission query or wire extension is introduced.

| Grant | Terminal endpoint paths | Announced resource path |
| --- | --- | --- |
| Directory | Canonical relative RX/TX; separate endpoints allowed | `movie/<generation>` below that same root |
| Single file | Both paths must be `.`; independent read/write fids | Descendants refused; no sibling or attach-root fallback |
| Different client's Styx grant | Refused before endpoint opening | No route created |
| Arbitrary generic provider | Refused by this native Styx adapter | No route created |

Generic providers retain their existing generic
[`dvm_terminal_channel_open`](../launch/terminal.h) path. This native adapter
does not reinterpret their opaque state or replace them with a supplied client's
root. Supporting another backend requires an explicit adapter/export.

## Root and ownership / Корень и владение

The constructor clones the captured root fid, or the immutable attached-file
root. It does not rewalk a captured directory's old pathname. Renaming that
directory and creating a different node at its old name therefore does not
redirect the grant. Names below the retained root remain live; no tree snapshot
is created.

The descriptor/provider is borrowed through construction. On success, the control
owns its root, RX/TX fids and client reference. The original descriptor/provider
can then retire. The host must keep the underlying physical/bidir owner alive
and retire external users before freeing the control/route.

[`styxbidircontrolwalk`](control.h) and
[`styxbidirroutewalk`](route/route.h) return owned unopened fids relative to the
retained root. Absolute names and parent traversal are rejected. They use the
current native cancellation scope, borrow their owner through return and never
expose its private root fid. The caller owns subsequent open/use/clunk and must
finish external walkers before owner disposal.

The typed-announced loader uses routewalk under its per-fetch scope, then passes
the owned fid into the common movie fetch implementation. Existing request/reply
framing, generation checks, private origin and final cancellation fences remain.
The legacy raw movie-session API still uses its explicitly supplied client's
attach root; it has no separate route grant.

RU: После успешного setup root и рабочие fids принадлежат control. Provider не
нужен для последующих обращений. При переименовании сохраняется захваченный
объект, но содержимое его namespace остаётся живым. Остановка не закрывает
общий client. Внешние вызовы завершаются до free владельца.

## Cancellation and verification / Отмена и проверка

Root cloning, endpoint walks/opens and consuming cleanup reuse the native VFS
scope-to-Styx cancellation bridge. Pre-cancelled setup is refused. Failure or
cancellation rolls back owned fids and any workers already started; no control
with weakened authority checks is published. No guest polling or guest suspension
is required. The existing S-record format and numeric tags are unchanged.

The [grant fixtures](tests/grant/cases.cmake) run 20 scenarios in both ordinary
and cancellation modes over original native Styx servers and fragmented bidir
PTY traffic. They cover colliding ambient names, nested/file roots, local ceilings,
foreign real clients, generic-provider refusal, retained child roots, pathname
replacement, separate RX/TX, server permission changes, cancellation and actual
scoped resource acquisition. Independent ordinary client operations remain usable.

Negative controls substitute the previous constructor or announced-fetch body
into the current build. The former fails scoped identity and foreign-client
refusal; the latter fails acquisition from the scoped resource service. These
checks exercise the original mistakes rather than only the new helper's output.
