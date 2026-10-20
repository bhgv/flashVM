# Bounded SDL event processing

[Русский](PLAYER_EVENT_FAIRNESS.ru.md). Since v0.658 each call to
fvm_desktop_window_events handles at most 64 SDL records. On reaching that limit
it returns immediately without waiting, leaving the remaining queue intact.
The player can then deliver input, poll timers, take a safe GC step and advance
or draw the movie. Window/exposure/user event storms cannot trap the host in one
unbounded SDL drain. A close behind a large backlog is delayed by bounded batches,
never discarded. Calls that drain fewer records retain the requested wait policy.

This budget is separate from the 128-entry copied input FIFO. It does not promise
bounded total SDL queue memory, event coalescing or lossless hostile producers.
FIFO overflow still fails explicitly. All SDL and VM operations remain on the
owner thread; this adds no VM worker or concurrent callback execution.

flashvm_frontend_event_budget uses an actual dummy SDL queue, verifies remaining
records after each batch and proves that queued keyboard/quit records survive.
The deliberately long wait argument must not block while a full batch was used.
This is a scheduling/queue contract check, not OS/device endurance acceptance.

## Touch window identity (v0.659)

SDL 2.0.12+ identifies a touch event's destination window. The same owned-window
filter now covers down/motion/up finger records, including a foreign release
with matching touch/finger IDs: it cannot terminate our contact. Zero IDs remain
explicit synthetic/unassigned input; older SDKs retain their device/finger-only
routing. Primary-contact selection and focus/background cancellation are unchanged.
The lifecycle fixture checks foreign start/release suppression and owned release.
