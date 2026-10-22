# Queued identities for stream/event composition

Current contract: one root dictionary numeric sound-ID namespace per tracked epoch.
Different SWF/child dictionaries are not automatically disambiguated. All native
owners and callback access require external serialization. Keep mixer/backend/queue
alive until producer quiescence; all starts/stops use the tracked APIs. Configure
clock_begin, then tracking, then admission. On dictionary replacement coordinate
pause/acknowledged clear/source selection explicitly; no automatic event replay.

Invalid/null/busy inputs give INVALID_ARGUMENT; disabled tracked APIs UNSUPPORTED;
byte budget/allocation errors OUT_OF_MEMORY; exact time overflow UNSUPPORTED.
Backend statuses propagate and exceptions become ERROR. Ordinary failures preserve
native ownership/deadlines; a valid observation may advance played time. No API here
executes a VM, collects GC, opens a device or supplies DAC/A-V/child scheduling.
Successful opt-in adapted.clear stops all mixer voices and resets deadlines/clock/
pending after backend acknowledgement. Failed clear preserves them. The untracked
clear policy is unchanged. The queue itself owns and releases the optional table.

| API | Contract |
| --- | --- |
| [fvm_swf_sound_mix_queue_set_identity_tracking](../include/flashvm/swf_sound_mix_queue.h#L109) | Enable a budgeted ID table in a zero shared epoch; disable only at observed drain with no voices/pending/unexpired registrations. Repeated selection is a no-op. No backend/VM/GC, one native table allocation on enable; failures preserve mode. |
| [fvm_swf_sound_mix_queue_submit_prepared_bounded](../include/flashvm/swf_sound_mix_queue.h#L123) | Required owned prepared handle; success consumes it, errors preserve progress/ownership. Queued NoMultiple consumes an unused start even with zero mixer budget. Ordinary starts extend the maximum end after accepted+pending. No allocation/VM/GC; FIFO poll may refresh played. |
| [fvm_swf_sound_mix_queue_identity_info](../include/flashvm/swf_sound_mix_queue.h#L141) | ID 1..65535 and exact-size required output; copy sound_id/played/end/until_end/active after a valid real FIFO poll. Invalid/disabled/backend failures preserve output. Expiry retains historical end; no allocation/VM/GC. |
| [fvm_swf_sound_mix_queue_sound_active](../include/flashvm/swf_sound_mix_queue.h#L146) | Required scalar output, identical identity_info status/ownership rules. True only while registered time is undequeued; no reservation or DAC audibility claim. |
| [fvm_swf_sound_mix_queue_stop_id](../include/flashvm/swf_sound_mix_queue.h#L154) | Valid ID and enabled tracking required; use immediate/DECLICK mixer stop, cancel deadline, preserve accepted/pending bytes. No backend callback, allocation, VM or GC. |
| [fvm_swf_sound_mix_queue_storage_info](../include/flashvm/swf_sound_mix_queue.h#L57) | Exact fixed+buffer=retained requests; buffer includes both S16 arrays and optional deadline table. Allocator/mixer/voice/backend/VM charges are separate. |
| [fvm_swf_sound_mix_queue_storage_required](../include/flashvm/swf_sound_mix_queue.h#L44) | Predict initial untracked adapter requests only; the optional table is an additional ID_SLOTS*sizeof(double) charge. Preserve scalar out on errors, no allocation/callback/VM. |


v0.951 prepares guarded queue observation for tracked admissions: the internal observe_locked helper operates only inside a public reentry scope. Ordinary adapted.queued retains its existing validation, output preservation and exception handling. Existing clock, tail and callback-control regressions are rerun. No queued identity API is enabled by this refactor alone. Native owners stay externally serialized and hold no managed/GC reference. Full desktop integration, devices/platforms and all roadmap stages remain open.

v0.952: set_identity_tracking adds an optional fixed ID_SLOTS deadline table. Enable requires active zero shared clock, no pending PCM or mixer voices; exact adapter budget includes owner, both S16 arrays and all deadline doubles. Insufficient budget fails before allocation. storage_info.buffer_bytes now includes optional deadline arrays, preserving fixed+buffer=retained; storage_required still predicts initial untracked creation. Repeated mode selection is a no-op. Disable requires observed drain, no voices/pending and expired registrations. begin/end reject while enabled. Opt-in acknowledged adapted.clear stops all tracked voices and resets deadlines/clock/pending; failed clear changes none. Default untracked behavior and existing public structures stay unchanged. No callback/VM/GC occurs during mode configuration.

v0.953: submit_prepared_bounded observes the shared FIFO under the same reentry guard, places new conversion-phase-zero voices after accepted plus pending frames, and registers the largest exclusive ID end. NoMultiple consumes an unused prepared voice while its registered interval remains undequeued, including pending output and already retired mixer voices. After drain it may restart. Success consumes the handle; failures preserve ownership/progress/old voices/deadlines while a valid poll may refresh played. Duration overflow is UNSUPPORTED; mixer_budget bounds mixer/voices only, not adapter/table. Disabled tracking is UNSUPPORTED. Native tests retain templates after source VM teardown and compare exact PCM before/after drain.

v0.954: identity_info returns an exact-size copied ID/dequeue/end/until-end/active snapshot; sound_active shares that validation path. IDs 1..65535, live serialized owner and exact output size are required. Disabled is UNSUPPORTED; invalid/reentry and backend failures preserve outputs; no allocation/VM/GC. Historical expired deadlines remain visible. Overlap tests prove a new voice starts after pending PCM, extending end from 5 to 7 without remixing the pending block; zero-budget adoption preserves handle/progress/deadline. An independent seven-sample PCM oracle, partial drain, full expiry and invalid/unregistered IDs are checked.

v0.955: stop_id cancels future same-ID composition and its deadline through the mixer immediate/DECLICK policy, without modifying accepted/pending PCM. The pending old block stays exact; a later NoMultiple start composes after it. Failed opt-in clear preserves pending, voices and deadline; acknowledged clear stops all tracked voices and resets deadlines/clock/pending. Safe disable and drained clock_end then succeed. ID stop does not retract already queued audibility; full queue cancellation is explicit.

v0.956: callback controls attempt recursive admission/query/stop/mode changes using a real prepared probe; every attempt preserves its handle and progress. Failed observation, write/queued/clear exceptions and zero mixer budget preserve old registrations and ownership. Ordinary tests cover exceptions; --no-throw is available for sanitizer selection. Instrumented allocation checks prove insufficient adapter budget makes zero new attempts, failed table allocation preserves mode, and one table allocation enables tracking. With allocation forbidden, adoption, NoMultiple suppression, query, exact tail retries, drain, stop, clear, disable and clock_end make zero new attempts.

v0.957: one actual embedded SWF supplies a converted root stream and two independently prepared event IDs after source VM teardown. Tracked admission registers exclusive ends at output frames 9 and 14; ID8 remains active after stream EOF and through rejected tail output. Exact stereo PCM is unchanged, shared output ends at 15 with padding, expired registrations retain their historical deadlines, and drained disable/end succeeds. Gating, pending retry and callback exceptions remain independently exercised; this is native composition acceptance, not production desktop or device acceptance.

v0.958: four actual OS workers each run 64 independent tracked epochs using distinct source PCM and native templates after source VM teardown. Every epoch rejects/retries output, suppresses a NoMultiple duplicate, retains ID activity after mixer retirement, verifies exact PCM, drains and acknowledges clear to reset ID/clock state. Tables stay local and reusable; no VM/mixer/queue/voice/template handle is shared. This does not make mutable shared handles thread-safe.



v0.960: actual root MovieClip callbacks prepare/adopt ordinary/NoMultiple events and route stop-ID through tracked queue APIs. Interpreter and native JIT execute frame actions with entry counters asserted; stopped ticks and GC safe points preserve delivery. A retired queued ID stays active, stop cancels its lease without deleting FIFO PCM, and the next root start follows accepted frames. Exact stereo PCM survives source VM teardown. Explicit resume precedes simulated FIFO drain, then disable/end succeeds. Both modes ran in the JIT profile. This is controlled root callback acceptance, not full desktop/seek/child scheduling.

v0.961: a real C89 fixture links every new API and adopts/queries/stops/clears native voices after source VM teardown with copied error-output preservation. A white-box counter seed checks exact MAX_CLOCK admission and pending-start overflow without pretending to play that duration; rejection preserves incoming progress, NoMultiple suppression still consumes unused starts, and clear permits a fresh epoch. Finished voices register no interval; partially read voices retain their current cursor. DECLICK cancellation permits a new NoMultiple start while the old releasing voice remains, with exact first overlap PCM. Ordinary tests and source declarations document these distinct policies.


## v0.962 validation

Queued composition identities v0.951–v0.962: budgeted optional per-ID dequeue deadlines keep SyncNoMultiple active through pending and accepted FIFO audio after mixer retirement. Prepared admission, observation, stop and acknowledged clear preserve retry ownership and cancellation rules. Tracking covers one root dictionary namespace; child dictionaries are not remapped. Validation: 125 native regression/tutorial checks, 34 selected ASan/UBSan checks plus seven explicit --no-throw runs, and an interpreter/native-JIT root callback test with GC and native entries asserted. Ordinary builds cover exceptions and allocation failures; LeakSanitizer is disabled. A renamed detached component configures; its copied PCM checker passes against the native executable, without a separate detached build. Production desktop stream/event integration, child timelines, automatic A/V/seek policy, real devices/platforms and all nine roadmap stages remain open. [Details](MIX_QUEUE_IDENTITIES.md).


v0.963: Add immediate tracked event stop_all; retain pending/FIFO PCM and the shared clock.


v0.964: Expose checked storage_required_for_tracking without owner allocation or callbacks.


v0.965: Verify exact retained base/tail retry and new NoMultiple placement beyond pending PCM.


v0.966: Reject stop_all reentry while allowing independent storage prediction inside callbacks.


v0.967: Verify immediate global stop retires DECLICK tails and restarts NoMultiple after the retry.


v0.968: Cancel both converted root IDs and restart their independent deadlines after accepted PCM.


v0.969: Instrument global new/new[] with zero allowance across prediction, restart and stop_all.


v0.970: Run 256 independent stop/retry/restart epochs with per-worker exact PCM and ID checks.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_identity_info` | function | [include/flashvm/swf_sound_mix_queue.h:141](../include/flashvm/swf_sound_mix_queue.h#L141) |
| `fvm_swf_sound_mix_queue_set_identity_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:109](../include/flashvm/swf_sound_mix_queue.h#L109) |
| `fvm_swf_sound_mix_queue_sound_active` | function | [include/flashvm/swf_sound_mix_queue.h:146](../include/flashvm/swf_sound_mix_queue.h#L146) |
| `fvm_swf_sound_mix_queue_stop_id` | function | [include/flashvm/swf_sound_mix_queue.h:154](../include/flashvm/swf_sound_mix_queue.h#L154) |
| `fvm_swf_sound_mix_queue_storage_info` | function | [include/flashvm/swf_sound_mix_queue.h:57](../include/flashvm/swf_sound_mix_queue.h#L57) |
| `fvm_swf_sound_mix_queue_storage_required` | function | [include/flashvm/swf_sound_mix_queue.h:44](../include/flashvm/swf_sound_mix_queue.h#L44) |
| `fvm_swf_sound_mix_queue_submit_prepared_bounded` | function | [include/flashvm/swf_sound_mix_queue.h:123](../include/flashvm/swf_sound_mix_queue.h#L123) |
<!-- source-links:end -->
