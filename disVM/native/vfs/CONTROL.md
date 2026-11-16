# Live mount event control / Управление подпиской работающего mount

Runtime0.152 adds native authority to replace or detach the configured event feed
of an existing Styx mount. The provider, namespace bindings, ordinary open fids
and their byte cursors keep their identity. No guest cooperation, scheduler poll,
guest pause or global stop is involved. This controls event subscription only;
it does not reconnect the transport or replace the mounted filesystem.

## Native API / Native API

Include `mount.h` (which includes `control.h`). The host receives a separate
`StyxMountCtl` capability from:

```c
/* Native host already owns connected client; NULL means no initial feed. */
StyxMountCtl *control = NULL;
DVM_Provider *provider = styxmountprovidercontrol(client, access, NULL, &control);
```

The constructor consumes client on every outcome. `control` output is required;
failure clears it, returns NULL and leaves optional initial decoder state with
the caller. Success returns an ordinary provider and an independently owned
control capability. Optional initial configuration follows [FEED.md](FEED.md).
Legacy mount constructors remain available; their providers own an internal
control but do not expose new authority through ordinary file access.

| Operation | Result and ownership |
|---|---|
| `styxmountctlinfo(control, &info)` | Copied version, closed, changing, configured;1/0 |
| `styxmountctlfeed(control, &version)` | Owned current `StyxFeed`/NULL, atomic with copied version |
| `styxmountctlset(control, expected, &config, &feed)` | New version/0; optional owned feed, cleared on failure |
| `styxmountctlclear(control, expected)` | New version/0; detach current feed |
| `styxmountctlretain(control)` | Retain borrowed capability;1/0 |
| `styxmountctlfree(control)` | Consume one owner; mount retains its independent owner |

Every caller must pin a control owner across each call, including native setup.
Version starts at1. Successful set/clear increment it; stale versions, exhausted
version space and closed controls refuse without wrap. Clearing an already empty
control still advances the version, so it can invalidate an admitted preparation.
`configured` means a current feed exists, including an EOF/FAILED feed; use that
feed's copied status to distinguish its operational state. Old feed capabilities
keep their identity and never silently refer to the replacement.

```c
/* Native control thread: cfg names an explicit relative endpoint and decoder. */
StyxMountInfo before;
StyxFeed *feed = NULL;
if (styxmountctlinfo(control, &before))
{
    unsigned long long revision = styxmountctlset(control, before.version, &cfg, &feed);
    if (revision)
    {
        /* cfg.state now belongs to feed; ordinary handles still use their own fids. */
        styxmountctlclear(control, revision);
    }
    /* On failure cfg.state remains caller-owned, even while aborted fid cleanup retires. */
}
styxfeedfree(feed);
styxmountctlfree(control);
```

These are native host APIs. Set performs walk/stat/open I/O with the existing
bounded RPC policy and must run on an appropriate native I/O/control thread.
The mount READ ceiling remains mandatory, and actual server Topen decides access.
A native control capability grants no extra filesystem visibility or permissions.
No guest builtin or automatic endpoint/codec discovery is added.

## Prepare and commit / Подготовка и переключение

Only one preparation per control is admitted. Another set refuses while
`changing` is true; there is no unbounded preparation queue. The admitted call
holds its own client/journal pins and releases the control mutex before I/O.
It prepares a separate fid and parked native worker without invoking/adopting
the new decoder state. Existing subscription remains active during this setup.

Commit checks the expected revision again. If it still matches, the control
pins the predecessor, requests its cancellation, installs the new feed, adopts
its context and advances the revision under its own mutex. Commit does not wait
for Rflush, old decoder return, clunk or worker retirement. Decoder and journal
callbacks never execute under that mutex. Clear or provider retirement can win
while setup is in progress; that prepared call then returns0, adopts no context
and retires its parked worker/fid independently. `changing` clears when the
admitted preparation returns, including after a concurrent clear/close.

The new native worker waits for predecessor cleanup before issuing its first
long-lived Tread. This preserves watch quota even with only two RPC slots and
a delayed Rflush. Rapid replacements follow the effective older outstanding
watch, rather than assuming that a cancelled standby's quick retirement frees
that watch. Clear preserves one owned retirement fence for a later set. That
bounded status/context reference is released on the next successful set or
provider retirement; external old status owners can retain it longer. A finished
feed releases its client/fid/journal even while status/context owners remain.

Ordinary RPC admission remains bounded and may refuse under concurrent load.
An accepted replacement means configured native worker state, not proof of
successful first-read admission. Inspect `styxfeedinfo` for subsequent failure.
No hidden retry queue, uncertain replay or automatic reconnect is introduced.

## Invalidation and lifetime / Уведомления и lifetime

A successful replacement posts a provider-wide invalidation for old/new class
masks plus CONNECTION. Clear posts all classes provider-wide. These hints cover
the subscription gap and ask observers to refresh; they do not grant rights or
assert TCP failure. Remote source cursors remain separate from the mount journal's
monotonic local generations. Existing health observation remains independent.

Old decoding that has not admitted publication at cancellation cannot add its
record afterward. An already admitted journal publication/callback may finish
after cutover, and callbacks can arrive out of order; existing generation fences
and conservative refresh still apply. There is no exactly-once stream promise.
Servers should supply private/independent subscription queues through prepared
views: replacing a fid does not duplicate records in a shared server inbox.

Final provider/ordinary-handle retirement permanently closes the control,
requests feed stop and releases its connection/journal pins without joining.
An extra control owner then provides closed status only; set/clear cannot
resurrect the mount. Pending preparation and detached workers retain their own
native resources through cleanup, never a provider pointer or guest Prog/REG.
Decoder `release` remains exactly once after worker and all feed owners retire;
it must be nonwaiting and cannot reenter feed/control operations.

## Acceptance / Проверки

The native suite has118 tests. Eight live-control TCP cases use the original
C libstyx server/codecs and a two-slot native client. They cover successful and
rapid replacement, held Rflush, clear/reinstall after standby retirement,
clear/close racing held preparation, rejection of concurrent preparation/stale
CAS, empty initial mount activation, READ-ceiling refusal and in-flight old
decoding. A new event is decoded by the latest worker after handoff. An ordinary
file opened before replacement remains readable through its original fid.
Context adoption/release, permanently closed status and asynchronous cleanup are
checked. Test-only native barriers do not require guest cooperation.

Native tests also run under ASan/UBSan (`detect_leaks=0`). The test drivers read
the final PASS line explicitly before `communicate`, avoiding loss of text already
prefetched into Python's stream buffer during fast process completion.

Подписка меняется у существующего mount через отдельную native capability и CAS
по версии. Новый endpoint готовится до переключения; при отказе state остаётся
у вызывающего. Clear/закрытие могут отменить commit уже начатой подготовки.
Управляющий вызов не ждёт старый read или decoder: только новый native worker
ожидает освобождения старого watch. Открытые ordinary fids сохраняются.
Формат событий и endpoint выбирает host; права окончательно решает операция.
