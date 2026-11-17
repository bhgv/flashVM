# Lookup absence without weakening union access / Отсутствие имени в union

Ordinary 9P2000 carries error **text**, not a portable numeric ENOENT. A failed
first element returns Rerror; a partial Rwalk reports a successful prefix without
the reason for the next failure. Permission, disappearance, wrong node kind and
connection failure must not all become missing-path fallback. See the original
[walk specification](https://9p.io/magic/man2html/5/walk) and imported
[libstyx implementation](../src/tools/libstyx/styxserver.c).

## Explicit compatibility profile / Явный профиль совместимости

The default `STYX_ERRORS_OPAQUE` never interprets remote text as absence. The host
can explicitly select `STYX_ERRORS_INFERNO` on an attached client, **before the
first walk fid or extra client reference is allocated**:

```c
if (!styxclienterrorprofile(client, STYX_ERRORS_INFERNO)) {
    /* Refused: invalid profile/client, shared/used client, or failed connection.
     * Caller still owns client; do not continue assuming the profile changed. */
}
```

Select it only when the granted endpoint uses Inferno's error vocabulary. It
matches the exact original libstyx `Enonexist` string, `file does not exist`, for
**Twalk only**. There is no substring matching, case folding, translation guessing,
Unix errno guessing, wire credential or permission grant. Unknown text, permission
denial, empty errors and lookalike prefixes remain failures. No 9P messages change.
Ordinary/bidir transport selection is independent of this semantic profile.

The native setter is serialized by the client lock. Unknown values, failed clients,
extra references and any allocated walk fid refuse without changing the profile.
Once a mount/provider/captured fid uses the client, later profile changes refuse.
This prevents another user of the shared connection from silently changing the
fallback interpretation of already published providers.

Профиль включается host до публикации клиента или mount. Он означает выбор
известного словаря ошибок сервера, не проверку прав. Сами операции и сервер по-прежнему
решают доступ. По умолчанию неизвестный сервер не получает интерпретацию ошибок
«по похожим словам».

## Per-call result / Результат каждого вызова

`styxclientlookup(client, path, cancel, &missing)` and
`styxclientlookupfrom(base, path, cancel, &missing)` return an owned fid or NULL.
Inputs are borrowed until return. The base must be pinned/unopened; serialize its
open/clunk. The required missing output is initialized to zero before validation.

| Outcome | Returned fid | missing |
| --- | --- | --- |
| Complete acknowledged walk | Owned | 0 |
| Exact acknowledged Inferno Enonexist, explicit profile | NULL | 1 |
| Opaque profile, permission/unknown/lookalike error | NULL | 0 |
| Partial Rwalk, invalid path/type, local refusal | NULL | 0 |
| Cancellation, missing acknowledgement, timeout or malformed reply | NULL | 0 |

Under the Inferno profile, this lookup sends **one path component per Twalk**.
Successful components retain the new fid; the next component yields its own exact
Rerror if it fails. This costs more RPCs on deep paths but avoids losing the
failure reason in a partial multi-element Rwalk. Zero-element cloning remains
ordinary Twalk; a failed clone never proves a missing name. Rooted lookups never
restart from the original attach pathname.
The existing walk APIs retain their MAXWELEM batching and legacy result contract.

The missing result is copied while the matching response is borrowed under the
client lock, before cleanup can overwrite reply storage. There is no shared
last-error query. Concurrent lookups and reverse reply ordering cannot overwrite
another caller's result. Failed candidate fid cleanup retains the existing native
retirement rules. No data operation, failed open or uncertain side effect is replayed.

A native cancellation token covers the whole component sequence through its
existing child tokens. Each RPC keeps the existing request/flush budgets. A
pre-cancelled lookup emits no Twalk. If an original exact Rerror arrives during
flush and wins under the existing acknowledgement rules, it remains authoritative;
a bare Rflush without that response never proves absence. A known absence result
is not revoked merely because the transport fails later during cleanup.

## VFS and launch / VFS и подготовка запуска

The ordinary mount's metadata/open path uses this per-call lookup outcome.
Only failed **lookup with missing1** maps to FVM_NOT_FOUND. A successful walk
followed by failed Tstat stays FVM_ERROR, even if its error text is Enonexist.
Topen failures, cancellation, malformed responses, permission errors and transport
loss remain failures. The file-root mount's locally known descendant exclusion
still reports NOT_FOUND without contacting a server.

Thus the existing absence-only namespace policy can select a lower provider after
acknowledged name absence. It stops on refusal/failure. This applies to ordinary
sessions and explicitly configured bidir channels equally; it does not add an
implicit host filesystem fallback, change provider order, bypass rwx or alter the
separate MAUTORWX policy.

For [`styxlaunchprepare`](../launch/README.md), select the Inferno profile on its
terminal client before retaining it or calling preparation. The function itself
never guesses a peer vocabulary. Captured roots and namespace copies share the
already selected immutable profile and retain their ordinary independent stack order.

## Verification / Проверки

An independent raw 9P peer on PTY tests default opacity, exact/profile mismatch,
permission denial, unknown/empty/lookalike error strings, deep one-component
walks, captured roots, partial Rwalk, wrong tags, physical loss, pre-cancel,
timeout/Rflush, original error winning a flush race and concurrent reversed
responses. It uses hand-built packets rather than the production codec.

Actual original libstyx PTY servers also exercise launch unions through both
ordinary and bidir sessions: an absent name reaches the local lower provider;
a directory lacking search permission and a known file with failing Tstat do not.
The local provider records that neither denied path was queried. The same cases
without opt-in remain conservative. Event delivery and actual Tflush continue on
the healthy sessions after those lookups.

All 318 Linux64 native SDK tests pass normally and under ASan/UBSan, with
LeakSanitizer disabled. Imported Inferno/libstyx source and the wire format are
unchanged. Native client header compilation uses its existing C99/64-bit types.
