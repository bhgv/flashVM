# Реестр выполнения / Compatibility status

Ориентир: [ROADMAP.md](ROADMAP.md). Базовый реестр уточняется по мере сравнения
с нормативными SWF/AS API; это ещё не полный каталог совместимости.
Статусы: отсутствует / реализовано / проверено / завершено. «Проверено» относится
к перечисленным сценариям, а не ко всем возможным входам или платформам.

| Пункт | Статус | Подтверждение и оставшаяся работа |
| --- | --- | --- |
| 8/9: Windows x64 offline encoder | Проверено для v0.560 на Linux GCC ms_abi | Separate Microsoft ABI, shadow space/alignment, volatile arguments, native branches/status and >1 MiB forward span. [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). Windows selection/memory/unwind registration and full native OS acceptance remain open. |
| 3/4/7: root streaming PCM | Проверено целевыми тестами v0.558 | Root header segments, frame-addressed blocks and allocation-free native PCM range extraction, strict C89 ABI and FWS/CWS inspection. [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md). Compressed stream decoding, progressive loading, clock/sprite/MovieClip integration and native platform acceptance remain open. |
| 2/3/4/7/8: native ToggleQuality | Проверено целевыми тестами v0.557 | Direct allocation-free stage helper; host/SWF1–8 interpreter/explicit/automatic native entries, four starting states, retained BEST/reset, context isolation, branches and budget ordering. C89 coverage and allocation-denied/lifecycle tests. [STAGE_QUALITY.md](STAGE_QUALITY.md). Sampling grids, bitmap downsampling, full Flash/foreign-platform acceptance remain open. |
| 2/4/7/8: native StopSounds | Проверено целевыми тестами v0.556 | Interpreter/explicit/automatic native host and SWF1–8 runs, actual native entries, ordered callback failures/retry, context isolation, callback GC/lifecycle barriers and OOM unwind. Actual swf_mix FILE cleanup executes the action. [JIT_SOUND.md](JIT_SOUND.md). Automatic sound timeline, other codecs, physical playback and foreign platforms remain open. |
| 3/4/7/9: root SWF sound resources | Initial implementation v0.548 | Five C89 APIs: count/ordinal/ID views, bounded 16-bit PCM conversion and explicit channel submission. SWF_SOUND_RESOURCES.md. Native ALSA null pipeline is separate from physical playback; StartSound/streaming/AS Sound and compressed/8-bit decoding remain open. |
| 4/5/7/9: Linux ALSA PCM output | Initial implementation v0.547 | Explicit static SDK, native nonblocking write/start/drop/reprepare/close. ALSA_AUDIO_OUTPUT.md. Real null-PCM library tests are separate from physical playback and other OS acceptance. |
| 2/4/5/7/8: context audio and StopSounds | Initial implementation v0.546 | Copied C89 host output registrations, active/retry/error policy, teardown, AVM1 0x09 and transported global stopAllSounds. AUDIO_CHANNELS.md. Full SWF sound decoding/scheduling/mixing and native sound-card acceptance remain open. |
| 2/3/4/7/8: Stage quality / ToggleQuality | Проверено целевыми тестами v0.538 | SWF1–8, independent contexts, operand preservation, interpreter/JIT fallback, LOW/HIGH/BEST transitions; solid/stroke/gradient/bitmap edge coverage on AGG/Cairo. STAGE_QUALITY.md. v0.541 adds named _quality/_highquality and intrinsic indices 16/19: SWF4–8 coercion, GC/error paths and graph transport. Sampling grids and bitmap downsampling remain open. |
| 4/5/7/9: native speech bridges | Initial implementation v0.540 | Real TTS/STT provider source and explicit C89 sessions; platform preparation examples. Linux protocol/ABI/parser and Android source compile evidence are separate from unverified audible/physical microphone and foreign native runs. SPEECH_BRIDGES.md. |
| 4/5/7/9: multimedia/device bridges | Implemented initial v0.539 layer | Real static FFmpeg decoding/capture interfaces, replaceable host source/output/device callbacks and native input functions. Linux WAV/PPM/evdev fixtures checked; physical capture/playback and foreign native runs remain open. MULTIMEDIA_BRIDGES.md. |
| 1/2/7: resolver result OOM publication | Проверено для v0.452 | Fresh-context SWF4–8 native/script/method resolver allocation sweeps, named NUL/indexed reads, cyclic exceptions, safe undefined output, cleanup/GC/retry. OBJECT_RESOLVE.md / .ru.md; C/OS allocators, scope/transport and stages open. |
| 1/2/7/8: resolved method invocation | Проверено для v0.451 | C/AVM1 ordinary/super Function dispatch, missing/nonfunction distinction, original rooted args, collecting deletion/detachment and no replay of failing callbacks in JIT. OBJECT_RESOLVE.md / .ru.md; scope/transport/OOM and stages open. |
| 1/2/7/8: loaded Object.__resolve reads | Проверено для v0.450 | Raw own/inherited resolver rules, names/index/super reads, GC/errors, special exclusion, native recursion and actual JIT GetMember. OBJECT_RESOLVE.md / .ru.md; C/super calls: v0.451; scope/transport/OOM and stages open. |
| 2/7: active native C-call capture | Проверено для v0.449 | Trusted own/inherited/global self-removal/replacement, cyclic capture/args, NUL, three status paths, GC retention/reclamation. PROPERTY_CASE.md / .ru.md; ordinary native barriers retained, other entries/finalizers/concurrency and stages open. |
| 2/7: C call pre-getter arguments | Проверено для v0.448 | Host/SWF4–8 traced NUL/cyclic args, getter mutation/GC, aliased output, getter/callee exceptions, cleanup/reclamation. PROPERTY_CASE.md / .ru.md; native captures/OOM/external buffers and stages open. |
| 1/2/7: C method call case/owner roots | Проверено для v0.447 | Host/SWF4–8 counted names, own/inherited Function/getter/native/shadowing, collecting deletion/detachment and errors. PROPERTY_CASE.md / .ru.md; call inputs/resolver and stages open. |
| 1/2/7/8: sortOn raw field casing | Проверено для v0.446 | SWF4–6 data/virtual/length/method selection, SWF7–8 exact, inherited exclusion, NUL/Unicode, unique/indexed returns, cache identity/GC and real native CallMethod. PROPERTY_CASE.md / .ru.md; full sorting/collisions/budgets and stages open. |
| 1/2/6/7/8: super prototype case aliases | Проверено для v0.445 | SWF5–6 intrinsic ancestor projection and folded user overrides; SWF7–8 exact/NUL, queue/GC topology, receiver/counts and actual native GetMember. PROPERTY_CASE.md / .ru.md; full raw proto/reflection and stages open. |
| 2/7/8: inline invocation roots/OOM | Проверено для v0.444 | Six live identities, all-new-denied entry/nested unwind, reclamation and fresh-context SWF5–8 conversion allocation sweeps with alias/error/GC/retry. PROPERTY_CASE.md / .ru.md; C/OS allocators, concurrent GC and stages open. |
| 1/2/7/8: super special conversion identity | Проверено для v0.443 | Original receiver/defining owner, folded names, noncallables, collecting deletion/detachment, errors and native ToNumber/ToString/Add2; PROPERTY_CASE.md / .ru.md. C++ allocation boundaries: v0.444; general resolution/C/OS allocation and stages remain open. |
| 1/2/7/8: super string fallback | Проверено для v0.442 | SWF5–8 non-string fallback, NUL/exception/callback counts, Function controls, GC and real native ToString; PROPERTY_CASE.md / .ru.md. Receiver/owner: v0.443; full conversions remain open. |
| 1/2/7/8: loaded super TypeOf | Проверено для v0.441 | object result for super, actual Function/instance controls, host, SWF5–8, in-place helper, GC and real native TypeOf. Direct parent invocation retained. PROPERTY_CASE.md / .ru.md; other reflection/conversion and stages remain open. |
| 1/2/3/7/8: registerClass super validation | Проверено для v0.440 | Reject before name coercion, preserve prior constructor; host, SWF5–8, NUL, throwing conversion, retry/unregister, GC and actual native CallMethod. PROPERTY_CASE.md / .ru.md; full constructors and stages remain open. |
| 1/2/7/8: borrowed Array super raw writes | Проверено для v0.439 | Seven methods run successively without facade/receiver mutation; SWF5–8, GC and actual native CallMethod entries. PROPERTY_CASE.md / .ru.md; complete super/Array and stages remain open. |
| 1/2/7/8: borrowed Array raw length casing | Проверено для v0.438 | slice/push/splice resolve own LENGTH storage and flags in SWF4–6 without callbacks; SWF7–8 exact. Virtual/readonly/NUL and actual native CallMethod matrix. PROPERTY_CASE.md / .ru.md; full Array and stages remain open. |
| 1/2/7/8: setter replacement spelling | Проверено для v0.437 | SWF4–6 reselect recreated data/virtual property and readonly flags after collecting setter; host/SWF7–8 exact. Counted strings and real native SetMember entries. PROPERTY_CASE.md / .ru.md; native replacements and stages remain open. |
| 1/2/7/8: watcher throw completion | Проверено для v0.432–434 | Deferred thrown graph survives collecting setters; own/inherited/readonly/numeric/Array order, getter-only, JIT entries and OOM recovery. WATCH_THROW.md / .ru.md. Other errors/resources/collisions remain open. |
| 1/2/6/7/8: super callback identity | Проверено для v0.435–436 | Borrowed definition/watch no-op with coercion validation; no parent invocation from virtual get/set; host/direct calls retained, GC/queue/native paths. PROPERTY_CASE.md / .ru.md. Full super remains open. |
| 1/2/7/8: watcher and definition names | Проверено для v0.427–431 | Requested watcher spelling, raw old values/Array length, replacement/unwatch, addProperty and rooted C definitions, NUL/GC/OOM and native SetMember. PROPERTY_CASE.md / .ru.md. Exceptions/collisions/budgets remain open. |
| 1/6/7: native method cache roots | Проверено для v0.430 | Four owner mutations release obsolete wrappers/captures after successful publication; pinned Functions survive, identity and OOM rollback checked. METHOD_CACHE_GC.md / .ru.md. Other caches/finalizers/concurrency remain open. |
| 1/2/7/8: property mutation/setter casing | Проверено для v0.424–425 | Selected own flags/delete/update, inherited setter stored identity and collecting failures, OOM sweeps and actual native SetMember entries; PROPERTY_CASE.md / .ru.md. Watch/definition/collision/budget rules remain open. |
| 1/2/7: versioned property queries/reads | Проверено для v0.423 | SWF4–6 Flash folding, exact host/SWF7–8, counted Unicode/NUL/surrogate names, own/flags/chain reads, stable method identity and stored own writes; PROPERTY_CASE.md / .ru.md. Mutation/definitions/collisions/budgets remain open. |
| 1/2/7/8: shared decimal numeric coercion | Проверено для v0.416 | Number/ToNumber/Subtract and parseFloat share pinned decimal accumulation; strict SWF5–8 vs relaxed SWF4, negative zero/wrapped exponents and allocation-free primitive conversion. Actual native entry tests. GLOBAL_PARSING.md / .ru.md; full binary64/overflow and general coercion budget remain open. |
| 1/2/7/8: numeric leading whitespace | Проверено для v0.415 | Flash-only TAB/LF/CR/space for loaded Number/ToNumber/parseFloat, host policy retained; 30-unit SWF4–8 matrix, traced strings and real native paths. GLOBAL_PARSING.md / .ru.md. Other numeric syntax/rounding and full player conformance open. |
| 1/2/7/8: loaded global numeric parsing | Проверено для v0.412–v0.414 | SWF4–8 radix-before-text parseInt, raw prefixes, counted NUL, decimal accumulation and exponent wrap; exact rounding fixture, work quotas, OOM/GC cleanup and real native CallFunction. GLOBAL_PARSING.md / .ru.md. Full whitespace/rounding/player oracle remains open. |
| 9: automatic project version refresh | Проверено для v0.408 | Production CMake version module registers VERSION.txt as a configure dependency. Isolated version-only edit/build/rebuild fixture checks metadata refresh without changing checkout version. BUILD_VERSION.md / .ru.md; foreign execution remains open. |
| 6/7/9: counted request identities | Проверено для v0.407 | Raw TCP NUL/nonhex source/call IDs and zero source context reject before peer creation/function entry. Distinct process/context ownership remains independent; valid requests recover. IDs are metadata, not authentication. broker_request_identity. |
| 6/7/9: authority startup C boundary | Проверено для v0.406 | Member constructor failures are contained as OUT_OF_MEMORY with null output; explicit errors retain status and initialized handles unwind. Allocation sweep, occupied listener and post-stop port reuse. broker_start_allocation. |
| 6/7: persistent-peer stop under OOM | Проверено для v0.405 | Inline nonce/context keys avoid allocations while clearing mutex peer records. All-new-denied stop releases two acquired remote locks while preserving original-function/host pins until source detachment. broker_peer_stop_allocation. |
| 6/7: individual function revocation/OOM cleanup | Проверено для v0.403 | Explicit revoke drops one authority root, stale interfaces reject, re-export changes target, host/other-owner pins survive. Inline fixed tokens avoid allocation in revoke/detach/entry removal at stop; mixed channel/function all-new-denied check. REMOTE_FUNCTIONS.md / .ru.md. |
| 6/7/9: VM-thread and LAN examples | Проверено локально для v0.402 | Public Worker owns a second runtime/context; TCP interface reaches the original VM thread. Configurable bind/advertised host/port for LAN, IPv4/DNS/wildcard local process checks and strict port rejection. Two physical computers remain untested. FUNCTION_RPC_EXAMPLES.md / .ru.md. |
| 6/7/9: process-separated C function interface | Проверено для сценариев v0.401 | C89 original callback stays in authority process with original userdata; independent client checks value/NUL exception/plain failures. Native Python harness has bounded waits/cleanup; foreign OS runtime validation remains open. REMOTE_FUNCTIONS.md / .ru.md. |
| 6/7: nested original function calls | Проверено для сценариев v0.400 | Same-authority function interfaces dispatch on the owning thread with copied graphs, original heap leases and depth bound 32. A→B→A, exception/diagnostic propagation and recovery; cross-authority cycles/resource reentrancy remain open. REMOTE_FUNCTIONS.md / .ru.md. |
| 6/7/8: function export roots/prepared GC | Проверено для сценариев v0.399 | New-target URI allocation precedes publication; failed C++ export allocations leave no target/root, canonical reuse pins once. Original prepared body self-unbind/GC retains capture and cyclic argument frame. Graph-wide export rollback remains open. |
| 6/7: function reply/value graph | Проверено для сценариев v0.398 | Reject missing OK/EXCEPTION values and contradictory successful fault replies; cyclic receiver/argument aliases and returned C interfaces verified. No unrestricted shared heap or reentrant callback support. |
| 6/7/9: original function RPC | Проверено для сценариев v0.397 | Opt-in callable interfaces retain original native/AVM1 state; one broker port with target/source/call IDs, exception diagnostics, forwarding and revocation. Exclusive idle-context handoff; serialized calls, no same-authority reentrancy. REMOTE_FUNCTIONS.md / .ru.md. |
| 1/2/4/7/8: identical TextField assignment | Проверено для сценариев v0.396 | Complete counted equality preserves loaded current formatting/layout; changed NUL suffix uses future format. Host policy retained; alias/GC and native SetMember/GetMember checks. TEXTFIELD_COUNTED_TEXT.md / .ru.md. |
| 4/6/7: TextField restore limits | Проверено для сценариев v0.395 | CRC-valid oversized graph payloads reject with object/allocation/font rollback; valid 4096-scalar and 16384-byte/8192-unit boundaries round-trip. TEXTFIELD_COUNTED_TEXT.md / .ru.md. |
| 1/4/7: counted formatting strings | Проверено для сценариев v0.394 | Full keyword length, NUL font rejection, true-empty URL/target validation; current/future style and extent rejection preserve field state. Supported formatting subset unchanged. TEXTFIELD_COUNTED_TEXT.md / .ru.md. |
| 1/4/6/7: counted TextField text | Проверено для сценариев v0.393 | Counted NUL storage/layout/length/range edit, extent, OOM/retry, traced GC, binary and queue transport. Strict scalar text and existing bounds retained; full TextField open. TEXTFIELD_COUNTED_TEXT.md / .ru.md. |
| 1/2/3/6/7/8: symbol constructor version | Проверено для сценариев v0.392 | attach/duplicate/authored placement select source library environment under mixed versions; GC/transport/fallback and attach OOM/retry. Native registration provenance verified separately. Other API/global provenance open. OBJECT_REGISTER_CLASS_CASE.md / .ru.md. |
| 1/2/3/6/7: registration source version | Проверено для сценариев v0.391 | Retained library SWF version and active base-clip selection after name coercion; real SetTarget/CallMethod across SWF4–8 vs ambient6/8, GC/transfer and OOM. Old graphs fall back; other APIs open. OBJECT_REGISTER_CLASS_CASE.md / .ru.md. |
| 1/2/3/6/7: separate constructor environments | Проверено для сценариев v0.390 | Independent legacy/exact maps, reload, counted names, GC, transfer and atomic replacement failures. Old flat records retain prior policy. Full globals/base-clip provenance open. OBJECT_REGISTER_CLASS_CASE.md / .ru.md. |
| 1/2/7: String character budget | Проверено для сценариев v0.389 | string_character_work: SWF4–8, index callbacks, nested/saturated budgets, exceptions, native GC barrier and lone-surrogate extraction. Receiver decoding work remains open. STRING_LOADED_WORK.md / .ru.md. |
| 1/2/7: String casing budget | Проверено для сценариев v0.388 | string_case_work: SWF4–8/host, counted NUL, lone/paired surrogates, exact/nested/saturated budgets, unchanged internal folding policy. Intermediate decode work remains open. STRING_LOADED_WORK.md / .ru.md. |
| 1/2/7: String builder budget | Проверено для сценариев v0.387 | string_build_work/_callbacks: SWF4–8/host, UTF-16 pairs/NUL, empty arguments, nested/saturated budget, conversion order/exceptions and GC barrier. Decoding/full conformance open. STRING_LOADED_WORK.md / .ru.md. |
| 1/2/7: String range copy budget | Проверено для сценариев v0.386 | string_portion_work: SWF4–8/host, paired/lone surrogates, exact budgets, nested/saturated counter, no partial output. Decode work remains open. STRING_LOADED_WORK.md / .ru.md. |
| 2/3/4/7/8: indexed dimension writes | Проверено для сценариев v0.385 | SetProperty 8/9 coerces value and atomically publishes two scales from untransformed vector bounds using pinned rotation rules; degenerate/descendant/GC/OOM and native checks. Named/full geometry semantics open. MOVIECLIP_DIMENSION_WRITES.md / .ru.md. |
| 1/2/6/7/8: loaded Number nondecimal values | Проверено для сценариев v0.384 | All 34 nondecimal bases use pinned Int32 truncation/MIN-sentinel digit policy; boundaries/fractions/NaN, counted control bytes, captured radix coercion, OOM/GC/transfer and native equivalence. Historical player profiles open. NUMBER_LOADED_RADIX.md / .ru.md. |
| 1/2/7/8: budgeted interface declarations | Проверено для сценариев v0.383 | Loaded snapshot capture/inspection shares AVM1 work; atomic OOM/budget failure, validated allocation-free direct noops, full 65536 count and interpreter/native retry. Public count bound/host policy retained. IMPLEMENTS_WORK.md / .ru.md. |
| 1/2/7/8: loaded String traversal work | Проверено для сценариев v0.382 | Loaded split passes 65537 fields; split/search scans and split UTF-16 copies share nested AVM1 work, with OOM/GC/retry, conversion exceptions and interpreter/native equivalence. Host cap retained; decoding/full conformance open. STRING_LOADED_WORK.md / .ru.md. |
| 1/2/7/8: budgeted interface membership | Проверено для сценариев v0.381 | Loaded InstanceOf removes 4096 visited-node rejection; identity hash set, shared charged traversal, saturation/OOM/GC/retry and large interpreter/native graphs. Host/prototype bounds retained. INSTANCE_OF_GRAPH_WORK.md / .ru.md. |
| 1/3/6/7/8: registerClass legacy casing | Проверено для сценариев v0.380 | SWF4–6 UTF-16-unit folded registration/query/removal; stored spelling replacement, atomic OOM, GC/transfer, attach/duplicate/fallback and interpreter/native CallMethod. Separate global environments/mixed provenance remain open. OBJECT_REGISTER_CLASS_CASE.md / .ru.md. |
| 2/3/4/7/8: indexed MovieClip dimensions | Проверено для сценариев v0.379 | GetProperty 8/9 reads transformed vector/descendant bounds in parent space, including singular ancestors; snapshot/barrier callbacks, real index-coercion GC, OOM and native equivalence. Writes added in v0.385; named dimensions/full Flash extents open. MOVIECLIP_INDEXED_SIZE.md / .ru.md. |
| 1/2/3/7/8: Unicode legacy display paths | Проверено для сценариев v0.378 | SWF4–6 Flash unit folding for child/property paths; counted NUL/surrogates, collecting own/inherited getters, OOM and interpreter/native targets; MOVIECLIP_UNICODE_PATHS.md / .ru.md. Duplicate-name/full player semantics remain open. |
| 1/7: large loaded sorting | Проверено для сценариев v0.377 | Native/borrowed sort/sortOn above 65536, charged partial captures/reversal/index output, permutations/indices, budget and OOM/GC/retry; ARRAY_LOADED_LARGE_SORT.md / .ru.md. Host limits and full Flash semantics remain open. |
| 1/7: large loaded generic mutations | Проверено для сценариев v0.376 | shift/unshift/reverse/splice above 65536 use shared budgets, with native/borrowed success, prototype accessor bypass and OOM/GC/retry; ARRAY_LOADED_LARGE_MUTATION.md / .ru.md. Host/sort bounds and full conformance remain open. |
| 1/7: negative loaded slice | Проверено для сценариев v0.375 | SWF4–8 native/borrowed signed ranges, dense raw own copy, coercion mutations, source retention, budgets and allocation/GC/retry; ARRAY_NEGATIVE_SLICE.md / .ru.md. Signed overflow and result length above Int32 maximum remain open. |
| 1/7: borrowed negative end operations | Проверено для сценариев v0.374 | SWF4–8 push/pop/shift/unshift signed own data, zero crossing, flags, raw accessors, budgets and OOM/GC/retry; ARRAY_NEGATIVE_ENDS.md / .ru.md. Native negative storage and signed overflow remain open. |
| 1/7: borrowed negative splice | Проверено для сценариев v0.373 | SWF4–8 signed own keys, captured conversions, zero crossing, readonly flags, budget failure, allocation sweep and cleanup; ARRAY_NEGATIVE_SPLICE.md / .ru.md. Native Array negative storage and signed overflow remain open. |
| 3/6/7: MovieClip drag methods | Проверено для сценариев v0.330 | startDrag/stopDrag share C value preparation, versioned truth, partial/forward bounds, stable appended IDs/transport, collecting direct calls/native barriers and OOM; MOVIECLIP_DRAG.md. Full input/drop/twip semantics open. |
| 2/3/7: StartDrag/EndDrag | Проверено для сценариев v0.329 | Raw Actions use shared C services; exact flag-one policy, seven-step coercion order, detached-target GC, missing operands and OOM; AVM1_DRAG.md. Script adapters/full mouse behavior remain open. |
| 3/7: explicit native drag engine | Проверено для сценариев v0.328 | C89 begin/update/end/target, transforms/limits/input order, drag-only GC root, cancellation/replacement and atomic OOM; NATIVE_DRAG_SERVICE.md. Script/Action adapters, drop-target and full Flash semantics open. |
| 2/3/6/7: clip-local pointer coordinates | Проверено для сценариев v0.340 | C89 shared projection, _xmouse/_ymouse, GetProperty 20/21, readonly policy, callback IDs/transport and OOM. Loaded binary32/twip projection, unit device-pixel rounding and singular fallback; CLIP_POINTER.md. Stage view/device scaling, detached ancestry and full input/drag open. |
| 3/7/9: context pointer position | Проверено для сценариев v0.326 | C89 world-pointer set/get, independent contexts, queued vs immediate publication and callback error visibility; INPUT_POINTER.md. Relative clip coordinates and drag remain open. |
| 3/6/7: native construction decomposition | Проверено для сценариев v0.325 | Drawing copier and sprite class selector moved to individual files with token-identical bodies; shared private contracts, graphics/transport/GC/OOM coverage; MOVIECLIP_CONSTRUCTION_DECOMPOSITION.md. Remaining decomposition and full semantics open. |
| 3/7/9: input callback failure recovery | Проверено для сценариев v0.324 | C++ exceptions contained at C dispatch boundary; queued pump resets, remaining/enqueued events survive, processed counts successes; INPUT_CALLBACK_FAILURES.md. Full MovieClip/input lifecycle remains open. |
| 3/7/9: consistent host input validation | Проверено для сценариев v0.323 | Direct/queued input share gesture/finger/finite-coordinate/phase checks; C89 limits header, invalid-input side effects and FIFO capacity coverage; INPUT_VALIDATION.md. MovieClip input/drag integration and other OS execution remain open. |
| 2/3/6/7: CloneSprite | Проверено для сценариев v0.322 | Explicit C89 request, Int32 depth/name/source order, absolute bias, raw Action, missing operands, collecting callbacks/throws/OOM; AVM1_CLONE_SPRITE.md. Native range/display type/removed ancestry gaps remain. |
| 3/6/7: intrinsic C sibling construction | Проверено для сценариев v0.321 | Shared duplicateMovieClip service, direct C89 graphics/initialization/transport, host/SWF4–8 and OOM; DUPLICATE_CLIP_SERVICE.md. Full duplicate/version/depth semantics open. |
| 3/6/7: intrinsic C hit geometry | Проверено для сценариев v0.320 | Separate C89 world-point/object handlers; direct calls bypass method overrides, native roots/barriers and OOM; CLIP_HIT_GEOMETRY_SERVICE.md. Complete stroke/filter/detached semantics and JIT remain open. |
| 3/6/7: explicit clip resolver and hitTest targets | Проверено для сценариев v0.319 | C89 shared resolution, loaded single-target paths and zero-argument false; MOVIECLIP_HIT_TARGET.md. Full geometry/display compatibility open. |
| 3/7: root-relative numeric hitTest | Проверено для сценариев v0.318 | host/SWF4–8, unlocked root affine mapping, shape truth, moving/throwing coercion and OOM; MOVIECLIP_HIT_ROOT_COORDINATES.md. Full overload/detached/stroke/filter semantics open. |
| 3/6/7: native _lockroot | Проверено для сценариев v0.317 | C89 service, host/SWF4–8 truth, root/path bounds, removed ancestry, stable native callback transport and OOM; MOVIECLIP_LOCKROOT.md. Load/unload inheritance and hitTest lock coordinates open. |
| 2/3/7: removed target selection | Проверено для сценариев v0.316 | SetTarget/2 filter detached bases/descendants; live sibling paths, host/SWF4–8 and unchanged selection on OOM. AVM1_REMOVED_TARGET_SELECTION.md. Full unload/scope policy open. |
| 3/6/7: removed native AVM1 ancestry | Проверено для сценариев v0.315 | Retained parent/root, absolute paths, target fallback, GC, channel remapping and atomic OOM; MOVIECLIP_REMOVED_ANCESTRY.md. Unload/lockroot/full lifetime open. |
| 2/3/7: RemoveSprite native dispatch | Проверено для сценариев v0.314 | Shared explicit C89 target removal; host/SWF4–8, GC/throws/OOM, activation target fallback. AVM1_REMOVE_SPRITE.md. Retained native ancestry added v0.315; unload and complete display lifetime open. |
| 3/7: dynamic native removal policy | Проверено для сценариев v0.313 | C89 explicit-object service, intrinsic depth/ancestry, host/SWF4–8 and OOM. DYNAMIC_CLIP_REMOVAL_SERVICE.md. RemoveSprite dispatch/path/unload policy remains open. |
| 3/7: native clip detach identity | Проверено для сценариев v0.312 | C89 query; fresh roots vs removed/replaced wrappers, legacy/malformed state, GC and atomic OOM. CLIP_DETACH_IDENTITY.md. Descendants/unload/RemoveSprite remain open. |
| 2: missing constant-pool entries | Проверено для сценариев v0.311 | Push Constant8/16: host/SWF4–8, absent/one-entry pool, boundaries, truncation and OOM. AVM1_CONSTANT_MISSING.md. Полная строковая семантика открыта. |
| 2: empty StoreRegister | Проверено для сценариев v0.310 | host/SWF4–8, global/private Function2, ignored slots, malformed payload and OOM; AVM1_STORE_MISSING.md. Полная семантика функций открыта. |
| 2: missing call operands | Проверено для сценариев v0.309 | CallFunction/NewObject/CallMethod/NewMethod: host/SWF4–8, fixed layout, zero arguments, GC/throws/OOM. См. AVM1_CALL_MISSING.md. Полная семантика вызовов открыта. |
| 1: значения String с NUL и суррогатами | Проверено для базовых сценариев v0.16 | standard_utf16.c, standard_string.c, string_wire.c; ABI 3, граф 4. См. UTF16_STRINGS.md. |
| 1: upper/lower и CASEINSENSITIVE | Проверено по таблице эталона v0.180 | Loaded Flash UCS2 mappings; все 65536 units обоих направлений сверены с pinned Ruffle в SWF8, отдельные SWF4..8/GC/UNIQUESORT тесты. Host Unicode 3.2 сохранён. Property-name case и независимый Flash Player остаются открытыми. См. STRING_LEGACY_CASE.md. |
| 1: Object, Array и ключи | Реализовано частично | Watch/перечисление есть; generic Array, наследование индексов и registerClass проверены в сценариях v0.17 (GENERIC_ARRAYS.md). Counted property get/set/flags/delete, AVM1 member operations и graph 8 проверены в v0.81 (COUNTED_PROPERTY_KEYS.md). Counted calls/registration, пределы длины и полная сверка открыты. |
| 1: Date NaN month setters | Проверено для сценариев v0.208 | date_legacy_month: SWF4–8 local/UTC, missing/undefined/NaN month → January, infinite month and NaN day → invalid; unloaded host unchanged. См. DATE_LEGACY_MONTH.md. Остальные календарные quirks открыты. |
| 1: Date minute setters | Проверено для сценариев v0.209 | date_legacy_minutes: SWF4–8 signed bounds/fractions/NaN/infinity, all three components, retained fractional milliseconds and local/UTC setters. См. DATE_LEGACY_MINUTES.md; полная Date-совместимость открыта. |
| 1: Date retained time components | Проверено для сценариев v0.211 | date_legacy_retained: SWF4–8 all four UTC time setters at fractional hour/day boundaries; retained hours/milliseconds use getter projections. См. DATE_LEGACY_FIELDS.md. Calendar setters/full equivalence open. |
| 1: Date calendar time-within-day | Проверено для сценариев v0.212 | date_legacy_calendar: SWF4–7 signed remainder, SWF8 nonnegative remainder, fractional clipping, local/UTC year/month/day setters. См. DATE_LEGACY_CALENDAR.md. Invalid-date and other conversion edge cases open. |
| 1: Date component year adjustment | Проверено для сценариев v0.213 | date_legacy_year: SWF4–8 constructor/UTC negative years, year zero and 99/100 boundary; unloaded host unchanged. См. DATE_LEGACY_YEAR.md. Fractional components/full Date open. |
| 1: raw-примитивы __proto__ | Проверено для сценариев v0.227 | object_raw_prototype и allocation: SWF4–8, flags/GC/channel/relink/delete; наследование использует только Object. Membership/casing/accessor initialization и циклическая модель остаются открытыми; исходная object_prototypes ещё расходится. См. OBJECT_RAW_PROTOTYPE.md. |
| Коды, флаги и числовые границы | Рефакторинг проверен v0.230 | 7 C89 .h с документированными UPPERCASE макросами; AVM1/SWF/resource clients используют смысловые имена. AGG/Cairo Release 432/432 каждого; 10 ограниченных ASan/UBSan, оригинальные SWF 17/22 в обоих профилях. См. SEMANTIC_CONSTANTS.md. |
| 1/2: числовой текст и StrictEquals NaN | Проверено для сценариев v0.229 | AVM1 fifteen-digit spelling/carry/subnormals и loaded paired-NaN equality; number_text/allocation, 432 полный профиль и 5 ограниченных ASan/UBSan. Исходные divide_swf4/strictequals_swf6 проходят, корпус 17/22. См. AVM1_NUMBER_TEXT.md. |
| 2/7/8: бюджет и safe points общего C-слоя | Проверено для сценариев v0.228 | execution_checkpoint и worker: общий бюджет вложенных вызовов, overflow, GC roots, остановка, debug mappings; 430 полного профиля и 6 ограниченных ASan/UBSan. Восстановленный SWF<=8 корпус: 15/22 проходят, 7 открыты. См. EXECUTION_CHECKPOINT.md. |
| 2/8: общий слой обработчиков и нативное тело | Проверено для сценариев v0.226 | C89 execution.h, handlers/ и общий builder активаций; execution_services, script_native_body, script_native_function2, execution_services_allocation; 426 полного профиля и 10 ограниченных ASan/UBSan. JIT backend и полный внешний SWF корпус не проверены. См. AVM1_DECOMPOSITION.md. |
| 2: AVM1 | Реализовано частично | Регистры Function2, основные preloads и aliasing именованных arguments проверены в v0.20 (AVM1_FUNCTION2.md). Extends/ImplementsOp/CastOp и транзитивные интерфейсы проверены в v0.22 (AVM1_INHERITANCE.md). Основные вызовы super и перенос фасадов проверены в v0.23 (AVM1_SUPER.md); числовые super-свойства, регистровые aliases и полная сверка областей/версий ещё открыты. |
| 2: Function2 paired flags | Проверено для сценариев v0.214 | avm1_function2/super: preload+suppress accepted; this/super register undefined, arguments object retained; combined registers, GC and channel transfer. См. AVM1_FUNCTION2_FLAGS.md. Named inherited bindings/full version semantics open. |
| 2/7: arguments.caller | Проверено для сценариев v0.215 | avm1_arguments_caller: SWF4–8 first-class nested caller identity/null, nonenumerable flag, escaped graph GC/channel, exception and injected allocation failure restoration. См. AVM1_ARGUMENTS_CALLER.md. Native/legacy caller identity and full scope rules open. |
| 2/3: Function2 display preloads | Проверено для сценариев v0.216 | avm1_function2: intrinsic base-clip root/parent, script getter bypass, absent parent slot skipped; original Ruffle define_function2_preload_order SWF6 byte-exact (--timeline 1). См. AVM1_DISPLAY_PRELOADS.md. Definition clip provenance/register capacity edge cases open. |
| 2: loaded arguments Array/copies | Проверено для сценариев v0.217 | avm1_arguments_array: SWF4–8 builtin Array prototype/length flags, independent named parameters, raw element creation, owned NUL strings, GC/channel. Host mapped arguments retained. См. AVM1_ARGUMENTS_ARRAY.md. Full Function2 version/scope equivalence open. |
| 2/7: Function2 inherited arguments/super names | Проверено для сценариев v0.218 | avm1_function2_named_scope: SWF4–8 preload/suppress/paired names inherit closure, registers retain own values, formal names shadow, GC/channel. См. AVM1_FUNCTION2_NAMED_SCOPE.md. this-cell and complete super/scope behavior open. |
| 3: MovieClip и SWF-сцена | Реализовано частично | В v0.24 проверен импорт корневых DoAction/FrameLabel из FWS/CWS по кадрам и перенос через очередь/файл (SWF_MOVIECLIP_TIMELINE.md). В v0.25 проверены DefineSprite/ExportAssets и attachMovie с initObject/registerClass и переносом библиотеки (SWF_SPRITE_RESOURCES.md). В v0.26 проверено duplicateMovieClip с независимым рисунком, конструкторами и переносом (MOVIECLIP_DUPLICATE.md). В v0.27 проверены знаковые глубины и совместимость прежних снимков (MOVIECLIP_DEPTHS.md). В v0.28 проверены размещение/перемещение/удаление спрайтов, реконструкция кадров, матрицы и цвет, перенос и отказы памяти (SWF_SCENE_CONTROLS.md). В v0.29 подключены сплошные DefineShape/2/3: контуры, кривые, отверстия, замена геометрии, перенос и отказы памяти (SWF_SOLID_SHAPES.md). Полный display list и остальные ресурсы открыты. |
| 4: линейные/радиальные градиенты | Проверено для сценариев v0.30 | SWF Shape1/2/3 и C89 API: матрицы, цвет, перенос и отказы памяти (SWF_GRADIENTS.md). В v0.31 проверен beginGradientFill pad/RGB, копирование аргументов, AVM1, дублирование/перенос и OOM (MOVIECLIP_GRADIENT_FILL.md). Shape4, другие режимы, полная геометрия и остальная графика/мультимедиа открыты. |
| 4: flash.geom.Point/Matrix | Проверено для числовых сценариев v0.32 | standard_geometry.cpp и geometry_allocation.cpp: все публичные методы, частичные конструкторы, матрицы, очередь/файл, GC и отказы памяти. В v0.33 проверены строки/NUL и отдельные getter/setter overrides (geometry_dispatch.cpp). Полная семантика getter/Math overrides и остальные классы открыты (GEOMETRY_CLASSES.md). |
| 4: flash.geom.Rectangle | Проверено для сценариев v0.34 | standard_rectangle.cpp: методы, производные свойства, границы, AVM1 NewMethod, queue/file/loopback TCP и GC. rectangle_allocation.cpp: отказы фабрик. Полная семантика coercion/Math и удалённые процессы открыты (RECTANGLE.md). |
| 4: flash.geom.ColorTransform | Проверено для сценариев v0.35 | standard_color_transform.cpp: каналы/rgb/concat, копия, queue/file/TCP, GC и валидация состояния. color_transform_allocation.cpp: копия и десериализация с OOM. Применение через Transform и полная сверка открыты (COLOR_TRANSFORM.md). |
| 3–4: Transform и MovieClip.transform | Проверено для сценариев v0.36 | standard_transform.cpp: матрицы/цвет/границы, snapshots, GC, queue/file/TCP с общими ссылками. transform_allocation.cpp: атомарная запись цвета и OOM фабрик. Полные pixelBounds, удаление клипов и точная квантизация открыты (TRANSFORM.md). |
| 4–9 | Открыты | См. ROADMAP.md и прежние контрольные точки; завершение не подтверждено. |

No roadmap stage is closed. Update this inventory with concrete tests, profiles
and limitations when implementing a remaining item. This initial inventory is
not yet a complete SWF tag/action/AS API compatibility catalogue.

## v0.37 BitmapData subset / Частичная реализация BitmapData

Native pixel storage, size/rectangle accessors, clone/dispose, pixel access,
fillRect, scroll and colorTransform are implemented. Queue/file graph snapshots
retain factories and validate binary state. See [BITMAP_DATA.md](BITMAP_DATA.md)
for test names and remaining conformance/graphics limitations. Stage 4 remains
open; no full BitmapData, rendering or media compatibility is claimed.

Реализован перечисленный набор BitmapData; остаются premultiplied rounding,
остальные операции, интеграция сцены/заливок и полная приёмка.

## v0.38 BitmapData.copyChannel

Implemented selected-channel copying with joint source/destination clipping,
overlap snapshots, opaque-alpha preservation and allocation-before-mutation.
standard_bitmap and bitmap_allocation cover the operation.
Реализировано копирование отдельного канала с отсечением и независимым снимком;
полная совместимость BitmapData и этап 4 остаются открытыми.

## v0.39 Number power-of-two formatting / Форматирование Number

Exact finite fractions and large integers in bases 2/4/8/16/32 are implemented
and covered by standard_number, including extreme binary64 values and queue
transport. [NUMBER_RADIX.md](NUMBER_RADIX.md) records the remaining limits.
Остальные дробные основания и историческая сверка открыты; этап 1 не закрыт.

## v0.40 Numeric AVM1 actions

Increment/decrement, remainder, bitwise operations/shifts and Greater are
implemented; avm1_numeric_operations verifies boundaries, coercion order and
error recovery. See AVM1_NUMERIC_ACTIONS.md. Полная семантика AVM1 открыта.

## v0.41 Vector MovieClip shape hit testing

MovieClip.hitTest point/shape queries and the backend-independent C89 path query
are implemented with documented subdivision/boundary tolerances. path_hit and
movieclip_shape_hit cover fill rules, curves, strokes, transforms, queue/file and
GC. See MOVIECLIP_SHAPE_HIT.md. Маски/bitmap/text и полный этап 3 остаются открыты.

## v0.42 BitmapData.copyPixels

Implemented clipped pixel copies, alpha masks, mergeAlpha and opaque source-over
composition with source/mask snapshots before mutation. bitmap_copy_pixels and
bitmap_allocation cover transport, overlap and OOM. See BITMAP_DATA.md for
premultiplied/rounding limitations. Полный этап 4 остаётся открытым.

## v0.43 MovieClip bitmap fills / Bitmap-заливки

beginBitmapFill and the C89 bitmap painter implement copied affine matrices,
live texture edges, repeat/edge extension, nearest/bilinear sampling, inherited
color transforms and graph transfer. Duplication shares texture identity while
copying geometry. Native path RAII and marked AGG exception-safety corrections
handle injected allocation failures. See MOVIECLIP_BITMAP_FILL.md for named tests
and remaining resource/filter/text/media limits. Полный этап 4 остаётся открытым.

## v0.44 Hidden bitmap attachments

attachBitmap implements live hidden display nodes, depth replacement, render-only
auto/always/never snapping and smoothing. Child detachment is staged before
publication to preserve old nodes on OOM. movieclip_attach_bitmap and its
allocation suite cover pixels, visibility through depth APIs, queue/file, GC
and imported methods. See MOVIECLIP_ATTACH_BITMAP.md for remaining conformance
limits. SWF resources, complete graphics/text/media and stage 4 remain open.

## v0.45: lossless SWF bitmaps

Проверены DefineBitsLossless/2, форматы RGB555/XRGB/ARGB и палитры, bitmap-заливки Shape1/2/3, GC/перенос и отказы памяти. Ограничения и RU/EN описание: [SWF_LOSSLESS_BITMAPS.md](SWF_LOSSLESS_BITMAPS.md). Этап 4 открыт.

Lossless tags and bitmap shape fills have format, rendering, GC/transport and allocation-failure coverage. JPEG resources and the rest of stage 4 remain open.

## v0.46: JPEG2/3

JPEG2/3: проверяются RGB/grayscale, отдельный alpha, границы потоков, пиксели, GC/очередь и отказы C++ памяти. RU/EN ограничения: [SWF_JPEG_BITMAPS.md](SWF_JPEG_BITMAPS.md). Пункт 4 открыт.

JPEG2/3 has binary, pixel, GC/queue and C++ allocation-failure coverage; legacy tables, PNG/GIF and CMYK remain open.

## v0.47: embedded PNG/GIF

Проверены PNG palette/tRNS, gray/16-bit/Adam7 и full-canvas GIF89a с palettes/interlace, ошибки потоков, pixels/GC/очередь и failed C++ copies. [SWF_PNG_GIF_BITMAPS.md](SWF_PNG_GIF_BITMAPS.md) описывает RU/EN ограничения. Пункт 4 открыт.

SWF 8 PNG and single-frame GIF layouts have pixel/transport and malformed-input coverage. Partial GIF canvases and exact Flash color behavior remain open.

## v0.48: DefineBits/JPEGTables

Проверен bounded subset общего JPEG dictionary и placement SWF 1..8, framing/truncation, pixels/GC/очередь и отказы памяти. RU/EN: [SWF_JPEG_TABLES.md](SWF_JPEG_TABLES.md). Полная JPEG/Flash сверка и этап 4 открыты.

Shared table JPEG and SWF 1..8 placement have binary/pixel/GC/transport and allocation-failure coverage; full reference conformance remains open.

## v0.49: BitmapData.loadBitmap

Static loadBitmap: независимые копии exported bitmap, AVM1/current/global scope, NUL/missing linkage, GC/OOM и queue/file/TCP. [BITMAP_LOAD_RESOURCE.md](BITMAP_LOAD_RESOURCE.md) содержит RU/EN ограничения; loaded-library/closure scope и этап 4 открыты.

Static resource copies and transport are checked; full library scope and remaining graphics/media remain open.

## v0.50: BitmapData.draw

Проверенный subset draw BitmapData/MovieClip: transforms, color, clipping, nearest/bilinear, 12 blend modes, GC/queue и allocation rollback/balance. [BITMAP_DRAW.md](BITMAP_DRAW.md) описывает RU/EN ограничения; alpha/erase, masks/group effects и этап 4 открыты.

Transactional drawing has fixed-vector/scene and allocation-failure coverage; complete blend/Flash conformance remains open.

## v0.51: draw alpha/erase

Проверены alpha/erase с geometry coverage, AA/holes/strokes, outside preservation, self alias, single getter read и OOM rollback/balance. [BITMAP_DRAW.md](BITMAP_DRAW.md) описывает текущую модель всех 14 draw IDs и ограничения. Полная Flash/group/filter сверка и этап 4 открыты.

Alpha/erase coverage has explicit pixel and allocation-failure coverage; all draw mode IDs have an implemented model, with reference conformance still open.

## v0.52: floodFill

Scanline floodFill проверен на ARGB/4-connectivity, bounded scratch, coercion/disposal, GC/queue и later-stack OOM rollback/balance. [BITMAP_FLOOD_FILL.md](BITMAP_FLOOD_FILL.md) содержит RU/EN ограничения; полная Flash-сверка и этап 4 открыты.

Backend-independent connected fills have bounded-memory and failure-path coverage; full Flash conformance remains open.

## v0.53: getColorBoundsRect

Проверены ARGB masks/matching, boundaries/default/inverse/empty cases, captured Rectangle и queue/GC/OOM/balance. RU/EN: [BITMAP_COLOR_BOUNDS.md](BITMAP_COLOR_BOUNDS.md). Premultiplied/Flash сверка и этап 4 открыты.

Masked color bounds and transported result factories have explicit edge/failure coverage; stage 4 remains open.

## v0.54: threshold

Проверены unsigned masked comparisons, copy/defaults/opaque alpha, shared clipping/snapshot, self alias, GC/queue и OOM/balance. RU/EN: [BITMAP_THRESHOLD.md](BITMAP_THRESHOLD.md). Fractional/premultiplied/Flash сверка и этап 4 открыты.

Threshold and aligned region snapshots have explicit edge/transport/failure coverage; complete Flash conformance remains open.

## v0.55: merge

Per-channel merge: проверены weights/rounding/clamps, opaque alpha, overlap, budgets, GC/queue и OOM/balance. RU/EN: [BITMAP_MERGE.md](BITMAP_MERGE.md). Полная Flash/premultiplied сверка и этап 4 открыты.

Channel merging has fixed-vector/alias/transport/failure coverage; complete reference conformance remains open.

## v0.56: paletteMap

Проверены sparse palettes, identity channels, modulo-32 contributions, overlap, quota, disposal during getters, GC/queue and OOM/balance. RU/EN: [BITMAP_PALETTE_MAP.md](BITMAP_PALETTE_MAP.md). Полная Flash-сверка и пункт 4 открыты.

## v0.57: BitmapData.hitTest

Проверены три overloads, пороги alpha, границы и integer wrap, disposal/getters, queue/file, GC и injected OOM/balance. RU/EN: [BITMAP_HIT_TEST.md](BITMAP_HIT_TEST.md). Полная историческая Flash-сверка открыта.

## v0.58: BitmapFilter / ColorMatrixFilter

Проверено для color-matrix bitmap subset: inheritance, clone/matrix copies, aligned applyFilter, unchanged generateFilterRect bounds, malformed state, queue/file/TCP, GC и OOM/balance. RU/EN: [COLOR_MATRIX_FILTER.md](COLOR_MATRIX_FILTER.md). Другие фильтры, MovieClip.filters, SWF filter lists и full Flash conformance открыты.

## v0.59: ConvolutionFilter

Проверено для bitmap convolution subset: defaults/options, private matrix copies/clone, edge clamp/replacement, halo sampling, overlap, transport, invalid state и OOM/balance. RU/EN: [CONVOLUTION_FILTER.md](CONVOLUTION_FILTER.md). Flash rounding/premultiplied/coercion conformance, MovieClip filters и полный этап 4 открыты.

## v0.60: BitmapData.noise

Проверено для deterministic noise subset: signed/extreme seeds, Schrage arithmetic against independent double oracle, masks/grayscale, ranges, opaque alpha, quota/OOM, getter disposal, queue/file/TCP и GC. RU/EN: [BITMAP_NOISE.md](BITMAP_NOISE.md). Premultiplied alpha quantization и полная историческая Flash-сверка открыты.

## v0.61: opaque noise sampling correction

Opaque noise skips alpha RNG samples, matching the current Ruffle implementation. All 16 masks are checked in color/grayscale; transparent vectors and OOM/transport coverage remain. Историческая Flash-квантизация и полная приёмка открыты (BITMAP_NOISE.md).

## v0.62: BitmapData.perlinNoise

Реализовано для bounded procedural subset: SVG gradient turbulence/fractal sums, channel packing/grayscale, public offsets, stitch boundary, atomic buffers и callback 1550. Проверки: bitmap_perlin.cpp / bitmap_perlin_allocation.cpp. RU/EN: [BITMAP_PERLIN.md](BITMAP_PERLIN.md). Полная Flash/premultiplied/version/offset conformance и пункт 4 открыты.

## v0.63 — Local file channel debugging

Реализованы freeze чтения и indexed non-consuming peek локальных файлов; GC/циклы, блокирующие читатели и malformed input проверены в file_channel_debug. Remote inspection и file state snapshot отсутствуют. См. FILE_CHANNEL_DEBUG.md.

Local file freeze and indexed inspection are implemented; remote inspection and file state snapshots remain absent. Full debugger acceptance remains open.

## v0.64 — Bitmap dissolve model

Реализована детерминированная модель pixelDissolve: continuation, clipped copy/fill, atomic staging, native callback transport. Полная Flash-семантика порядка/seed/count/premultiplied alpha **не подтверждена**; метод не завершён. См. BITMAP_DISSOLVE.md.

Deterministic dissolve API implemented; exact historical Flash behavior is not certified and remains open.

## v0.65 — Local file state snapshots

Non-consuming local file pending/frozen/closed inspection implemented; malformed/read failures preserve output and cursor. Проверки в file_channel_debug; remote snapshot и полная приёмка стадии 7 открыты. См. FILE_CHANNEL_DEBUG.md.

## v0.66 — Managed fractional box blur

BlurFilter constructor/properties/clone, bitmap application and expanded bounds implemented with staged premultiplied planes and private graph validation. Полная Flash/scene совместимость остаётся открытой. Проверки: blur_filter, blur_filter_allocation; BLUR_FILTER.md.

## v0.67 — Remote channel debugger controls

Compatible broker queue/file freeze/state implemented with validated controls and bounded file scans. Проверки: channel_debug_network, channel_debug_protocol, file_channel_debug. Remote payload inspection, distributed wait/deadlock graphs and complete acceptance remain open; REMOTE_CHANNEL_DEBUG.md.

## v0.68 — Native string result lifetime

Context-owned snapshots retain native string results across userdata finalization, with counted-byte arena descriptors and atomic indexes. Проверки: gc_string_pool, gc_native_string_result, gc_native_string_allocation. Arena strings remain retained to context teardown; traced reclamation and full GC acceptance remain open (GC_STRING_STORAGE.md).

## v0.80 — Shared-proxy/local mutex cycle

Проверено / tested: locally observed mixed wait cycle between one retained TCP
proxy shared by two OS workers and one local mutex; owner/waiter locations,
endpoint, cancellation cleanup and reuse. Regression: `mutex_mixed_deadlock.c`.
Default AGG regression suite: 242/242 (examples disabled); mixed test 100 repeats;
ASan/UBSan targeted 5/5, leak scanning unavailable; portable excludes live TCP.
Separate proxy/process owners and full distributed deadlock detection remain
open. RU/EN scope and validation: [REMOTE_MUTEX_DEBUG.md](REMOTE_MUTEX_DEBUG.md).

## v0.81 — Counted property keys

Проверено / tested: counted UTF-8/WTF-8 names with embedded NUL and equivalent
UTF-16 scalar/pair representations, property attributes, lookup, enumeration,
watch and AVM1 member access; graph 8 queue/file/TCP with malformed-input and
allocation rollback checks. Default suite 244/244; portable 2/2; headless
ASan/UBSan 5/5 with stack/leak exclusions. Stage 1 remains open; counted calls,
native registrations and variable names are not covered yet.
RU/EN: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).

## v0.82 — Counted native member dispatch

Проверено / tested: native member/accessor registration and invocation with NUL
and canonical UTF-16 keys, retained captures, inherited receivers, cached Function
identity and AVM1 CallMethod/NewMethod path. Size-aware table property queries
added. Default 245/245; headless ASan/UBSan targeted 4/4; portable new test 1/1.
Global/function variable names and full stage-1 conformance remain open.
RU/EN and regression scope: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).

## v0.83 — Counted global and lexical names

Проверено / tested: global get/set/native registration/call, AVM1 local declaration,
variable access/assignment and function lookup with full canonical names. Missing
NUL-suffixed native constructors cannot invoke a terminated prefix. Legacy graph
keys normalize during restore; graph version remains 8 and public layout ABI 3.
AGG 246/246; Cairo Release 246/246; ASan/UBSan targeted 5/5 with stack/leak
exclusions; portable targeted 2/2. Stages 1–9 remain open. RU/EN behavior and
remaining scope: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).
Final acceptance gates: [RELEASE_ACCEPTANCE.md](RELEASE_ACCEPTANCE.md).

## v0.84 — Counted constructor and stack APIs

Проверено / tested: canonical counted native construction/registration,
AVM1 NewObject fallback, rooted stack/Lua-style registration and calls,
counted field/global/string auxiliary helpers. Regression:
native_counted_construction. AGG 247/247, Cairo Release 247/247;
headless ASan/UBSan targeted 4/4, portable new test 1/1. ABI 3 / graph 8 unchanged.
Remaining parameter aliases and full standard-type/version conformance keep
stage 1 open. RU/EN: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).

## v0.85 — Canonical formal and registered-symbol identity

Проверено / tested: rightmost named arguments alias for UTF-16-equivalent formal
names, counted NUL formals and named calls, canonical script/debug keys and
registered-class symbols. Regression function_parameter_identity reproduces the
former wrong alias and verifies GC/graph restore; class/native-variable tests
extend symbol/debug coverage. AGG/Cairo Release 248/248 each; restricted headless
ASan/UBSan focused 4/4; portable focused 2/2. Function2 register-only aliases and
remaining type/version conformance remain open. ABI 3 / graph 8 unchanged.
RU/EN: [FUNCTION_PARAMETER_KEYS.md](FUNCTION_PARAMETER_KEYS.md).

## v0.86 — Counted sort fields and selected slice ranges

Проверено: `sortOn` сохраняет полное имя поля, включая NUL, и нормализует
UTF-16 ключи. Заимствованный `slice` допускает источник длиной uint32 max,
если выбранный диапазон не превышает 65536; наследуемые индексы сохраняются.
Ограничение размера результата и остальных полных обходов остаётся.

Tested: complete canonical sortOn field keys and borrowed slice of a small
range from a uint32-length receiver, including inherited slots, negative and
empty ranges. The 65536 selected-range limit and other full-scan limits remain.
Regression: array_counted_ranges. AGG/Cairo Release 249/249 each; focused
headless ASan/UBSan 3/3 (stack/leak exclusions), portable 3/3. Stage 1 remains open.

## v0.87 — Legacy AVM1 coercion

Проверено / tested: Equals/Less numeric coercion and callback order, rooted
operands, conversion failures and general logical operands. avm1_numeric_operations
extended; AGG 249/249, Cairo Release 249/249 (six launch permissions restored and
rerun); focused headless ASan/UBSan and portable 4/4 each. Additional comparison
failure-order assertions passed on AGG. Historical string truth/version coercion
and full stage 2 remain open. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.88 — Loaded SWF string truth

Проверено / tested: Not/And/Or/If string truth switch for loaded SWF 5/6 vs
7/8, including counted NUL strings. Regression avm1_version_truth; AGG and
Cairo Release 250/250 each after launch-permission reruns (3 and 1 tests);
focused headless ASan/UBSan and portable 3/3 each. Stack/leak sanitizer
exclusions remain. Earlier numeric parsing/function-version rules are open.
RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.89 — AVM1 own-property deletion

Проверено / tested: ActionDelete counted keys, own/prototype/flag behavior,
accessor deletion without getter calls, Array indices/length and coercion errors.
Regression avm1_delete; AGG and Cairo Release 251/251 each (AGG launch-permission
failures and dependent process test rerun); focused restricted headless
ASan/UBSan and portable 3/3 each. Stage 2 remains open.
RU/EN: [AVM1_DELETE.md](AVM1_DELETE.md).

## v0.90 — AVM1 host trace delivery

Проверено / tested: ActionTrace and retained host sink, counted text, conversion
without delivery, callback replacement/errors and GC capture lifetime.
Regression avm1_trace; AGG/Cairo Release 252/252 each after launch-permission
reruns (5 and 1 tests), restricted headless ASan/UBSan and portable 2/2 each.
Historical formatting and reference corpus acceptance remain open.
RU/EN: [AVM1_TRACE.md](AVM1_TRACE.md). ABI 3 / graph 8 unchanged.

## v0.91 — Reproducible top-level SWF trace runner

Реализовано / implemented: opt-in C89 runner for exact trace-byte comparison
with separate failure status. Synthetic FWS/CWS, truncated input and usage
checks passed on Linux AGG. External SWF 6 reference probe fails legacy string
coercion/CallMethod; two selected Array corpus SWFs are version 15 and outside
scope. No new conformance gate is closed.
RU/EN: [SWF_REFERENCE_RUNNER.md](SWF_REFERENCE_RUNNER.md).

## v0.92 — Legacy undefined strings and nullish method calls

Проверено / tested: ordinary undefined ToString/Add2 vs Trace below SWF 7 and
nullish CallMethod result after operand validation. AGG/Cairo Release 253/253
each (AGG two launch-permission reruns); focused restricted headless ASan/UBSan
and portable 4/4 each. Original external SWF 6 undefined_to_string_swf6 finishes
with exact 22-byte reference output. Other numeric/version probes still differ;
stages 1/2 remain open. RU/EN: [SWF_REFERENCE_RUNNER.md](SWF_REFERENCE_RUNNER.md).

## v0.93 — Version-aware AVM1 numbers

Проверено / tested: primitive gates, counted legacy numeric grammar, signed
32-bit radix/exponent wraparound and pre-7 string truth. AGG/Cairo Release
254/254 each; focused restricted headless ASan/UBSan and portable 5/5 each.
Three original external SWF 5/6 traces exactly match pinned reference output.
Historical rounding/object fallback/function provenance and remaining actions
keep stage 2 open. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.94 — Top-level declarations

Проверено / tested: top-level clip/global DefineLocal/DeclareLocal, complete
coerced names, inherited setters, non-getting declarations and preserved formals.
AGG/Cairo Release 255/255 each (AGG four launch-permission reruns); restricted
headless ASan/UBSan and portable focused 4/4 each. External SWF 8 biturshift
now finishes but reveals a signed-result version difference; that probe still
fails exact comparison. Scope/path conformance remains open.
RU/EN: [AVM1_DECLARATIONS.md](AVM1_DECLARATIONS.md).

## v0.95 — SWF 8 shift result and bitwise coercion order

Проверено / tested: signed SWF 8 URShift result and right-first bitwise/remainder
coercion. AGG/Cairo Release 256/256 each (AGG three launch-permission reruns);
restricted headless ASan/UBSan and portable focused 3/3 each. Fourth original
external SWF trace matches: biturshift_swf8, 93 bytes. Other arithmetic/actions
and whole-stage acceptance remain open. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.96 — Legacy arithmetic operands

Проверено / tested: legacy arithmetic coercion/order, rooted operands and SWF 4
division-by-zero spelling. AGG/Cairo Release 257/257 each (AGG three
launch-permission reruns); restricted headless ASan/UBSan and portable focused
4/4 each. External SWF 4/8 coercion probes remain blocked by object conversion
or additional missing actions. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.97 — Numeric valueOf-only object conversion

Проверено / tested: loaded AVM1 numeric valueOf-only lookup, noncallable/missing
methods, object-result primitive gates and recursion recovery. AGG/Cairo Release
258/258 each after launch-permission reruns (3 and 2 tests); restricted headless
ASan/UBSan and portable focused 4/4 each. Four prior exact external SWF matches
reverified. equals_swf4 now completes but unversioned installed NaN/Infinity
globals still differ from its reference. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.98 — AVM1 string-only object conversion

Проверено / tested: loaded AVM1 toString-only conversion, non-string fallback,
boxed String bypass, counted results and recursion/error recovery. AGG/Cairo
Release 259/259 each (AGG two launch-permission reruns); restricted headless
ASan/UBSan and portable focused 4/4 each. MovieClip paths, Add2 hint behavior
and standard constructor/global version gates remain open.
RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.99 — Standard numeric member names

Проверено / tested: global and standard resource numeric/string member identity,
Object indexed bridge/prototype/accessor/flag behavior and graph transfer. Native
table integer/named separation retained. AGG/Cairo Release 260/260 each after
launch-permission reruns (3 and 1 tests); restricted headless ASan/UBSan and
portable focused 5/5 each. Full standard type/API inventory remains open.
RU/EN: [AVM1_NUMERIC_MEMBERS.md](AVM1_NUMERIC_MEMBERS.md).

## v0.100 — Legacy counted UTF-16 string actions

Проверено / tested: nine legacy string/integer actions, callback order and
converted-string roots across real script-triggered traced GC. AGG/Cairo Release
261/261 each (AGG six launch-permission reruns); restricted headless ASan/UBSan
and portable focused 4/4 each. Non-ASCII pre-SWF-6 length/extract are explicitly
unsupported; default-hint Add2 and remaining actions are open.
RU/EN: [AVM1_LEGACY_STRINGS.md](AVM1_LEGACY_STRINGS.md).

## v0.101 — Primitive coercion roots

Проверено / tested: transient primitive/string roots across script-triggered
traced GC and right-first Add2 conversion. The new regression reproduced the
former failure and now passes thirty repeated coercion cycles. AGG/Cairo
Release 262/262 each (AGG five launch-permission failures and dependent timezone
profile rerun); restricted headless ASan/UBSan and portable focused 4/4 each.
Default-hint/version conformance and whole stages remain open.
RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.102 — AVM1 primitive hint policy

Проверено / tested: valueOf-only ordinary/numeric hints, versioned Date default
hint, preserved default object results and early nonprimitive relational false.
AGG/Cairo Release 263/263 each (AGG two launch-permission reruns); restricted
headless ASan/UBSan and portable focused 5/5 each. Loose equality, player NaN,
MovieClip paths and per-function version provenance remain open.
RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.103 — Unordered relational result type

Проверено / tested: undefined Less2/Greater NaN results, right-first Greater
coercion and corresponding early-false/GC paths. AGG/Cairo Release 264/264 each
(AGG two launch-permission reruns); restricted headless ASan/UBSan and portable
focused 4/4 each. Large external relational SWFs still differ in root/version
behavior; they are not passing acceptance evidence.
RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.104 — SWF 5 loose equality and player NaN behavior

Проверено / tested: SWF 5 preconversion/callback counts, valueOf-only recursive
Equals2, loaded Flash Player 7/8 NaN equality, separate strict/legacy rules and
undefined prototype clearing. AGG/Cairo Release 265/265 each (AGG one
launch-permission rerun); restricted headless ASan/UBSan and portable focused
8/8 each. Three large external equality SWFs finish but still differ in top-level
this/root setup. Prototype attributes, MovieClip path equality and player-profile
selection remain open. RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.105–v0.113 — Rooted reference execution and display target actions

Проверено / tested: stage background frame controls and C89 host color API;
bounded root-timeline runner; four character-code actions; rooted local and
target-prefixed frame Call; host URL request bridge; separate dot/slash clip
paths with legacy callback restoration; counted target resolution; SetTarget/
SetTarget2 activation roots, variable scope and native frame controls; twelve
classic indexed display fields and read-only C89 target introspection.
RU/EN details: [stage](STAGE_BACKGROUND.md), [runner](SWF_REFERENCE_RUNNER.md),
[strings](AVM1_LEGACY_STRINGS.md), [frame Call](AVM1_FRAME_CALL.md),
[URL](AVM1_URL.md), [paths](MOVIECLIP_PATHS.md), [target scope](AVM1_TARGET_SCOPE.md),
[display properties](AVM1_DISPLAY_PROPERTIES.md).

v0.113 evidence: 276/276 core tests in AGG/Cairo, additional AGG runner test,
four focused portable/restricted-sanitizer tests each. Five launch failures
required restored execute bits and successful reruns. Ten original pinned SWF
4–8-scope probes (actual matching assets are SWF 5–8) complete with exact outputs;
large equal/less tests use actual imported roots, without patched movie headers.
Dimensions/system fields, full named-member/rename semantics, historic Unicode
case/codec tables, function source/version provenance, URL loaders/forms and
full SWF resources remain open. All nine roadmap stages remain open; stage 8
still has no implemented native JIT backend.

## v0.114 — native sparse slice / разреженный slice

Removed the 65,536 result-range cap from real/borrowed slice, with ordered live
prototype reads and candidate refresh after getters. Native work and nested
AVM1 callbacks share the instruction limit. Allocation failure tests verify
source preservation and execution-scope restoration. See
[RU/EN details](ARRAY_SPARSE_SLICE.md). Other Array limits and all stages remain open.

Evidence: 278 core tests in both Linux AGG and Cairo Release, plus the AGG
runner; four focused slice/Array tests in restricted ASan/UBSan and portable
profiles. Two AGG launch-permission failures passed after restoring execute
bits. The final added callback-budget and inherited-setter cases passed in
all four profiles. These checks do not certify complete Array or Flash semantics.

## v0.115 — large join / большой join

Removed the real/borrowed join length cap, retaining live inherited reads,
counted separator bytes and recursion cleanup. Native slot/output work shares
the execution limit with callbacks. [RU/EN details](ARRAY_LARGE_JOIN.md).
Evidence: 280 core tests in AGG/Cairo plus the AGG runner, four focused tests
in restricted sanitizer/portable profiles. The allocation test was corrected
from a large fixture whose allocation count exceeded its injection-loop bound
to exhaustive failure injection on a compact fixture; final tests pass in all
four profiles. Two AGG execute-bit failures passed after restoring permissions.
All stages and other Array semantic/size limitations remain open.

## v0.116 — inherited concat / наследуемые индексы concat

Concat now reads inherited indices in array arguments through the shared sparse
copy path, refreshing after getters. Plain borrowed receivers spread by length;
plain arguments stay single values, checked against pinned Ruffle source.
[RU/EN details and remaining historic gaps](ARRAY_CONCAT_SPARSE.md).
Evidence: 282 core tests in AGG/Cairo plus the AGG runner, four focused tests
in restricted sanitizer/portable profiles. Five AGG execute-bit launch failures
passed after permissions were restored. Final cases for later-argument length
changes and uint32 overflow passed in all four profiles. Exhaustive allocation
failures through success retain the source and restore execution nesting.
All roadmap stages remain open; no native JIT or other-OS completion is claimed.

## v0.117 — rejected splice arguments / отклонённые аргументы splice

Missing/undefined starts and explicit undefined/negative deletion counts now
return undefined without splice mutation. Missing starts do not read length.
Zero/null and omitted deletion remain valid; the already-supported undefined
slice endpoint has a regression check. [RU/EN details](ARRAY_UNDEFINED_BOUNDS.md).
Evidence: 283 core tests in AGG/Cairo plus the AGG runner; four focused Array
tests in restricted sanitizer/portable profiles. Three AGG execute-bit launch
failures passed after permission restoration. Final null/omitted/huge-range
cases passed in all four profiles. Initial development compiler errors in
undefined/null value initialization were fixed before running final binaries.
Historic integer/evaluation-order details and all roadmap stages remain open.

## v0.118 — splice coercion order / порядок преобразований splice

Coerce start before length and fast-path selection; avoid a second call to the
original start object when falling back to inherited reads. Removed values are
own result data without inherited setter calls. Errors precede length reads.
[RU/EN details](ARRAY_SPLICE_COERCION.md). Evidence: 284 core tests in AGG/Cairo
plus the AGG runner, four focused Array tests in restricted sanitizer/portable
profiles. One AGG file-writer launch lacked execute permission and its dependent
reader was skipped; both passed after permissions were restored. Delete-count
coercion-driven prototype changes and other historic Array gaps remain open.
All roadmap stages remain open.

## v0.119 — prepared splice bounds / подготовленные границы splice

A shared private preparation captures start, length and deletion once before
choosing the copy path. Delete-count-installed getters now use generic reads
without repeated conversions or length reads. Fast copies exclude late indices
outside the captured source interval. [RU/EN details](ARRAY_SPLICE_BOUNDS.md).
Evidence: 284 core tests in AGG/Cairo plus the AGG runner, four focused Array
tests in restricted sanitizer/portable profiles. Two AGG JPEG tests lacked
execute bits and passed after permission restoration. Getter installation,
source growth/shrinkage and bounds retention are checked. Historic integer,
hole, watcher and result-prototype semantics remain open; all stages are open.

## v0.120 — retained Array result prototype / встроенный прототип результатов

Standard installation publishes a retained original Array prototype as a GC
root. Six native result builders use it independently of custom source or
replacement public prototypes; explicit construction reads the public getter
once. [RU/EN details](ARRAY_RESULT_PROTOTYPE.md). Evidence: 285 core tests in
AGG/Cairo plus the AGG runner, seven focused Array/String tests in restricted
sanitizer/portable profiles (sanitizer still excludes stack/leak certification).
All pass without launch reruns. A real collection after public-prototype
replacement verifies retention and finalization once at teardown. Four newly
fetched original SWF 6/8 probes stop at unsupported FileAttributes/Metadata
tags before execution; no reference success is claimed for them. Imported
uninstalled-context fallback, Array() call semantics and all stages remain open.

## v0.121 — neutral SWF document tags / нейтральные служебные теги

Validate first-position FileAttributes (zero/HasMetadata only) and one declared
scalar UTF-8 Metadata string. Store flags and owned metadata in the clip graph;
new C89 document query leaves output unchanged on errors. Network/AVM2/hardware
policies remain unsupported. [RU/EN details](SWF_DOCUMENT_METADATA.md).
Evidence: 288 core tests in AGG/Cairo plus the AGG runner, four focused tests
in restricted sanitizer/portable profiles. Three AGG bitmap launch-permission
failures passed after restoring execute bits. Final document/API/allocation
tests pass in all four profiles. Ten earlier original SWF probes again complete
with exact outputs. Four new original SWF 6/8 probes now reach execution through
real timelines: Array/Object/InitArray mismatches and unhandled FSCommand remain;
array_trivial's matching Trace followed by failure does not count as acceptance.
Graph transport, real GC, malformed input and allocation failures are covered.
All roadmap stages remain open.

## v0.122 — runner host policy / тестовая политика хоста

Optional quit acknowledgement accepts only the exact counted FSCommand URL
with zero flags and continues bounded execution without external operations.
Default runner behavior is retained; unknown URLs/flags fail. The strict C89
runner build and generated runner test pass. Two original pinned assets now
finish successfully with exact output: add_swf5 (229 bytes, action mode) and
array_trivial SWF 6 (1447 bytes, real root timeline). Twelve probes pass under
explicitly documented modes/policies; actual player-quit behavior is not claimed.
[RU/EN details](SWF_REFERENCE_RUNNER.md). Core code is unchanged from the v0.121
288-test acceptance subset. Array/Object/InitArray gaps and all stages remain open.

## v0.123 — addProperty names/flags / имена и флаги addProperty

Coerce supplied names before descriptor validation, retaining numeric/object
and counted-NUL names. Null setters set read-only; writable replacement clears
that bit. Descriptor allocation failure restores new flag records before stored
values are removed. [RU/EN details](OBJECT_ADD_PROPERTY_NAMES.md). Evidence:
290 core tests in AGG/Cairo plus the AGG runner, four focused Object/Function/
Array checks in restricted sanitizer/portable profiles. Two AGG GC binaries
lacked execute permission and passed after restoration. Exhaustive allocation
retries preserve prior value/flags. Other descriptor/version rules and historic
Array join semantics remain open, as do all roadmap stages.

## v0.124 — loaded-SWF own-data join / исторический join

Loaded join reads own stored elements/length, uses signed wrapping length and
versioned string parts/separators. The existing AVM1 string helper is shared
with standard join/addProperty without changing public C conversion. Unloaded
contexts keep property-aware join. [RU/EN details](ARRAY_LEGACY_JOIN.md).
Evidence: 291 core tests in AGG/Cairo plus the AGG runner; four focused checks
in restricted sanitizer/portable profiles. One AGG GC callback test launch
needed execute-bit restoration and passed. An initial test exposed accidental
use of unversioned host conversion; shared AVM1 conversion fixes the SWF 4..8
spelling checks. All thirteen original probes complete with exact outputs under
documented policies, including array_prototyping SWF8 (121 bytes). Other Array
methods, function-version provenance and every roadmap stage remain open.

## v0.125 — initializer counts / счётчики инициализаторов

Loaded InitArray/InitObject coerce counts, preserve values when signed range
is invalid, truncate valid fractions and map NaN to zero. Valid counts beyond
the remaining materialization cap fail explicitly as unsupported.
[RU/EN details](AVM1_INITIALIZER_COUNTS.md). Evidence: 292 core tests in AGG/Cairo
plus the AGG runner, four focused checks in restricted sanitizer/portable
profiles. Two AGG channel/filter launch failures passed after execute-bit
restoration. The unit test checks preserved stack Trace for both initializers.
Original init_array_invalid remains blocked earlier by required ActionEnd;
no fourteenth oracle success is claimed yet. All roadmap stages remain open.

## v0.126 — bounded AVM1 blocks / ограниченные блоки

SWF frame import and execution accept empty blocks and implicit End exactly at
complete record boundaries; source bytes remain unchanged. Public inspect and
unloaded raw execution stay strict. Branch/invalid payload/graph transfer with
GC checks: [RU/EN details](AVM1_BOUNDED_BLOCKS.md). Evidence: 293 core tests in
AGG/Cairo plus the AGG runner; four focused tests in restricted sanitizer and
portable profiles. Three AGG channel test launches passed after execute-bit
restoration. All fourteen original probes now complete with exact expected
bytes, including init_array_invalid SWF8 (16 bytes, timeline mode). Semantic
version provenance across clip transfer and all roadmap stages remain open.

## v0.127 — initializer GC roots and names / корни и имена

InitObject roots the partial result and temporary names/values through script
coercion and setters; InitArray roots its result through indexed assignment.
Loaded Object keys use versioned AVM1 string conversion.
[RU/EN details](AVM1_INITIALIZER_ROOTS.md). Evidence: 294 core tests in AGG/Cairo
plus the AGG runner and four focused checks in restricted sanitizer/portable
profiles. Four AGG bitmap/filter launch failures passed after restoration and
retries. The final GC test was rebuilt and passed in all four profiles after
strengthening its partially populated object fixture: every script collection
collects at least one object, twenty times. Full literal setter semantics,
missing operands, constructor/version provenance and all stages remain open.

## v0.128 — initializer stack underflow / нехватка операндов

Loaded literals use undefined for missing counts, elements and pair operands;
Array results retain own undefined slots, Object names remain versioned.
Unloaded embedding retains strict errors. [RU/EN details](AVM1_INITIALIZER_UNDERFLOW.md).
Evidence: 295 core tests in AGG/Cairo plus the AGG runner, four focused checks
in restricted sanitizer/portable profiles. Four AGG AVM1 launch failures passed
after execute-bit restoration. Materialization, full constructor/descriptor
semantics, version provenance and all roadmap stages remain open.

## v0.129 — own Array result definitions / собственные индексы

InitArray and String.split bypass inherited accessors/readonly assignment and
define own stored slots. Existing slice/splice/concat share the private helper;
split records final length directly. [RU/EN details](ARRAY_RESULT_OWN_SLOTS.md).
Evidence: 296 core tests in AGG/Cairo plus the AGG runner; five focused checks
in restricted sanitizer/portable profiles. Six AGG Array/font/function launch
failures passed after execute-bit restoration. All fourteen original selected
SWF probes still complete with exact outputs. Full Array descriptors, version
provenance and all roadmap stages remain open.

## v0.130 — plain getter/setter Objects / обычные объекты дескрипторов

addProperty accepts plain Object descriptors; non-function getters yield
undefined, non-function setters ignore writes, and mixed descriptors invoke
the function side. [RU/EN details](OBJECT_DESCRIPTOR_OBJECTS.md). Evidence:
297 core tests in AGG/Cairo plus the AGG runner; five focused checks in
restricted sanitizer/portable profiles. Three AGG Array and two Cairo AVM1
launch failures passed after execute-bit restoration. The descriptor test
includes queue transfer and GC before reading/writing in the receiving context.
Protected replacement, full prototype/Array descriptor rules and all stages
remain open.

## v0.131 — loaded slice own data / исторический slice

Loaded slice captures own data length, uses signed wrapping Int32 endpoints,
ignores inherited/accessor elements and materializes own undefined slots.
Unloaded sparse/property-aware behavior stays available.
[RU/EN details](ARRAY_LEGACY_SLICE.md). Evidence: 298 core tests in AGG/Cairo
plus the AGG runner; five focused checks in restricted sanitizer/portable
profiles. All fourteen selected original SWF probes still complete with exact
output. Original array_slice is SWF15 and is not accepted or patched. Full Array
semantics, version provenance and every roadmap stage remain open.

## v0.132 — loaded concat own data / исторический concat

Loaded concat uses signed own-data length at each source turn, copies own stored
slots and defines explicit undefined results for missing data. Plain Object
arguments remain singletons; ordinary borrowed receivers spread. Unloaded
property-aware sparse behavior remains available.
[RU/EN details](ARRAY_LEGACY_CONCAT.md). Evidence: 299 core tests in AGG/Cairo
plus the AGG runner; five focused checks in restricted sanitizer/portable
profiles. Three AGG bitmap launch failures passed after execute-bit restoration.
Loaded allocation retries preserve source/root state and collect after every
attempt. All fourteen selected original SWFs still complete with exact outputs.
Full Array/version/prototype semantics and every roadmap stage remain open.

## v0.133 — signed splice endpoints / signed-границы splice

Loaded splice converts start/deleteCount through AVM1 signed wrapping Int32
exactly once, preserving conversion order and rejected-count no-mutation.
Unloaded mode retains truncation. [RU/EN details](ARRAY_LEGACY_SPLICE_BOUNDS.md).
Evidence: 300 core tests in AGG/Cairo plus the AGG runner; six focused checks
in restricted sanitizer/portable profiles. Four AGG ActionScript launch failures
passed after execute-bit restoration. Legacy fixtures now exclude their main
when reused, avoiding non-main implicit-return warnings. Splice length remains
property-aware/unsigned; historical removed slots, movement/descriptors and
all roadmap stages remain open.

## v0.134 — own removed splice slots / собственные удалённые элементы

Loaded splice removed results contain own undefined slots for missing/accessor/
inherited data. Generic removed copying bypasses getters; fast copying fills
missing slots, charges the shared budget and retains the partial result graph
with an allocation-free root. [RU/EN details](ARRAY_LEGACY_SPLICE_REMOVED.md).
Evidence: 302 core tests in AGG/Cairo plus the AGG runner; six focused checks
in restricted sanitizer/portable profiles. Five AGG Array/version launch failures
passed after execute-bit restoration. Fast allocation retries preserve source
and root/depth state through success with GC; the initial GC fixture was fixed
to pin its host method Array. All fourteen selected original SWFs still complete
with exact outputs. Historical source movement/write and signed own-data length,
generic mutation atomicity and all roadmap stages remain open.

## v0.135 — splice own signed length / собственная signed-длина

Loaded splice ignores virtual/inherited length, captures own data with Int32
wrapping after start and before deleteCount, and explicitly rejects negative
signed mutation lengths before deletion coercion/mutation. Unloaded mode stays
property-aware/unsigned. [RU/EN details](ARRAY_LEGACY_SPLICE_LENGTH.md).
Evidence: 303 core tests in AGG/Cairo plus the AGG runner; six focused checks
in restricted sanitizer/portable profiles. Five AGG Array/bitmap and three Cairo
bitmap launch failures passed after execute-bit restoration. Negative signed
mutation indices remain unsupported; this early rejection is not claimed as
full historical behavior. Source set_data/movement and all stages remain open.

## v0.136 — retained accessor data / data под getter

addProperty retains prior own stored values separately from getter/setter and
preserves them on descriptor replacement. Loaded Array reads/length use this
data; plain getters fall back to it. Existing hidden descriptor roots preserve
strings and cycles through graph transport without a wire-structure change.
[RU/EN details](OBJECT_ACCESSOR_DATA.md). Evidence: 304 core tests in AGG/Cairo
plus the AGG runner; seven focused checks in restricted sanitizer/portable
profiles. Traced NUL strings, owner cycles and virtual length survive GC and
queue transfer. All fourteen original selected SWFs still match exactly.
Ordinary assignment latent-data updates, replacement flags, native accessor/
method data and full version semantics remain open, as do all roadmap stages.

## v0.137 — accessor assignment data / data при присваивании

Successful ordinary setters update retained data, including non-function
setter Objects; callable getters still execute and plain getters return stored
data. Counted strings/cycles survive GC and transfer; readonly/error cases
preserve data. An allocation-free descriptor root spans setter execution.
[RU/EN details](OBJECT_ACCESSOR_ASSIGNMENT.md). Evidence: 305 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted sanitizer/
portable profiles. Four AGG bitmap and three Cairo Function/inheritance launch
failures passed after execute-bit restoration. All fourteen selected original
SWFs still match exactly. Array mutation set_data, replacement flags, inherited
setter data, thrown-setter/version rules and all stages remain open.

## v0.138 — InitObject empty names / пустые имена

Loaded InitObject coerces names, then ignores an empty result; unloaded host
mode retains empty keys. Earlier underflow/GC expectations for SWF6 were
incorrect and are corrected. [RU/EN details](AVM1_INITIALIZER_EMPTY_NAMES.md).
Evidence: 306 core tests in AGG/Cairo plus the AGG runner; five focused checks
in restricted sanitizer/portable profiles. Eight AGG Array/bitmap and four
Cairo Array launch failures passed after execute-bit restoration. Fifteen
original SWFs now complete with exact expected output, including newly selected
init_object_invalid SWF8 (16 bytes). add_property/init_object_order are SWF15
and do not count as supported successes. General empty-name property behavior
and all roadmap stages remain open.

## v0.139 — preserved virtual flags / flags замены

addProperty preserves existing own-property flags, allows DONT_DELETE descriptor
replacement while keeping deletion protection, and marks only new null-setter
properties readonly. Writable existing null setters use the script data bridge.
[RU/EN details](OBJECT_ADD_PROPERTY_FLAGS.md). Evidence: 307 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted sanitizer/
portable profiles. One AGG bitmap launch failed and passed after execute-bit
restoration. Full suites used serial CTest scheduling for this change.
All fifteen selected original SWFs still match exactly. Corrected names and
allocation fixtures now retain existing flags. Real Array index/length guards,
readonly callbacks, inherited setter data and all roadmap stages remain open.

## v0.140 — inherited setter ownership / владелец setter

CAPI setter dispatch scopes actual lookup ownership with allocation-free roots
for owner, receiver and capture. Inherited script setters execute with receiver
without changing prototype data; nested own setters restore prior metadata.
Existing descriptors need no new wire fields.
[RU/EN details](OBJECT_INHERITED_SETTER_DATA.md). Evidence: 308 core tests in
AGG/Cairo plus the AGG runner; seven focused checks in restricted sanitizer/
portable profiles. Two AGG property/prototype launch failures passed after
execute-bit restoration. Nested setters, counted NUL, GC/queue transfer and
owner restoration after errors pass; all fifteen original SWFs still match.
Readonly callback rules, self-detachment/replacement, Array mutation set_data,
versions and every roadmap stage remain open.

## v0.141 — loaded readonly setter execution / readonly callback

Loaded script setters execute on readonly descriptors while current own flags
control retained data after callback. Clearing readonly inside the setter enables
storage; inherited readonly data permits own shadowing. Unloaded C checks remain
strict. A scoped owned counted key survives callback mutation/nesting; copy
failure precedes metadata changes. [RU/EN details](OBJECT_READONLY_SETTER.md).
Evidence: 309 core tests in AGG/Cairo plus the AGG runner; seven focused checks
in restricted sanitizer/portable profiles. Three AGG GC launch failures passed
after execute-bit restoration. Numeric/countеd NUL callbacks and metadata restore
pass; all fifteen original SWFs still match. Readonly watcher order, self-replaced
property storage, real Array virtual guards/version rules and all stages stay open.

## v0.142 — setter replacement / замена setter

Successful own script setters store into the current receiver property after
callback deletion/replacement, without another setter invocation; current own
readonly suppresses storage. Counted bytes and both descriptor graphs remain
owned/rooted. [RU/EN details](OBJECT_SETTER_REPLACEMENT.md). Evidence: 310 core
tests in AGG/Cairo plus the AGG runner; seven focused tests in restricted
heap/global ASan+UBSan and portable Linux profiles. One AGG blur-filter launch
failure passed after execute-bit restoration. Fifteen original SWFs still match
exactly. Native C accessor replacement remains UNSUPPORTED; callback side effects
are not rolled back. Array mutation set_data and all stages stay open.

## v0.143 — loaded reverse own data / reverse собственных данных

Loaded reverse reads own signed length and raw element data, ignores prototypes
and accessors, deletes paired positions before storing without setter callbacks,
and materializes missing undefined values. Protected virtual data retains own
flags. [RU/EN details](ARRAY_LEGACY_REVERSE.md). Evidence: 311 core tests in
AGG/Cairo plus the AGG runner; five focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. One AGG bitmap-allocation launch failure
passed after execute-bit restoration. All fifteen original SWFs still match
exactly. The mutation size limit/native accessor model, other mutators and every
roadmap stage remain open.

## v0.144 — loaded shift/pop raw data / удаление элементов

Loaded shift/pop use own raw data without getter/prototype lookup. Borrowed
length remains unchanged; only native Arrays shrink. Protected native tail
indices may remain beyond the smaller length instead of clamping it.
[RU/EN details](ARRAY_LEGACY_REMOVE.md). Evidence: 312 core tests in AGG/Cairo
plus the AGG runner; six focused checks in restricted heap/global ASan+UBSan
and portable Linux profiles. One AGG broker-lifetime launch failure passed after
execute-bit restoration. All fifteen original SWFs match exactly. Negative signed
lengths, shift size limits, native accessor data and all stages remain open.

## v0.145 — empty Object assignment / пустое имя свойства

Loaded ordinary Object writes ignore empty keys before property callbacks,
while SetMember still coerces the key once. Preexisting empty host properties
remain readable; single counted NUL keys remain writable.
[RU/EN details](OBJECT_EMPTY_ASSIGNMENT.md). Evidence: 313 core tests in
AGG/Cairo plus the AGG runner; five focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. No launch failures in these suites.
All fifteen original SWFs still match exactly. SetVariable/local scope, function
versions and every roadmap stage remain open.

## v0.146 — ASSetPropFlags set precedence / приоритет установки

The AS builtin clears before setting overlapping flags; its set bits win.
The direct C flag extension remains unchanged.
[RU/EN details](STANDARD_PROPERTY_FLAGS.md). Evidence: 314 core tests in
AGG/Cairo plus the AGG runner; four focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Exhaustive 512 flag combinations pass
in unloaded mode and each SWF4..8; counted NUL, readonly and own selection pass.
Four AGG and two Cairo launch failures plus an AGG runner PermissionError passed
after execute-bit restoration. Fifteen original SWFs still match exactly.
Selector/default coercions, version masks and all stages remain open.

## v0.147 — ASSetPropFlags selectors/defaults / выбор свойств

Only NULL selects all own fields. Undefined is converted with loaded AVM1
string rules (empty in SWF4..6, "undefined" in SWF7..8); missing masks are zero,
and missing object/list returns undefined without mutations.
[RU/EN details](STANDARD_PROPERTY_FLAGS.md). Evidence: 314 core tests in
AGG/Cairo plus the AGG runner; four focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. No launch failures. Existing C89 tests
cover each selector/default path and all 512 mask combinations in each mode.
Fifteen original SWFs still match. Full receiver/mask conversion and all stages
remain open.

## v0.148 — loaded watcher names/data / watcher в SWF

Loaded watch/unwatch coerce names before callback validation, allow empty and
virtual names, and read old values through stored own/prototype data without
getters/metatable hooks. Native field/data restrictions remain explicit.
[RU/EN details](OBJECT_LEGACY_WATCH.md). Evidence: 315 core tests in AGG/Cairo
plus the AGG runner; five focused checks in restricted heap/global ASan+UBSan
and portable Linux profiles. Three AGG bitmap launch failures passed after
execute-bit restoration. Numeric/object/empty/counted NUL, GC and inherited
setter ownership pass; fifteen original SWFs still match. Readonly and exception
ordering, native accessor data, case/version rules and all stages stay open.

## v0.149 — watcher before readonly / watcher до readonly

Loaded ordinary/indexed writes run watchers before current readonly checks.
Callbacks can alter flags, indexed setter dispatch consumes prepared data once,
and lookup ownership is rooted through detachment.
[RU/EN details](OBJECT_READONLY_WATCH.md). Evidence: 316 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Three AGG Array launch failures passed
after execute-bit restoration. Named/numeric/countеd NUL, virtual setter flags,
root unwind and post-write GC pass; all fifteen original SWFs still match.
Native Array guard and watcher exception ordering, case/version and all stages
remain open.

## v0.150 — registerClass name validation / имя класса

Constructor validation precedes name conversion; loaded registration uses the
AVM1 string bridge and permits empty symbols. Null/undefined removes entries.
[RU/EN details](OBJECT_REGISTER_CLASS_NAMES.md). Evidence: 317 core tests in
AGG/Cairo plus the AGG runner; seven focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Invalid constructors skip name callbacks;
empty/undefined/counted NUL, GC and queue transfer with restored execution pass.
Four AGG launch failures, one dependent reader and one subprocess PermissionError
passed on explicit rerun after restoring executable bits. Fifteen original SWFs
still match; case-sensitive environment separation, function versions and all
stages remain open.

## v0.151 — single property-name coercion / однократное имя

Own/enumeration queries coerce a counted key once with loaded AVM1 rules, and
return false without an argument. Enumeration presence and flags share that
key instead of invoking toString twice.
[RU/EN details](OBJECT_PROPERTY_QUERIES.md). Evidence: 318 core tests in
AGG/Cairo plus the AGG runner; four focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Changing callback names, missing versus
undefined, hidden flags and counted NUL pass across unloaded/SWF4..8. Two AGG
bitmap launch failures passed after execute-bit restoration. Fifteen original
SWFs still match. Case/version/native fields and all stages remain open.

## v0.152 — loaded unshift raw data / unshift собственных данных

Loaded unshift moves raw own data without accessor/watcher callbacks, materializes
holes even with no arguments and preserves borrowed length. Native readonly
length allows out-of-length raw slots. Length coercion shares traversal work
accounting in loaded unshift/shift/pop.
[RU/EN details](ARRAY_LEGACY_UNSHIFT.md). Evidence: 319 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Protected virtual data, counted NUL/GC,
native/borrowed lengths and combined valueOf+traversal budget pass. Three AGG
bitmap/shape/mutex launch failures passed after execute-bit restoration. Fifteen
original SWFs still match. Signed/size limits, native accessor data and all
roadmap stages remain open; partial mutation is not rolled back.

## v0.153 — loaded push raw data / push собственных данных

Loaded push writes raw own data without descriptor deletion or accessor/watch
callbacks and updates raw length under own flags. Native readonly length permits
raw slots beyond it. [RU/EN details](ARRAY_LEGACY_PUSH.md). Evidence: 320 core
tests in AGG/Cairo plus the AGG runner; six focused checks in restricted
heap/global ASan+UBSan and portable Linux profiles. Virtual length/index, readonly
data, no watchers and counted NUL/GC pass. One AGG error-transfer launch failure
and its dependent subprocess PermissionError passed after execute-bit restoration.
Fifteen original SWFs still match. Signed/native accessor limits, case/version
and all stages remain open; no rollback on partial mutation is promised.

## v0.154 — generic splice raw moves / общий перенос splice

Loaded generic splice moves by own presence/raw data, bypasses callbacks/watch,
handles both directions and retains borrowed tails beyond new length. Native
generic protected tails do not clamp length. Getter-only C accessor raw writes
retain their immutable contract; writable native accessor data is unsupported.
[RU/EN details](ARRAY_LEGACY_SPLICE_MUTATION.md). Evidence: 321 core tests in
AGG/Cairo plus the AGG runner; nine focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. An initial getter-only length regression
was corrected; all repeated suites pass without launch failures. Virtual flags,
prototype holes, borrowed/native tails and counted strings after traced GC pass.
Fifteen original SWFs still match. Native fast splice watch/flags, signed/size
limits, native accessor data and all stages remain open.

## v0.155 — native splice flags/watch / native флаги splice

Loaded indexed flags/readonly length route native splice through live generic
raw operations, respecting source deletion before later destination writes.
The ordinary unflagged fast path skips watchers while retaining staging/OOM
checks. [RU/EN details](ARRAY_LEGACY_SPLICE_FLAGS.md). Evidence: 322 core tests
in AGG/Cairo plus the AGG runner; nine focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. All eight indexed masks, protected tails,
readonly length, watcher bypass and inherited getter holes pass. The final pinned
prototype fixture passed all four profiles. Two AGG bitmap/broker launch failures
passed after execute-bit restoration. Fifteen original SWFs still match. Routed
flagged array size limits, sort mutation/case/version and all stages remain open.

## v0.156 — native sort primitive keys / примитивные ключи sort

Loaded native fast sort/sortOn skips commit watchers and converts primitive/field
keys with the AVM1 string bridge. SWF4 Boolean and pre-SWF7 missing fields use
versioned strings. [RU/EN details](ARRAY_LEGACY_SORT_KEYS.md). Evidence: 323 core
tests in AGG/Cairo plus the AGG runner; four focused checks in restricted
heap/global ASan+UBSan and portable Linux profiles. Unloaded/SWF4..8 watcher,
Boolean and missing sortOn field ordering pass. Four AGG gradient/table/codec
launch failures passed after execute-bit restoration. All fifteen original SWFs
still match. Generic property/prototype reads/writes, callback order, flags/holes
and all roadmap stages remain open.

## v0.157 — generic sort own snapshot / собственный снимок sort

Loaded generic sort/sortOn snapshots own raw element data and signed length,
materializes undefined holes and writes raw data without index getters/setters/
watchers. Snapshot has an explicit GC root through callbacks/copyback.
[RU/EN details](ARRAY_LEGACY_SORT_DATA.md). Evidence: 324 core tests in
AGG/Cairo plus the AGG runner; five focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Borrowed virtual length/index, inherited
holes, readonly data, no index callbacks and post-GC data pass. Five AGG Array/GC
launch failures passed after execute-bit restoration. Fifteen original SWFs
still match. Native sparse holes, callback/flag/return rules, native accessor data
and all stages remain open.

## v0.158–v0.160 — loaded sorting corrections / исправления сортировки

Loaded Array.sort handles DESCENDING after comparison by reversing the complete
result, including equal keys. Undefined uses versioned AVM1 string keys and is
passed to custom comparators; the SWF6 v0.157 expectation is corrected. Loaded
sortOn returns the original receiver, including generic borrowed copyback.
[Direction/return details](ARRAY_LEGACY_SORT_COMPARATOR.md) and
[raw-data/undefined details](ARRAY_LEGACY_SORT_DATA.md) describe RU/EN behavior.
All earlier stage completion states remain open. Exact comparator order,
custom UNIQUESORT, native sparse holes and sortOn field lookup remain open.

Загруженный sort разворачивает весь результат для DESCENDING, включая равные
ключи. undefined сравнивается с учётом версии AVM1 и передаётся comparator.
sortOn возвращает исходный receiver, включая заимствованный метод.
Исправлено ожидание SWF6 из v0.157. Все стадии остаются открытыми.

Evidence through v0.160: AGG 326/Cairo 325 tests, five focused checks in
portable Linux and restricted ASan/UBSan, and fifteen byte-exact original SWFs.
AGG execute-bit launch failures passed after restoring executable bits.
Полные проверки AGG/Cairo и профильные portable/ASan/UBSan прошли;
15 исходных SWF совпали побайтово. Это не подтверждает полную совместимость.

## v0.161 — splice source retention / сохранение source index

Corrected the v0.154/v0.155 source-deletion interpretation: pinned Ruffle
2372a4e splice_internal copies values without deleting sources. Generic loaded
splice now retains source properties/flags; native length shrinking separately
removes trailing indices. Borrowed tails remain beyond the new length. All eight
indexed masks now retain their flags, and readonly blocks subsequent writes.
[RU/EN details](ARRAY_LEGACY_SPLICE_FLAGS.md). Comparator order, large/signed
Array limits and all roadmap stages remain open.

Исправлено прежнее ошибочное удаление источника: splice копирует данные,
сохраняя флаги. Только native length удаляет хвост; borrowed хвост остаётся.
Это исправляет описание и ожидания v0.154/v0.155. Все стадии открыты.

Evidence v0.161: 326 AGG and 325 Cairo tests, eight focused checks each in
portable Linux and restricted heap/global ASan+UBSan, fifteen byte-exact original
SWFs. Cairo timezone launch failure and its dependent profile test passed after
execute-bit restoration. Проверки пройдены; stage completion не заявлено.

## v0.162 — rooted loaded sort snapshot / полный снимок сортировки

Loaded sort captures all own values before key coercion; native sparse holes
become own undefined values. An explicit [FVM_Value](../include/flashvm/value.h#L24) collector frame retains the
complete original graph through callbacks and restores caller roots on all exits.
[RU/EN details](ARRAY_LEGACY_SORT_SNAPSHOT.md). Evidence: AGG 327/Cairo 326,
six focused portable/restricted heap-global ASan+UBSan tests, fifteen byte-exact
original SWFs. Seven AGG launch failures passed after execute-bit restoration.
Tests verify coercion clearing the receiver, original rows after actual post-sort
GC, callback error root/depth restoration and native undefined holes. Arbitrary
native callback GC remains limited by the existing native barrier. Callback order,
custom UNIQUESORT and all roadmap stages remain open.

Полный снимок захватывается до toString и сохраняется в корнях GC. Проверены
очистка receiver из callback, сохранение исходных объектов и sparse native holes.
Сборка выполняется после sort; GC внутри произвольного native callback не заявлен.

## v0.163 — loaded quicksort / порядок сравнений

Loaded sort/sortOn uses left-pivot quicksort with the reference LIFO comparison
order; unloaded stable merge remains unchanged. Every comparison/partition shares
the instruction budget and cancellation checks. [RU/EN details](ARRAY_LEGACY_SORT_ORDER.md).
Evidence: AGG 327/Cairo 326, six focused portable/restricted heap-global ASan+UBSan
checks, fifteen byte-exact original SWFs. Three AGG launch failures passed after
execute-bit restoration. Seven pinned comparator pairs, DESCENDING/indexed return,
third-comparison error, root/depth restoration, pre-callback budget exhaustion and
sorted worst-case bounds pass. Eager builtin/field keys, custom UNIQUESORT and all
stages remain open.

Проверены эталонные пары comparator, ошибки и предел работы quicksort.
Точная семантика преобразования ключей и все стадии пока открыты.

## v0.164 — comparison-time coercion / преобразования пар

Loaded Array.sort performs builtin AVM1 string conversion per comparison, left
before right, while NUMERIC compares two Numbers directly. Intermediate operand/
string roots remain live across later callbacks. Custom UNIQUESORT checks adjacent
final values with builtin comparison instead of invoking the custom callback.
[RU/EN details](ARRAY_LEGACY_SORT_COERCION.md). Evidence: AGG 328/Cairo 327,
seven focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. Fourteen pinned toString calls, conversion errors, equal/positive
custom uniqueness and snapshot clearing pass. A test-local expired string buffer
was replaced by a literal; all seven final focused tests passed all four profiles.
One AGG bitmap launch failure passed after restoring executable bits. sortOn field
lookup/coercion, native GC barriers and all stages remain open.

Проверены повторные преобразования каждой пары и отдельная builtin проверка
UNIQUESORT. Исправлена ошибка времени жизни строки в тесте. Все стадии открыты.

## v0.165 — own live sortOn fields / собственные данные полей

Loaded sortOn reads own stored fields at each comparison without getters/prototype
lookup, using retained script descriptor data, native Array length/index values and
lazy native method values. Field coercion is left before right; changes from
callbacks are visible later. Primitive elements use full first-field options.
[RU/EN details](ARRAY_LEGACY_SORT_FIELDS.md). Evidence: AGG 328/Cairo 327,
seven focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. Counted NUL fields, pinned ascending/descending conversions,
callback field changes, inherited getter bypass, versioned missing values,
conversion errors and primitive numeric descending pass. Two AGG launch failures
passed after execute-bit restoration. Native display fallback, SWF case/version
masks, argument overloads and all stages remain open.

Проверены собственные virtual данные, повторное чтение изменённых полей и
отсутствие getter. Native display fallback и все стадии остаются открытыми.

## v0.166 — loaded raw sort copyback / обратная запись sort

Loaded native sort/sortOn writes captured positions sequentially through shared
raw element storage, preserving current receiver length/tails and readonly length
semantics after callback changes. Unloaded staging is unchanged.
[RU/EN details](ARRAY_LEGACY_SORT_COPYBACK.md). Evidence: AGG 328/Cairo 327,
seven focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. Callback tail growth, readonly length one/zero with own outside
slots, errors after growth, indexed return with captured length, restored roots/
depth and getter/watcher bypass pass. Three AGG launch failures passed after
restoring execute bits. Sequential loaded copyback may leave partial mutation on
allocation/budget errors; full argument/flag cases and all stages remain open.

Проверены сохранение callback хвоста, READ_ONLY length и индексы за длиной.
Ошибки обратной записи могут оставить частичные изменения. Все стадии открыты.

## v0.167 — virtual Array indices / виртуальные индексы

Loaded native Array addProperty accepts numeric indices, retaining original data
and flags in script descriptors before removing indexed slots. Ordinary getters
and loaded raw methods now expose their distinct values.
[RU/EN details](ARRAY_LEGACY_VIRTUAL_INDEX.md). Evidence: AGG 329/Cairo 328,
eight focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs, no launch failures. Counted NUL strings, readonly/nondelete flags,
self-cycles, raw slice/getter distinction and AVM1 getter graph transfer all pass
with traced GC in both contexts. Unloaded rejection is unchanged. Native length
virtualization, readonly native setter dispatch, version masks and all stages
remain open.

Проверены сохранение indexed data/flags, cycle и queue transfer после GC.
Виртуальная native length и все стадии остаются открытыми.

## v0.168 — readonly Array virtual setter / setter readonly индекса

Loaded native Array script setters execute for readonly virtual index data, while
retained data stays protected. Watcher ordering/raw old-value behavior is retained.
[RU/EN details](ARRAY_LEGACY_VIRTUAL_INDEX.md). Evidence: AGG 329/Cairo 328,
five focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. AVM1 setter receiver markers, counted old strings in watchers,
writable/readonly data distinction, traced GC and repeat callback execution after
channel transfer pass. Three AGG launch failures passed after execute-bit
restoration. Native readonly length growth guards, virtual length, SWF attribute
version masks and all stages remain open. Unloaded behavior is unchanged.

Проверены setter/watcher после GC и передачи, защищённые raw данные и receiver.
Нативная length и все стадии пока открыты.

## v0.169 — loaded sort allocation faults / отказы памяти

Successive C++ new/new[] boundaries are faulted through SWF8 sort, UNIQUESORT,
RETURNINDEXEDARRAY and sortOn, with fresh contexts and traced post-call GC.
Temporary roots/depth/setter metadata restore on each OOM. C++ allocation counts
return to runtime baseline after every context and zero after runtime destruction.
[RU/EN details](ARRAY_LEGACY_SORT_ALLOCATION.md). Evidence: AGG 330/Cairo 329,
nine focused portable/restricted heap-global ASan+UBSan tests, fifteen byte-exact
original SWFs, no launch failures. Partial sequential copyback is explicitly
accepted; indexed return leaves the original values unchanged. C malloc, other
lengths/callback effects, native GC barriers and all roadmap stages remain open.

Проверены каждый C++ allocation boundary до успеха, очистка корней и временной
памяти. Это не заменяет C malloc/leak-инструменты и не закрывает стадии.

## v0.170 — indexed length / индекс и readonly length

Loaded indexed assignment grows native Array length after watching and before data
flags/setters; readonly length blocks growth but permits own outside slots and
virtual setters. Shared note_index respects readonly length. Graph decode accepts
valid outside slots while retaining reserved-index validation.
[RU/EN details](ARRAY_LEGACY_INDEX_LENGTH.md). Evidence: AGG 331/Cairo 330,
nine focused portable/restricted heap-global ASan+UBSan tests, fifteen byte-exact
original SWFs. Outside child strings through traced GC/queue transfer, readonly
virtual data/setter markers, growth ordering and CRC-valid reserved-index rejection
without destination allocations pass. One AGG/four Cairo launch failures passed
after execute-bit restoration. Unloaded guards remain unchanged. Virtual length,
signed indices, ordinary truncation/nondelete, version masks and all stages remain
open. Growth before allocation failure may persist; atomicity is not claimed.

Проверены readonly length, own хвосты и согласованность decoder после передачи.
Защита зарезервированного индекса сохранена; все стадии остаются открытыми.

## v0.171 — empty watcher decode / пустое имя watcher

Graph decoding accepts empty watcher keys supported by loaded watch, preserving
descriptor/duplicate/reference validation. [RU/EN details](OBJECT_EMPTY_WATCH_TRANSFER.md).
Evidence: AGG 332/Cairo 331, eight focused portable/restricted heap-global ASan+UBSan
checks, fifteen byte-exact original SWFs. SWF4..8 pre-load empty data, AVM1 handler,
self userData, traced GC/queue transfer/GC, aliasing, ignored empty assignments,
unwatch true/false and restored watch/GC state pass. Three AGG resource launch
failures plus their subprocess check and two Cairo launch failures passed after
restoring execute bits. Format/ABI, attribute version masks and all roadmap stage
completion states remain unchanged.

Пустой watcher сохраняется после передачи и удаляется корректно. Исходное
правило игнорирования assignment пустого имени сохранено; все стадии открыты.

## v0.172 — length truncation / сокращение Array.length

Loaded native Array deletes the signed bounded tail before readonly length
storage. Nondelete slots remain outside length; slots beyond the old signed
length are untouched. Work-budget failure retains prior deletions. Unloaded
behavior is preserved. [RU/EN details](ARRAY_LEGACY_LENGTH_TRUNCATE.md).
Tests cover SWF4..8, readonly/nondelete flags, the 2^31 boundary, partial budget
failure, root restoration and NUL tails through traced GC/channel transfer.
Evidence: AGG 333/Cairo 332, 23 focused checks each on Linux portable and
restricted heap/global ASan+UBSan profiles, and fifteen byte-exact original SWFs.
Three AGG launch failures passed after restoring executable permissions.
Full ToInt32 input conversion, negative stored length and virtual native length
remain open, as do all roadmap stages.

Порядок удаления хвоста и записи readonly length исправлен. Защищённые
свойства не удерживают длину; все стадии роудмапа остаются открытыми.

## v0.173 — sort argument dispatch / аргументы sort

Loaded sort wraps numeric options through Int32 and masks known bits; a second
Number overrides the first Number. Nonnumeric comparator options are ignored.
Other primitive first arguments return undefined without sorting; empty sortOn
returns undefined. Unloaded validation is preserved.
[RU/EN details](ARRAY_LEGACY_SORT_ARGUMENTS.md). SWF4..8 tests cover fractions,
high/wrapping options, NaN/infinities, ignored primitives and numeric overloads.
Full sortOn dispatch, noncallable Object behavior and all stages remain open.
Evidence: AGG 334/Cairo 333 tests, nine focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, fifteen byte-exact original SWFs.
Four AGG launch failures passed after restoring executable permissions.

Исправлены числовые flags и часть перегрузок загруженной сортировки;
полнота Array и завершение стадий не заявлены.

## v0.174 — sortOn overloads / перегрузки sortOn

Loaded field/option Arrays read retained own data without getters. All field
names are converted before option entries; a common primitive option is coerced
once, mismatched Arrays/non-Array Object options are ignored, and a single
primitive field accepts only Number options. Empty field Arrays return this;
non-Array Object fields select ordinary comparison. Preparation shares the work
budget. [RU/EN details](ARRAY_LEGACY_SORT_ON_ARGUMENTS.md).
SWF4..8 tests cover overloads, virtual indices, getter bypass, wrapping flags,
empty fields, fallback and restored root/depth state. The 64-field cap, full
signed native length, arbitrary native GC and all roadmap stages remain open.
Evidence: AGG 335/Cairo 334 tests, ten focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, fifteen byte-exact original SWFs.
Three AGG launch failures passed after restoring executable permissions.

Исправлены перегрузки и порядок подготовки полей/flags загруженного sortOn;
проверены сохранённые данные виртуальных индексов без вызова getter.

## v0.175 — no-op sort dispatch / ранний выход сортировки

Loaded unsuitable primitive sort arguments, argument-free sortOn and empty
field Arrays return before borrowed length reads/snapshots/copyback. This
preserves receiver data and avoids budget/cap failures on no-op overloads.
[RU/EN details](ARRAY_LEGACY_SORT_NOOP.md). SWF4..8 tests cover native length
100000, borrowed length getters, budget 1, missing indices and original data.
Additional coercion tests pin all field conversions before options, mutation
of the later option, errors without sorted copyback, retained callback effects
and restored root/depth/setter state. All roadmap stages remain open.
Evidence: AGG 337/Cairo 336 tests, twelve focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, fifteen byte-exact original SWFs.
Four AGG and two Cairo launch failures passed after restoring execute bits.

Ранний выход выполняется без чтения и перезаписи receiver; проверены порядок
пользовательских преобразований и их побочные эффекты при ошибках.

## v0.176 — plain sort comparator / обычный Object comparator

Loaded sort accepts a noncallable Object comparator and treats its undefined
call result as equal keys without invoking call/valueOf properties. Descending,
indexed output and builtin uniqueness retain their existing rules. Unloaded
noncallable guarding is preserved. [RU/EN details](ARRAY_LEGACY_SORT_PLAIN_COMPARATOR.md).
SWF4..8/unloaded tests cover trap methods, flags 0/2/8/10/4/12, ordering, index
no-copyback, distinct/duplicate values and restored root/depth state. Full
Array/AVM1 semantics and all roadmap stages remain open.
Evidence: AGG 338/Cairo 337 tests, thirteen focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, fifteen byte-exact original SWFs.
Three AGG launch failures passed after restoring execute bits.

Обычный Object больше не вызывает UNSUPPORTED в загруженном sort; его
свойства call/valueOf не участвуют в сравнении.

## v0.177 — borrowed sortOn capture / захват receiver после подготовки

Loaded sortOn prepares fields/options once before native or borrowed receiver
capture. Private synchronous metadata forwarding carries copied strings/flags
into the snapshot sorter without managed edges or retained pointers. Callback
changes to length/data are included. Non-Array Object field fallback now works
on borrowed receivers. Public C APIs and unloaded semantics are preserved.
[RU/EN details](ARRAY_LEGACY_SORT_ON_BORROWED_PREPARE.md). Tests cover SWF4..8,
Object/Array growth, field/option mutations, exactly-once conversion, errors in
both phases retaining effects without sorted copyback, post-call traced GC and
restored state. Full Array/AVM1 semantics and all stages remain open.
Evidence: AGG 339/Cairo 338 tests, sixteen focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, fifteen byte-exact original SWFs.
The original object_prototypes SWF remains mismatched for primitive prototype
data, prototype deletion and property-name case; it is not a passing oracle.

Заимствованный sortOn захватывает длину и элементы после преобразования всех
полей/flags; пользовательские изменения участвуют в текущей сортировке.

## v0.178 — numeric globals by version / версия числовых globals

Native NaN/Infinity getters use the current loaded version: SWF4 undefined,
SWF5..8 IEEE values. Loaded writes are ignored; unloaded host values remain
mutable, traced and transportable. Staged accessor/property maps publish together;
new numeric callback IDs 158..160 preserve existing IDs.
[RU/EN details](GLOBAL_NUMERIC_VERSION.md). Tests cover install-before/after-load,
SWF4..8, mutable Object/NUL values, loaded writes, traced GC/channel transfer and
destination version changes. Original equals_swf4 now matches all 4049 output
bytes; the original SWF/expected output were not modified. Other globals/case,
attribute masks, separate function/clip versions and all stages remain open.
Evidence: AGG 340/Cairo 339 tests, four focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles including allocation failure,
and sixteen byte-exact original SWFs. Two AGG launch failures passed after
restoring execute bits.

Исправлены NaN/Infinity для SWF4; исходный equals_swf4 добавлен к побайтным
эталонным проверкам. Незагруженные host значения сохраняются после GC/передачи.

## v0.179 — numeric global attributes / атрибуты NaN и Infinity

NaN/Infinity carry DONT_ENUM without DONT_DELETE, including unloaded host mode.
The attribute map is staged/published with descriptors and globals. Extended
global_numeric_version checks own/nonenumerable membership, transported flags,
explicit nondelete protection, clearing flags, enumeration, deletion and ordinary
data recreation after deletion/post-GC. [RU/EN details](GLOBAL_NUMERIC_VERSION.md).
Other global/version attribute masks and all roadmap stages remain open.
Evidence: AGG 340/Cairo 339 tests, seven focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, sixteen byte-exact original SWFs.
Four AGG/two Cairo launch failures passed after restoring execute bits.

NaN/Infinity скрыты из перечисления, допускают удаление без DONT_DELETE;
проверены изменение flags, передача и обычное свойство после удаления.

## v0.180 — Flash UCS2 casing / регистр загруженных строк

Loaded String casing and CASEINSENSITIVE sorting use pinned Flash one-unit
mappings rather than Unicode scalar/context/expansion rules. Surrogates remain
unchanged; host Unicode 3.2 behavior is retained. Bundled upstream tables and
MIT license, plus an offline generator, reproduce the C++98 header.
[RU/EN details](STRING_LEGACY_CASE.md). Tests cover SWF4..8/host sharp-S, dotted I,
sigma, ligature, supplementary/lone surrogates, NUL, retained post-GC results and
dotted-I UNIQUESORT. A separate public-API SWF8 probe matched both upper/lower
outputs for all 65536 units against independently parsed pinned upstream data.
Property-name case, independent original Flash Player verification and all
roadmap stage completion states remain open.
Evidence: AGG 341/Cairo 340 tests, seventeen focused checks each in Linux portable
and restricted heap/global ASan+UBSan profiles, sixteen byte-exact original SWFs.
Five AGG launch failures passed after restoring execute bits.

Загруженные строки и сортировка используют Flash UCS2 mappings; выполнена
полная сверка диапазона 16-bit units с таблицей pinned Ruffle, без заявления
полной совместимости всех API или завершения стадии.

## v0.181 — borrowed String receiver / преобразование receiver String

Loaded String methods use AVM1 string coercion for non-boxed receivers instead
of host primitive/result conversion. Nonstring toString results use type labels
without valueOf fallback. Typed String and unloaded paths remain unchanged.
[RU/EN details](STRING_LEGACY_RECEIVER.md). SWF4..8 tests cover numeric/undefined/
Object/counted-NUL results, callback errors, exactly-once conversion, character
access/search/concat/split, post-GC casing, Function fallback and restored state.
Full method argument coercions, boxed overrides, separate function/clip versions
and all roadmap stages remain open.

Заимствованные строковые методы используют AVM1 string coercion без host
valueOf fallback; проверены пользовательские результаты и восстановление состояния.

Evidence for v0.181: AGG 342/Cairo 341 tests, five focused checks each in Linux
portable/restricted heap-global ASan+UBSan, sixteen byte-exact original SWFs.
Four AGG launch failures passed after restoring execute bits.

## v0.182 — signed String indices / знаковые индексы строк

Loaded character/portion/fromCharCode indices use AVM1 numeric coercion and
wrapping signed Int32. Substr uses a signed wrapping relative end; missing
portion arguments return undefined before receiver coercion. SWF5 nonnegative
out-of-range charCodeAt returns zero; negatives remain NaN. Host rules are
preserved. [RU/EN details](STRING_LEGACY_INDICES.md). Tests cover SWF4..8/host,
fractions/overflow/infinity, SWF6+ octal syntax, Object fallback, SWF5 boundaries,
negative substr count, empty arguments and restored state. Search/split argument
rules, boxed overrides and all roadmap stages remain open.
Evidence: AGG 343/Cairo 342 tests, six focused checks each in Linux portable and
restricted heap/global ASan+UBSan, sixteen byte-exact original SWFs. Five AGG
launch failures passed after restoring execute bits. The initial octal test
expectation was corrected to SWF6+; final fixtures pass in all four profiles.

Исправлены AVM1 ToInt32, исторический substr и charCodeAt SWF5; незагруженный
контракт сохранён, полнота String и завершение стадий не заявлены.

## v0.183 — String searches / поиск в строках

Loaded needle coercion and signed positions follow AVM1; missing needles return
undefined after receiver coercion. Undefined positions retain defaults, negative
reverse and too-large forward positions return -1, including empty needles.
NaN/infinities use zero rather than a reverse-end fallback. Host rules remain.
[RU/EN details](STRING_LEGACY_SEARCH.md). SWF4..8/host tests cover missing/undefined/
Object needles, receiver ordering/errors, exceptional/wrapping positions and
restored state. Split limits, boxed overrides and all roadmap stages remain open.
Evidence: AGG 344/Cairo 343 tests, seven focused checks each in Linux portable
and restricted heap/global ASan+UBSan, sixteen byte-exact original SWFs.

Исправлены преобразования и границы поиска; проверен вызов receiver даже при
отсутствии needle, включая ошибку пользовательской функции.

## v0.184 — loaded String.split / разделитель и limit

Loaded split uses signed AVM1 limits clamped at zero and versioned delimiter
coercion. SWF5 defaults missing delimiters to comma and treats empty delimiters
as whole-string requests. Absent-effective-delimiter output ignores limit.
Host rules remain unchanged. [RU/EN details](STRING_LEGACY_SPLIT.md).
SWF4..8/host tests cover default/empty/undefined delimiters, signed/wrapping limits,
boolean/Object coercions, limit-before-delimiter ordering/errors and restored
state. The 65536-part cap, boxed overrides and all roadmap stages remain open.
Evidence: AGG 345/Cairo 344 tests, eight focused checks each in Linux portable
and restricted heap/global ASan+UBSan, sixteen byte-exact original SWFs. Three
AGG launch failures passed after restoring execute bits.

Исправлены предел и исторические правила SWF5; преобразование delimiter не
пропускается при limit 0, включая пользовательские ошибки.

## v0.185 — String function/constructor/concat / преобразования String

Loaded String calls, construction and concat arguments share AVM1 string coercion
with SWF4 boolean and SWF4..6 undefined rules and Object type-label fallback.
Missing constructor/function arguments remain empty; concat captures the receiver
before converting arguments sequentially. Host behavior is preserved.
[RU/EN details](STRING_LEGACY_CONVERSION.md). SWF4..8/host tests cover all three
paths, undefined/boolean/Object, exactly-once callbacks, missing inputs, NUL results
after traced GC, callback errors and restored state. Boxed overrides, separate
function/clip versions and all roadmap stages remain open.
Evidence: AGG 346/Cairo 345 tests, eleven focused checks each in Linux portable
and restricted heap/global ASan+UBSan, sixteen byte-exact original SWFs. Two AGG
launch failures passed after restoring execute bits.

Функция, конструктор и concat используют единое AVM1 преобразование аргументов;
проверены результаты, callback ошибки и сохранение строк с NUL после GC.

## v0.186 — borrowed String value methods / valueOf и toString

Loaded String value methods return undefined for non-String receivers without
coercion. Boxed receivers return raw counted values regardless of instance method
overrides. Invalid owners/corrupt String state and unloaded guards are preserved.
[RU/EN details](STRING_LEGACY_VALUE.md). SWF4..8/host tests cover Object/Number/
Boolean/Array/Function receivers, trap conversions, both methods, overridden NUL
values, traced GC and restored state. Full boxed coercion/prototype/case rules
and all roadmap stages remain open.

Borrowed value methods для не-String больше не возвращают INVALID_ARGUMENT
в загруженном режиме; проверены отсутствие преобразований и raw boxed value.

Evidence for v0.186: AGG 347/Cairo 346 tests, twelve focused checks each in Linux
portable/restricted heap-global ASan+UBSan, sixteen byte-exact original SWFs.
Two AGG launch failures passed after restoring execute bits.

## v0.187 — own boxed String length / собственное length

Loaded String construction defines own writable UTF16-count length data with
DONT_ENUM | DONT_DELETE instead of a computed getter. Public length does not
alter stored text or methods; READ_ONLY protects the data. Unloaded construction
and existing graph/ABI layouts are preserved. [RU/EN details](STRING_LEGACY_LENGTH.md).
SWF4..8/host tests cover installation-before-load, flags, numeric/readonly/Object
writes, text independence, cycles/NUL through traced GC/channel transfer, aliasing,
nondelete protection and own deletion. Prototype length, host-before-load instances,
boxed overrides and all roadmap stages remain open.

Собственное length загруженного boxed String стало data property; проверены
защита flags и независимые текст/данные после GC и передачи.

Evidence for v0.187: AGG 348/Cairo 347 tests, fourteen focused checks each in Linux
portable/restricted heap-global ASan+UBSan, sixteen byte-exact original SWFs.
Four AGG launch failures passed after restoring execute bits.

## v0.188 — generic String prototype / обычный Object прототип

Loaded String prototypes are ordinary Objects without native text/length.
Untouched host prototypes transition on successful SWF loading after allocating
lookups, before allocation-free SWF publication. Identity/user properties persist;
invalid loads and explicitly replaced/customized length shapes are preserved.
Validation accepts both layouts; wire/ABI versions are unchanged.
[RU/EN details](STRING_LEGACY_PROTOTYPE.md). SWF4..8 tests cover installation
order, invalid SWFs, identity/NUL marker, missing length/value methods/casing,
GC/channel/GC, restored constructor alias and String construction. Customized
overrides, pre-load host instances, property case and all stages remain open.

Исправлен native тип String.prototype и переход стандартного host прототипа
при загрузке SWF; сохранены identity, свойства и alias восстановленного класса.

Evidence for v0.188: AGG 349/Cairo 348 tests, seventeen focused checks each in Linux
portable/restricted heap-global ASan+UBSan, sixteen byte-exact original SWFs.
Three AGG launch failures passed after restoring execute bits.

## v0.189 — String load allocation failures / отказы памяти при загрузке

No VM implementation changes. The new allocation test sweeps C++ new/new[]
failures through SWF decoding and String prototype activation, preserving old
state on OOM, then retrying successfully. Identity/NUL marker, traced GC,
validation/root/depth state and balanced C++ allocations pass in AGG, Cairo,
Linux portable and restricted heap/global ASan+UBSan profiles. Last full VM
validation remains v0.188: AGG 349/Cairo 348 and sixteen original SWFs. C malloc
failure coverage and all roadmap stage completion states remain open.
[RU/EN details](STRING_LEGACY_PROTOTYPE.md).

Проверены границы C++ allocation, отсутствие частичной активации при OOM и
успешный retry того же контекста. Код VM не изменялся; все стадии открыты.

## v0.190 — borrowed numeric value methods / Number и Boolean

Loaded numeric valueOf/toString return undefined for wrong native receiver types
without evaluating receiver conversions or Number radix. Typed stored data,
invalid-owner/corrupt-state checks and unloaded guards are preserved.
[RU/EN details](NUMERIC_LEGACY_VALUE.md). Tests cover SWF4..8/host multiple receiver
types, trap conversion/radix methods, both projections, valid overridden values,
traced GC and restored state. Prototype layouts, full radix semantics, separate
function/clip versions and all roadmap stages remain open.

Числовые borrowed value methods не преобразуют receiver другого типа;
проверены undefined, отсутствие вызовов radix и сохранение boxed data.

Evidence for v0.190: AGG 351/Cairo 350 tests, six focused checks each in Linux
portable/restricted heap-global ASan+UBSan, sixteen byte-exact original SWFs.
Three AGG launch failures passed after restoring execute bits.

## v0.191 — Number radix selection / выбор основания

Loaded Number.toString uses AVM1 radix coercion, truncates valid bases and falls
back to decimal for invalid values without wrapping. Unloaded guards remain.
[RU/EN details](NUMBER_LEGACY_RADIX.md). Tests cover SWF4–8/host, invalid and
fractional bases, octal strings, callback counts/errors and restored state.
Nondecimal formatting limitations and all roadmap stages remain open.

В загруженном SWF исправлены выбор десятичной системы при недопустимом radix
и version-dependent преобразование аргумента; ошибки callback сохраняются.

Evidence: AGG 352/Cairo 351 full tests; three focused checks each in Linux
portable and restricted heap/global ASan+UBSan (no stack/leak coverage);
sixteen byte-exact original SWFs. No launch failures in these runs.

## v0.192 — Boolean versioned conversion / преобразования по версии

Boolean function/constructor and AVM1 branches share truth conversion: numeric
strings in SWF4–6, nonempty strings in SWF7–8, objects true without callbacks.
Loaded Boolean() returns undefined; new Boolean() stores false. Host behavior
is preserved. [RU/EN details](BOOLEAN_LEGACY_CONVERSION.md). Tests cover missing
arguments, strings, null/undefined, NaN/Infinity, trap object conversions and GC.
Prototype layouts, clip/function versions and all roadmap stages remain open.

Исправлены version-dependent строки и различие function/constructor без
аргументов; правила truth conversion объединены с интерпретатором.

Evidence: AGG 353/Cairo 352 full tests, six focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Four AGG launch failures passed after restoring execute bits.

## v0.193 — Number function/constructor / числовые преобразования

Loaded Number() and new Number() use the shared AVM1 numeric conversion rather
than host fallback. Missing/undefined arguments, octal strings and object-valued
valueOf results follow loaded versions; callback errors propagate. Host behavior
is preserved. [RU/EN details](NUMBER_LEGACY_CONVERSION.md). Tests cover both entry
points, SWF4–8/host, callback counts/errors, traced GC and restored state.
Prototype layouts, function/clip versions and all roadmap stages remain open.

Исправлены Number function/constructor: общий AVM1 helper сохраняет различия
SWF4–8 и не запускает лишний host toString после object-valued valueOf.

Evidence: AGG 354/Cairo 353 full tests, five focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Four AGG and one Cairo launch failures passed after restoring
execute bits.

## v0.194 — Math operands and arity / аргументы Math

Loaded Math methods coerce the first two arguments in order even for unary
operations/random, ignore later arguments and apply SWF4–6 arity checks.
min/max use two operands; numeric coercion is versioned. Host Math and separate
global predicates retain their previous behavior. [RU/EN details](MATH_LEGACY_ARGUMENTS.md).
Tests cover callback order/counts/errors, unchanged random state after failure,
ignored third operands, min/max, missing/undefined operands and octal strings.
Random distribution, exact numeric rounding, metadata and all stages remain open.

Исправлены первые два аргумента, отсутствие лишних callbacks, min/max и arity
старых SWF. Ошибка random сохраняет предыдущее состояние генератора.

Evidence: AGG 355/Cairo 354 full tests, four focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Three AGG and three Cairo launch failures passed after restoring
execute bits. The final expanded Math fixture passed separately in all profiles.

## v0.195 — numeric predicates / isNaN и isFinite

Loaded isNaN/isFinite coerce supplied arguments with AVM1 rules, preserving
versioned undefined/object-valued valueOf results without host string fallback.
Missing-argument results and unloaded host behavior remain unchanged. Callback
errors propagate. [RU/EN details](NUMERIC_LEGACY_PREDICATES.md). Tests cover both
functions, SWF4–8/host, missing/undefined operands, callbacks/errors and restored
state. Property metadata, function/clip versions and all stages remain open.

Исправлены преобразования переданного аргумента isNaN/isFinite; отсутствие
аргумента сохраняет отдельное поведение, callback errors не теряются.

Evidence: AGG 356/Cairo 355 full tests, five focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Four AGG launch failures passed after restoring execute bits.

## v0.196 — ordinary numeric prototypes / прототипы без boxed data

Loaded Number/Boolean prototypes are ordinary objects; value methods return
undefined. Installation after load creates that layout; load after installation
preserves identity/properties and atomically activates unchanged host layouts.
Number/Boolean preparation precedes String commit, leaving no allocating step
after any mutation. Validation accepts both layouts. Graph/API versions remain.
[RU/EN details](NUMERIC_LEGACY_PROTOTYPE.md). Tests cover installation order,
invalid loads, old instances, NUL marker, GC/channel/GC, constructor aliases and
joint allocation-failure recovery/balanced allocations. Array/other prototypes,
customized native layouts, C malloc failures and all stages remain open.

Исправлены Number.prototype/Boolean.prototype и совместная активация примитивных
прототипов без частичных изменений при OOM; boxed экземпляры сохраняются.

Evidence: AGG 357/Cairo 356 full tests, five focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. No launch failures in the full runs.

## v0.197 — ordinary Array prototype / прототип Array

Loaded Array.prototype is ordinary Object without native length. Both install
orders preserve identity/methods/properties and the cached result prototype;
existing Arrays remain native. Generic push creates ordinary writable length.
Array preparation joins the atomic primitive activation, with retained keys
for allocation-free commit. [RU/EN details](ARRAY_LEGACY_PROTOTYPE.md). Tests cover
invalid loads, NUL marker, old instances, slice results, GC/channel/GC, restored
class construction and joint OOM recovery/allocation balance. Customized layouts,
other prototypes, C malloc failures and all roadmap stages remain open.

Исправлены Array.prototype и его совместная активация без native length;
проверены методы generic объекта и прототипы результата slice.

Evidence: AGG 358/Cairo 357 full tests, thirty-two focused checks each in Linux
portable and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen
byte-exact original SWFs. One AGG and two Cairo launch failures passed after
restoring execute bits.

## v0.198 — ordinary Date prototype and method arguments / Date

Loaded Date prototype is ordinary Object without timestamp, with both installation
orders and atomic joint activation. Instance methods coerce at most seven operands
before checking the receiver, stopping at undefined; other native types return
undefined and errors propagate. Existing timestamp engine receives numeric
projections, retaining unloaded behavior and callback/wire IDs.
[RU/EN details](DATE_LEGACY_PROTOTYPE.md). Tests cover old instances, wrong/valid
receivers, callbacks/limits/errors, preserved timestamp after failed setTime,
versioned strings, GC/channel/GC, class aliases and joint OOM recovery.
Constructor/UTC coercion, Date() global semantics, calendar fidelity, customized
layouts and all roadmap stages remain open.

Исправлены Date.prototype и порядок аргументов instance methods; getter также
преобразует аргументы, ошибка setter сохраняет старое timestamp.

Evidence: AGG 359/Cairo 358 full tests, five focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Two Cairo launch failures passed after restoring execute bits.

## v0.199 — Date constructor/UTC operands / конструктор и UTC

Loaded Date construction/UTC share the seven-value AVM1 operand prefix, stopping
at undefined and avoiding repeat callbacks. UTC with fewer than two projected
operands returns undefined; empty construction selects current time. Callback
errors propagate. Host behavior and callback/wire IDs remain unchanged.
[RU/EN details](DATE_LEGACY_CONVERSION.md). Tests cover both paths, cutoff/counts,
numeric references, missing/undefined/ignored errors, octal timestamps, current
time selection, errors and GC/channel restored class calls/construction.
Date() global semantics, precise time/calendar behavior and all stages remain open.

Конструктор Date и UTC приведены к общей обработке аргументов; восстановленный
класс сохраняет distinct вызов UTC и создание native экземпляра.

Evidence: AGG 360/Cairo 359 full tests, five focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Three AGG launch failures passed after restoring execute bits.

## v0.200 — Date function vs constructor / обычный вызов Date

Loaded Date() returns local-time text and ignores arguments; construction remains
separate. Unloaded host fallback is retained. Staged function registration selects
the class invocable callback separately; new stable callback 752 transports the
captured class while 751 remains construction. Graph/API formats are unchanged.
[RU/EN details](DATE_LEGACY_CALL.md). Tests cover host/SWF4–8, trap arguments,
direct class invocation, AVM1 CallFunction/NewObject, retained strings after
traced GC and GC/channel restored call/construct paths. Exact text/calendar/
clock fidelity, remaining Date semantics and all stages remain open.

Исправлен Date() в загруженном SWF: строка текущего времени без coercion;
проверено отдельное создание native Date и перенос обоих callback.

Evidence: AGG 361/Cairo 360 full tests, six focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. One AGG launch failure passed after restoring execute bits.

## v0.201 — fractional Date state and text / дробные Date

Loaded single-timestamp construction retains finite bounded fractions; setTime
floors negative/positive fractions after its range guard. Loaded text uses an
unpadded day. Host truncation/spelling remain. Graph decoder accepts bounded
fractional Date states while retaining NaN/type/range validation and wire layout.
[RU/EN details](DATE_LEGACY_FRACTIONAL.md). Tests cover constructor/setter differences,
GC/channel preservation, local day spelling, nonfinite/range setters, CRC-valid
invalid Date records with rollback and wrong-state encoder rejection. Constructor
unbounded state, calendar/version fields, exact Player comparison and all stages
remain open.

Исправлены дробный bounded timestamp, отрицательное округление setTime и день
в строке Date; перенос не отклоняет допустимое дробное native состояние.

Evidence: AGG 362/Cairo 361 full tests, ten focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Two AGG and two Cairo launch failures passed after restoring
execute bits. Expanded invalid-Date fixture passed separately in all profiles.

## v0.202 — fractional Date field projections / поля Date

Loaded millisecond getters truncate signed remainders before normalization;
hours use the half-millisecond boundary projection, shared with local Date text.
Timestamp data and unloaded behavior remain unchanged. [RU/EN details](DATE_LEGACY_FIELDS.md).
Tests cover SWF4–8 exact UTC fractions/hour/day boundaries, positive zero,
integer local milliseconds and hour/text consistency. Original Date SWF has
header 17 and is excluded unchanged from the SWF≤8 corpus; source and its 5.5-ms
output support the projection rules. Actual Player boundary verification,
calendar setters/version arithmetic, timezone fidelity and all stages stay open.

Исправлены целые milliseconds и историческая граница hours; первоначальное
ожидание равенства local/UTC milliseconds заменено корректной проверкой диапазона.

Evidence: AGG 363/Cairo 362 full tests, eight focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. No launch failures in the full runs.

## v0.203 — Date property attributes / атрибуты Date

Loaded Date prototype methods use flags 3; UTC uses readonly/nondelete flags 7.
Shared installation supplies only missing method flags, preserving module-staged
metadata. Loading after installation prepares Date attribute snapshots before
any primitive mutation; nondefault custom flags/descriptors/replaced values
remain. Commit swaps maps without allocation. [RU/EN details](DATE_LEGACY_ATTRIBUTES.md).
Tests cover installation order, host defaults, custom flags, writable prototype
methods/delete guards, UTC writes/deletes/calls, GC/channel and OOM preserving
old flags before retry. Other builtin metadata and all stages remain open.

Исправлены Date flags и общий этап, перезаписывавший staged metadata. Уточнён
host default UTC: 1. Проверены snapshot commit и сохранение пользовательских flags.

Evidence: AGG 364/Cairo 363 full tests, nine focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Three AGG launch failures passed after restoring execute bits.

## v0.204 — numeric method/constant flags / Number и Boolean flags

Loaded Number/Boolean methods use flags 3; canonical Number constants use
readonly/nondelete flags 7. Both installation orders stage flags atomically;
nondefault attributes/descriptors and changed constants remain. Commit swaps
prepared maps without allocating. [RU/EN details](NUMERIC_LEGACY_ATTRIBUTES.md).
Tests cover host defaults, writable protected methods, five constants, NaN
write/delete guards, changed MAX_VALUE, nondefault MIN_VALUE flags, GC/channel
and OOM preserving old flags before retry. Other metadata and all stages stay open.

Исправлены Number/Boolean metadata и canonical constants. No-op установка
нулевого default не считается отличающимся пользовательским состоянием.

Evidence: AGG 365/Cairo 364 full tests, ten focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Two AGG and two Cairo launch failures passed after restoring
execute bits.

## v0.205 — base/String native method flags / Object, Array, String

Loaded Object/Array/String methods use flags 3; registerClass uses 7 and writable
fromCharCode uses 3. Both installation orders prepare defaults atomically,
preserving changed flags/descriptors/data; Object commit swaps attributes and
Array/String retain ordinary-layout activation. [RU/EN details](BASE_LEGACY_ATTRIBUTES.md).
Tests cover writes/delete guards/static protection, installation order/custom
flags, GC/channel and joint OOM preserving old base/String flags before retry.
Version availability, raw __proto__, other metadata and all stages remain open.

Исправлены attributes уже установленных methods и static entries без изменения
availability по версии; shared helper меняет только canonical default slots.

Evidence: AGG 366/Cairo 365 full tests, thirty-seven focused checks each in Linux
portable and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen
byte-exact original SWFs. Two Cairo focused and one Cairo full launch failures
passed after restoring execute bits.

## v0.206 — Object locale projection / Object.toLocaleString

Loaded toLocaleString projects its receiver through AVM1 instead of the old
ordinary-string alias; operands are ignored. Selected toString runs once without
operands, counted strings/type labels/errors follow coercion and recursion
restores state. Borrowed Object.toString recognizes functions. Callback 627 is
new; legacy 611/612 retain the ordinary string callback. [RU/EN details](OBJECT_LEGACY_LOCALE.md).
Tests cover host/SWF4–8, NUL, nonstring returns, errors, noncallable method data,
function receiver, GC/graph callback restoration, recursion and legacy IDs.
Raw __proto__, case/version availability and all stages remain open.

Исправлен существовавший alias locale method, а не добавлен отсутствующий API;
проверены coercion, исключения и bounded recursion без утраты callback identity.

Evidence: AGG 367/Cairo 366 full tests, six focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Five AGG and seven Cairo launch/permission failures (including
two dependent process checks) passed after restoring execute bits.

## v0.207 — Array locale inheritance / locale method Array

Loaded Array prototype inherits Object locale projection rather than an own join
alias; call operands are ignored and current toString handles receiver conversion.
Canonical host alias/cache removal is staged before mutation; retained join
Function references stay callable and custom shapes remain. Prototype-free
results use locale projection directly. [RU/EN details](ARRAY_LEGACY_LOCALE.md).
Tests cover own shape, cache removal, old references, ignored separators, NUL/
nonstring/error callbacks, GC/channel, bare results, custom flags/data and joint
OOM alias preservation/retry. Raw __proto__, case/version visibility and all
roadmap stages remain open.

Исправлено наследование Array.toLocaleString; сохранены host alias и явные
пользовательские изменения, а кеш штатного alias удаляется без allocation.

Evidence: AGG 368/Cairo 367 full tests, seven focused checks each in Linux portable
and restricted heap/global ASan+UBSan (no stack/leak coverage), sixteen byte-exact
original SWFs. Expanded custom-shape fixture passed separately in all profiles;
no launch failures in the full runs.


## v0.232 — blank class sprites and super text

Class-only DefineSprite streams may declare blank frames while containing only
End tags. Their declared duration is retained through GC and graph transfer;
partially populated timelines still reject mismatched frame counts. Callable
super facades use Object text instead of Function text when Object.toString
formats them. The unchanged pinned function_suppress_and_preload SWF now passes
with byte-exact output, zero exit and empty stderr: selected corpus 18/22.
Remaining probes: object_prototypes, this_swf5/6/7 (ActionWith unsupported).
Linux AGG and Cairo each pass 433 tests after retrying environment execute-bit
launch failures; five focused heap/global ASan+UBSan checks pass. Stack and leak
instrumentation remain unavailable in this environment. All roadmap stages
remain open; JIT and non-Linux execution are not claimed.


## v0.237 — current AVM1 checkpoints

| Part | Status | Evidence and limits |
| --- | --- | --- |
| Loaded named-call this and intrinsic prototype edges | Tested | AVM1_THIS_BINDING.md, OBJECT_RAW_PROTOTYPE.md; general property casing remains open. |
| Retained With scopes and definition clips | Tested | AVM1_WITH_SCOPE.md, AVM1_FUNCTION_BASE_CLIP.md; removed/readded clip references remain open. |
| Missing register destinations | Tested in focused fixtures | AVM1_REGISTER_MISSES.md; global-register persistence/private count contract remain open. |
| Original SWF corpus | Tested for 23 cases through v0.236 | Unmodified SWF<=8 files and byte-exact trace comparison; not complete AVM1 coverage. |
| JIT and cross-platform acceptance | Open | Common C execution services/native-body ABI exist; all requested backends and non-Linux runtime acceptance remain open. |

Все девять стадий остаются открытыми. Эти контрольные точки уточняют проверенные
части, но не заменяют полную приёмку роудмапа.


## v0.238 — context-global register values

Loaded legacy and zero-private-bank functions now share context-owned registers,
including prepared native bodies. AVM1_GLOBAL_REGISTERS.md documents C89 value
access, owned strings, GC roots and context isolation. Private count boundaries
and broader historical register semantics remain open.

Общие регистры сохраняются между активациями и доступны через C-слой. Данные
принадлежат контексту; GC и отказы памяти проверяются вместе с новой службой.
Полная семантика частных наборов и общая приёмка роудмапа остаются открытыми.


## v0.239 — loaded private register span

Loaded Function2 now allocates its declared count exactly, including slot zero.
AVM1_PRIVATE_REGISTER_COUNT.md records boundary fixtures and the distinction
between private misses and zero-bank global fallback. Older statements about an
open private-count contract are superseded for these checked loaded cases.

Границы частного набора проверяются на числе ячеек 0, 1, 2 и 255. Это уточнение
проверенных регистровых правил, а не завершение стадии AVM1.

## v0.240 — shared method invocation service

CallMethod/NewMethod now share a value-based C89 service with future generated
bodies. See [EXECUTION_CALL_METHOD.md](EXECUTION_CALL_METHOD.md) for dispatch,
GC, allocation checks and retained compatibility limitations.

Вызов метода и создание через метод вынесены в общий C-слой. Все девять
стадий роудмапа остаются открытыми.

## v0.241 — loaded method-name values

Loaded method names use version-aware string coercion and distinguish null from
undefined. Nullish NewMethod receivers and nonobject constructor members return
undefined. [AVM1_METHOD_NAME_VALUES.md](AVM1_METHOD_NAME_VALUES.md) records
callback/GC/exception checks and remaining dispatch/count limitations.

Проверены значения имён методов для SWF4–8 и исполнение преобразования имени
через подготовленное нативное тело; общая совместимость AVM1 остаётся открытой.

## v0.242 — deletion, legacy comparison and logical C services

Interpreter and future generated bodies share value-based deletion, old numeric
comparison and logical operations. See [EXECUTION_VALUE_OPERATIONS.md](EXECUTION_VALUE_OPERATIONS.md)
for conversion order, roots, C89 checks and remaining scope/casing limitations.

Крупные ветви удалены из основного цикла; байткод и стек остаются в адаптере,
а семантика доступна через общий C-интерфейс. Девять стадий остаются открытыми.

## v0.243 — coercible method argument counts

CallMethod/NewMethod use a rooted C89 count service, unsigned truncation/wrapping
and available-value clipping. [AVM1_METHOD_ARGUMENT_COUNT.md](AVM1_METHOD_ARGUMENT_COUNT.md)
records bounds, callback/failure checks and the still-open other call variants.

Для SWF4–8 проверяется преобразование счётчика и нехватка значений без чрезмерного
выделения памяти. Это уточнение вызовов методов, не завершение стадии AVM1.

## v0.244 — named call/constructor counts

CallFunction and loaded NewObject share the rooted C count-conversion service.
[AVM1_NAMED_ARGUMENT_COUNT.md](AVM1_NAMED_ARGUMENT_COUNT.md) records checked
SWF4–8 cases, retained unloaded-host bounds and open name/dispatch policies.

Подключены оставшиеся именованные виды вызовов к преобразованию счётчика;
это проверенная часть стадии 2, не закрытие общей приёмки.

## v0.245 — shared string coercion and named-call names

Loaded named calls/constructors, method names and ToString share the C89
string-conversion service. [AVM1_NAMED_NAME_VALUES.md](AVM1_NAMED_NAME_VALUES.md)
records callback order, result ownership, GC/exception checks and open lookup policy.

Преобразование имени выполняется до счётчика, а полученная строка удерживается
до дальнейших callbacks. Это частичная проверка стадии 2; JIT ещё отсутствует.

## v0.246 — resolved named-call C dispatch

CallFunction/NewObject share a C89 dispatcher taking resolved values and binding
presence. [EXECUTION_RESOLVED_CALL.md](EXECUTION_RESOLVED_CALL.md) records roots,
constructor bounds, fallback checks and unchanged compatibility limitations.

Поиск области видимости отделён от общего вызова; стек/байткод не входят в ABI.

## v0.247 — source archive reproducibility

A reusable source/history export script replaces host-state-dependent ad hoc ZIP
packing. See [SOURCE_ARCHIVE.md](SOURCE_ARCHIVE.md) for reproducibility evidence,
reachable-history scope and remaining cross-toolchain/platform limits. Runtime
compatibility and all nine roadmap stages remain at their open v0.246 status.

## v0.248 — encoded operand/framing macro names

[ENCODED_OPERAND_CONSTANTS.md](ENCODED_OPERAND_CONSTANTS.md) records documented
uppercase widths/sign encodings/SWF framing constants replacing numeric literals.
Runtime compatibility and all roadmap-stage acceptance states remain unchanged.

## v0.249 — archive Git object-format configuration

The source exporter preserves SHA-1/SHA-256 repository configuration. Two-export
identity, extracted HEAD/object format and fsck checks cover the SHA-256 correction;
project SHA-1 exports retain reproducibility. Runtime evidence remains v0.248:
454 AGG/Cairo tests per profile, 23 original traces per profile, seven focused
heap/global ASan+UBSan checks. No roadmap stage is closed by this packaging fix.

## v0.250 — unqualified Delete2 and C scope deletion

Previously unsupported Delete2 now handles simple scoped names and Boolean
results. [AVM1_DELETE2.md](AVM1_DELETE2.md) records getter-free shadowing,
GC/allocation/exception checks and the still-unsupported qualified path cases.

Добавлена проверенная часть удаления переменных; все стадии остаются открытыми.

## v0.251 — qualified Delete2 and rooted script getters

Delete2 handles dot/colon object-member chains through a C89 value service.
[AVM1_DELETE2_PATH.md](AVM1_DELETE2_PATH.md) records traversal roots, trusted getter
GC entry, preserved host barriers and still-open display-path/scope retry cases.

Добавлены составные имена и корни GC при обходе; полная совместимость путей
и все стадии роудмапа остаются открытыми.

Expanded reference evidence at v0.251: 24/25 original SWF<=8 probes match.
The delete SWF7 uses its upstream num_frames=3 policy. The remaining
prototype_delete SWF8 mismatch is MovieClip.prototype string projection and stays
open; no file/header/oracle is changed to bypass it. Upstream delete2 SWF16 and
global_proto_decls_delete SWF15 are preserved but outside the accepted version set.

## v0.252 — MovieClip class-prototype string projection

The retained builtin prototype follows Object text/custom toString policy while
real clips remain display references. [MOVIECLIP_CLASS_PROTOTYPE_TEXT.md](MOVIECLIP_CLASS_PROTOTYPE_TEXT.md)
records marker ownership, GC/channel checks and unchanged/open type/display policy.
The earlier prototype_delete trace mismatch is addressed without modifying its oracle.

Current expanded oracle checkpoint: 25/25 unchanged SWF<=8 probes match on both
AGG and Cairo. The new delete probe uses three ticks as declared by its upstream
configuration; prototype_delete now has exact text, zero exit and empty stderr.

## v0.253 — trusted script setter collector entry

Prepared native script setters may collect under the known rooted addProperty
bridge. [SCRIPT_SETTER_COLLECTOR.md](SCRIPT_SETTER_COLLECTOR.md) records copied
arguments, descriptor/owner retention, preserved C barriers and fault restoration.

Проверена ещё одна часть корней GC и повторного использования нативных тел;
это не завершение общей приёмки GC, JIT или платформ.

## v0.254 — primitive conversion C services

Number/Integer, Boolean and TypeOf handlers expose common C89 value interfaces.
[EXECUTION_CONVERSIONS.md](EXECUTION_CONVERSIONS.md) records version policies,
integer zero/wrapping, callback roots and retained type-classification limitations.

Интерпретатор и будущий JIT используют одни преобразования; это часть
декомпозиции, а не реализация генератора машинного кода.

### v0.255 — explicit-base display deletion

[`fvm_exec_delete_path`](../include/flashvm/execution.h#L371) resolves slash-containing prefixes through the shared
MovieClip resolver and deletes a counted final member without reading it.
[DELETE_DISPLAY_PATH.md](DELETE_DISPLAY_PATH.md) records tests and the remaining
interpreter lexical-scope/path-selection boundary. Display resolver constants are
named/documented. Общая служба расширена; полная семантика Delete2 ещё открыта.

### v0.256 — scoped Delete2 display paths

[SCOPED_TARGET_PATHS.md](SCOPED_TARGET_PATHS.md) records a common C89 complete-path
retry service and interpreter integration for slash-containing Delete2 prefixes.
Explicit display root/this inputs preserve target/receiver independence and roots.
This supersedes v0.255's slash-prefix rejection in Delete2. Non-slash Delete2 and
general GetVariable/SetVariable retain documented limitations. Поиск внешних
областей подключён для этого подмножества путей; полной совместимости ещё нет.

### v0.257 — shared Delete2 and final-member presence

[DELETE_REFERENCE.md](DELETE_REFERENCE.md) documents the single C89 Delete2
handler and scoped-member read service. Presence is distinct from undefined/null/
primitive values, preventing an incorrect outer deletion. Versioned two-step
reference parsing supersedes v0.256's broader direct-path interpretation; SWF4
literal dotted prefixes and SWF5+ slash cases have explicit fixtures. General
variable lookup and historical casing/this-cell rules remain open. Обработчик
вынесен из цикла; полная совместимость AVM1 и готовность JIT не заявляются.

### v0.258 — shared GetVariable path phase

[VARIABLE_PATH_PHASE.md](VARIABLE_PATH_PHASE.md) records C89 qualified/slash
reference resolution shared by counted interpreter reads and Delete2 owner reads.
Simple/literal fallback preserves previous binding lookup; presence, aliasing,
versioned separators, GC and allocation behavior have focused fixtures. Common
syntax/bounds use FVM_REFERENCE_PATH_* macros. SetVariable/named CallFunction and
full legacy case semantics remain open. Общий разбор подключён к чтению, но вся
семантика областей/переменных и стадии 1–9 ещё не завершены.

### v0.259 — getter-free shared variable assignment

[VARIABLE_ASSIGNMENT.md](VARIABLE_ASSIGNMENT.md) documents C89 SetVariable
assignment, managed lexical binding selection without old-value getters, scoped
qualified writes and rooted setter callbacks. Bare slash names remain literal;
empty names are ignored. Version/path/With/setter/GC/OOM fixtures accompany it.
Implicit-this mutation, full legacy casing/removed-target behavior and complete
AVM1 conformance remain open. Присваивание вынесено из цикла; стадии 1–9 открыты.

### v0.260 — plain variable lookup and captured With receiver

[VARIABLE_LOOKUP.md](VARIABLE_LOOKUP.md) documents the C89 plain-name/read receiver
service and compatible scope lookup facade. Capturing With class before getters
fixes receiver selection after scope-chain detachment and GC; direct C and nested
interpreter fixtures cover it. Versioned this spelling, undefined presence and
error/OOM root restoration are exercised. Full this-cell/casing/qualified-call and
removed-target equivalence remain open. Основной поиск вынесен из интерпретатора;
все девять стадий остаются открытыми.

### v0.261 — persistent loaded catch assignment

[LOADED_CATCH_ASSIGNMENT.md](LOADED_CATCH_ASSIGNMENT.md) records loaded named
catch binding through common variable writes, persistent local/With/global and
qualified assignments, rooted setter GC and binding-error propagation. Host-mode
temporary restoration remains separate. The external corpus gains unchanged SWF8
catch_references_registers; SWF15 Try originals stay excluded. Source-based
fixtures do not claim full Flash conformance. this-cell/encoding/base-target and
all roadmap gates remain open. Присваивание catch уточнено по версии исполнения.

### v0.262 — shared declarations and With-aware function publication

[VARIABLE_DECLARATIONS.md](VARIABLE_DECLARATIONS.md) records C89 initialized/
uninitialized declaration and named function binding over managed activation inputs.
Own/inherited With distinctions, top-level qualified names, existing undefined,
rooted coercion/new-function setters and OOM publication are exercised. Host own-
data declarations remain separate. Complete local prototype/casing, cached named-
call fallback and qualified-call receivers remain open. Объявления вынесены в
общую службу; полный AVM1, JIT и все стадии роудмапа ещё не завершены.

### v0.263 — scoped named calls and defining-method ownership

[CALL_FUNCTION_PATHS.md](CALL_FUNCTION_PATHS.md) records the common C89 named-call
service, qualified receiver/prototype selection and loaded undefined results for
missing or noncallable names. Deleted script-cache entries no longer resurrect
functions in loaded calls. Host fallback remains compatible. Direct C, interpreter,
GC, exception, inherited-super and allocator fixtures cover the implemented paths.
Full virtual-super lookup, metatable/scope equivalence and all roadmap stages
remain open. Вызовы по составным именам вынесены в общий C-слой.

### v0.264 — scoped literal construction and prototype-getter root

[NEW_OBJECT_NAMES.md](NEW_OBJECT_NAMES.md) records the common C89 named constructor
service, literal loaded scope lookup and undefined nonconstructor results. Host
qualified/native registry behavior remains separate. A reproduced ASan use-after-
free during a script constructor's prototype getter is fixed by retaining the
fresh instance until invocation. C89/interpreter, GC, thrown-body and allocator
fixtures exercise the supported branches. Full boxing/scope/encoding and all
roadmap gates remain open. Конструктор и промежуточный экземпляр защищены от GC.

### v0.265 — common string length and character services

[STRING_CHARACTER_SERVICES.md](STRING_CHARACTER_SERVICES.md) records three C89
unary services and their ordinary/multibyte action adapters. NUL/surrogate/Uint16
behavior, aliases, coercion-GC/exception and allocation restoration are exercised;
root layouts, substring origin and Unicode bounds use documented macros. The
loaded SWF4–5 ASCII-only restriction is preserved: historical byte/locale details
remain open. Candidate string_ops_swf6 is actually SWF15 and is excluded without
header/oracle edits. Все стадии 1–9 остаются открытыми.

### v0.266 — named WTF-8/UTF-16 representation constants

[STRING_CODEC_CONSTANTS.md](STRING_CODEC_CONSTANTS.md) records documented sequence,
surrogate, continuation and payload macros in the shared codec, plus the borrowed
terminated-size marker. Representation/decoding/transport behavior is unchanged;
this is a readability checkpoint, not new compatibility coverage. Historical
encodings and all nine roadmap gates remain open. Константы вынесены в `.h`.

### v0.267 — shared StringAdd and explicit string comparisons

[STRING_BINARY_SERVICES.md](STRING_BINARY_SERVICES.md) records two C89 services
and four interpreter adapters, with separate input/text roots, exact counted-byte
concatenation and preserved comparison order. Direct C, traced-string GC, callback
exception, alias, UTF-16 order and allocator/depth-boundary fixtures accompany
the decomposition. Numeric/dynamic operators remain separate. Legacy locale
semantics and every roadmap completion gate stay open. Двухместные строковые
обработчики доступны через общий C-интерфейс.

### v0.268 — complete value-based GetVariable entry

[GET_VARIABLE_SERVICE.md](GET_VARIABLE_SERVICE.md) records loaded name coercion,
qualified/plain lookup and the SWF4 MovieClip result filter through a C89 service.
C/interpreter numeric and counted names, target-only roots through conversion GC,
exceptions and allocation restoration are covered. The generated SWF4 target-scope
fixture now uses explicit clip paths; modern this assertions and independent SWF
oracles remain intact. Other display kinds, Enumerate, full this/encoding semantics
and all roadmap stages remain open. GetVariable вынесен в отдельный C-обработчик.

### v0.269 — plain `_global` version and binding precedence

[GLOBAL_BINDING_LOOKUP.md](GLOBAL_BINDING_LOOKUP.md) records loaded SWF6+ namespace
fallback after ordinary bindings, undefined-presence shadowing, and versioned
case spelling. Host reserved-name priority remains compatible. C89/interpreter
fixtures cover namespace availability and target/global/lexical shadows. Qualified
path and Delete2 alias rules, the failing original swf4_vars probe, historical
system properties and all roadmap gates remain open. Уточнён простой поиск `_global`.

### v0.270 — concrete MovieClip namespace in target/With scopes

[MOVIECLIP_GLOBAL_SCOPE.md](MOVIECLIP_GLOBAL_SCOPE.md) refines namespace lookup:
loaded SWF6–8 concrete clips supply _global after own slots and before prototypes
or global scope. With scopes share the rule; class prototypes are excluded. The
v0.269 generated concrete-clip global-shadow assertion is corrected against the
primary display-object search order. C89, own/undefined/inherited, class/case and
allocator metadata fixtures cover the implemented scope paths. Direct GetMember,
qualified paths, Delete2 and all full roadmap gates remain open.

### v0.271 — shared member/path/Delete2 namespace policy

[GLOBAL_MEMBER_PATHS.md](GLOBAL_MEMBER_PATHS.md) records intrinsic MovieClip
namespace reads before prototypes, virtual-presence versus own-descriptor
separation, versioned qualified paths and Delete2 fallback. Explicit global starts
honor normal bindings; native MovieClip prototypes also participate. C89/action,
own/undefined/inherited, namespace-start and allocation fixtures accompany it.
Generated SWF5 namespace expectations are corrected; independent oracle data is
untouched. Ordinary property case folding, other display fields and all roadmap
gates remain open. Общие правила `_global` применены к членам и путям.

### v0.272 — ordinary SWF4 names and loaded root/parent intrinsics

[DISPLAY_PATH_BINDINGS.md](DISPLAY_PATH_BINDINGS.md) records ordinary timeline
this below SWF5, shared root/parent/global intrinsic lookup, own-slot precedence,
parentless undefined and private preload identity. Loaded execution bypasses the
host root/parent compatibility descriptors; custom accessors remain visible.
C89/action, legacy assigned-name, modern root-shadow and four-path allocator
fixtures cover the change. Other system fields, levels/casing, mutable this and
all roadmap gates remain open. Уточнены версии и собственные привязки путей.

### v0.273 — MovieClip limits, display indices and transport identities

[MOVIECLIP_SEMANTIC_CONSTANTS.md](MOVIECLIP_SEMANTIC_CONSTANTS.md) records named
frame/depth/action/tick/geometry bounds, units, property IDs, display roots and
stable callback transport IDs. Equal values retain separate semantic namespaces;
wire identities and existing bounds remain unchanged. Index coercion uses its
retained input snapshot. This readability checkpoint does not complete remaining
system/display fields, literal cleanup, compatibility or any roadmap stage.

### v0.274 — virtual path presence in With scopes

[VIRTUAL_PATH_SCOPES.md](VIRTUAL_PATH_SCOPES.md) records assignment receiver
selection, non-own deletion barriers and DefineLocal2 presence. Initialized
declarations retain own-only placement. C89 SWF4–8, actual With actions and
allocation failures cover the shared services. Removed clips, other system
fields, mutable this and all roadmap gates remain open. Неявные свойства
учитываются без вызова getter при выборе области видимости.

### v0.275 — canonical-name and ordering constants

[STRING_CODEC_CONSTANTS.md](STRING_CODEC_CONSTANTS.md) records shared semantic
UTF-16 constants for canonical property names and allocation-free comparison.
Existing representation and order remain unchanged; broad legacy string
conformance and all roadmap gates remain open. Макросы заменяют границы и маски.

### v0.276 — builtin String semantic constants

[STRING_CODEC_CONSTANTS.md](STRING_CODEC_CONSTANTS.md) records named String
wrap/encoding/casing bounds, split allocation limit and stable callback IDs.
Existing wire values and version rules remain unchanged. Full string/standard
object compatibility and all nine roadmap stages remain open.

### v0.277 — loaded SetVariable name conversion

[SET_VARIABLE_NAMES.md](SET_VARIABLE_NAMES.md) records version-aware name
conversion in the common C service, retained inputs through GC, counted NUL,
conversion exceptions and allocation failures. Real SWF4–8 action dispatch
accepts numeric names; unloaded host calls retain their String requirement.
Mutable this, removed clips and target mutation remain open. Преобразование
выполняется до записи; все стадии роудмапа остаются открытыми.

### v0.278 — qualified DefineLocal2 undefined results

[VARIABLE_DECLARATIONS.md](VARIABLE_DECLARATIONS.md) records value-based qualified
initialization checks, getter/setter dispatch, exceptions and SWF4 display-value
filtering. C89/action/allocator fixtures cover the change. The old generated
undefined-getter assertion is corrected against the pinned primary implementation.
Simple declarations remain presence-based. Все стадии роудмапа остаются открытыми.

### v0.279 — primitive receiver boxing for member access

[PRIMITIVE_BOXING.md](PRIMITIVE_BOXING.md) records C89 value-based object
conversion, loaded global constructors, temporary primitive/nullish member
receivers and prototype setter calls. GC, exceptions, metadata priority,
constructor ordering, action dispatch and allocation failures are covered.
Native raw-table keys retain their extension policy. With/call boxing and scalar
native-constructor results remain separate boundaries. Стадии 1–9 открыты.

### v0.280 — common primitive boxing in With

[PRIMITIVE_BOXING.md](PRIMITIVE_BOXING.md) records shared constructor selection
and bare wrapped objects when a loaded class is unavailable. Actual With actions,
outer-parent GC, constructor exceptions and allocation failures are covered.
Nullish body skipping and host constructor policy remain unchanged. Call boxing
and remaining scope/conformance gates remain open. Все стадии 1–9 открыты.

### v0.281 — single-read named method calls

[NAMED_MEMBER_CALLS.md](NAMED_MEMBER_CALLS.md) records rooted defining owners,
single getter evaluation, missing/noncallable undefined results and no stale
loaded script-cache fallback. Method boxing shares current global constructors;
invalid NewMethod constructors and anonymous noncallables yield undefined. C89,
action, detached-prototype GC, exception and allocation fixtures cover the change.
Super/casing and other conformance boundaries remain open. Все стадии открыты.

### v0.282 — shared CastOp and discarded primitive boxing

[CAST_SERVICE.md](CAST_SERVICE.md) records C89 casts, original-value preservation,
loaded primitive boxing side effects, constructor GC/exceptions and allocation
failures. The interpreter retains only operand adaptation. General constructor
and prototype coercions in InstanceOf remain open, alongside all nine stages.
Приведение примитива вызывает упаковку, но возвращает null.

### v0.283 — own-data InstanceOf and rooted class/prototype coercion

[INSTANCE_OF_SERVICE.md](INSTANCE_OF_SERVICE.md) records noncallable class objects,
raw own prototype lookup, versioned field spelling, both boxing phases and
separate interface edges. C89, action, GC/exception, three-phase allocator and
host-policy fixtures cover the change. Interface snapshots, full property folding,
super and removed clips remain open. Проверка не вызывает getter prototype.

### v0.284 — first-install interface snapshots and graph v9

[INTERFACE_SNAPSHOTS.md](INTERFACE_SNAPSHOTS.md) records SWF7–8 prototype capture,
first/empty-list retention, count conversion and version gating. Mode-aware GC
and graph identity preserve snapshots through memory/file/localhost network
channels. Graph v9 is emitted only for new snapshot state; ordinary graphs remain
byte-compatible v8 and schemas 1–8 still decode. Allocation rollback and malformed
flags are covered. Historical player/count differences and full roadmap gates
remain open. Снимки интерфейсов сохраняются при смене поля конструктора и переносе.

### v0.285 — value-based Extends

[EXTENDS_SERVICE.md](EXTENDS_SERVICE.md) records ordered primitive boxing,
ordinary superclass prototype getters, fresh own metadata and final subclass
setters. Direct C and Action fixtures cover SWF4–8; GC, exception and allocation
fixtures cover intermediate lifetimes. Host strict class policy is preserved.
Полная совместимость и все девять стадий остаются открытыми.

### v0.286 — named AVM1 operand layouts

Documented uppercase constants in operand_constants.h and literal_constants.h
replace literal widths/offsets for comparisons, arithmetic, members, deletion,
swap, URL operands, method calls and Array/Object literal roots. The signed
literal count limit now uses the existing shared Int32 bound. Values, action
semantics and independent raw fixtures are unchanged. Смысловые макросы разделяют
назначение одинаковых чисел; это улучшение читаемости, не закрытие стадии.

### v0.287 — shared Increment/Decrement

[NUMERIC_STEP_SERVICE.md](NUMERIC_STEP_SERVICE.md) records the common C89 numeric
step service, its versioned conversion, alias/error policy and documented unit.
Direct C, actual Action, collecting/throwing valueOf and allocation fixtures
exercise SWF4–8 behavior and root restoration. Existing numeric coercion policy
is preserved. Машинный JIT и все стадии приёмки остаются открытыми.

### v0.288 — documented coercion syntax constants

AVM1 coercion_constants.h names decimal/octal/hex radices, the hexadecimal letter
value and raw prefix width, the bounded exponent spelling buffer, counted method
names and each currently accepted leading whitespace code point. SWF5/6/7 gates
use existing version macros. The accepted set and conversion behavior are
unchanged; full historical Unicode/coercion equivalence remains open.
Числовые коды снабжены комментариями; разные назначения имеют отдельные имена.

### v0.289 — value-based enumeration and loaded sentinel policy

[ENUMERATION_SERVICE.md](ENUMERATION_SERVICE.md) records the shared C89 key
snapshot service, loaded nonobject/empty-stack behavior and undefined sentinel.
Enumerate stringifies its variable name once under roots; primitive enumeration
does not call boxing constructors. Direct C, Action/GC and allocation fixtures
cover SWF4–8, counted names and failure atomicity. Existing native key ordering
is preserved; historical ordering and removed clips remain open.
Примитивы больше не приводят к ошибке перечисления в загруженном SWF.

### v0.290 — RandomNumber Action and shared context RNG

[RANDOM_NUMBER_SERVICE.md](RANDOM_NUMBER_SERVICE.md) records the newly supported
Action, value-based C service, one-pass versioned bound conversion and saturating
Int32 limit. Math.random retains its existing sequence and shares the same state.
C89/Action/GC/exception/allocation fixtures cover SWF4–8; Math graph transport
resumes bounded draws correctly. Historical player RNG sequences and locale
behavior remain open. Добавлен отсутствовавший код Action, а не только адаптер.

### v0.291 — GetTime and separate monotonic adapters

[GET_TIME_SERVICE.md](GET_TIME_SERVICE.md) records the new Action and allocation-free
C89 elapsed-time service. Deterministic SWF4–8 fixtures cover origins, load retention,
fractional truncation, unsigned 32-bit wrap, rollback, read failure and recovery.
A real Linux adapter fixture and allocation/root checks accompany the change.
Date retains calendar time. Other target adapters await real builds and suspend/
resume behavior remains open. Добавлен отсутствовавший таймер, стадии не закрыты.

### v0.292 — TargetPath and bounded dispatch inventory

[TARGET_PATH_SERVICE.md](TARGET_PATH_SERVICE.md) records the new Action/C service,
rooted primitive boxing, native counted paths and failure/alias policy. Direct C,
Action/GC/exception and allocation fixtures cover SWF4–8. The separate
[dispatch inventory](AVM1_DISPATCH_INVENTORY.md) audits 100 pinned catalog entries:
92 source references and eight absent actions. Source presence is not semantic
certification. Другие display-типы, удалённые клипы и все стадии остаются открытыми.

### v0.293 — WaitForFrame and WaitForFrame2

[FRAME_AVAILABILITY_SERVICE.md](FRAME_AVAILABILITY_SERVICE.md) records the common
C89 availability service, distinct direct/value bounds, strict frame strings and
Int32 compatibility arithmetic. Actual Action tests cover whole-record skips,
variable payloads, nested bodies, terminal overshoot and invalid lengths. GC,
exception, alias/allocation and intrinsic-getter fixtures accompany SWF4–8.
The pinned dispatch catalog has 94 source references and six missing entries.
Current native timelines are fully loaded; partial streaming remains absent.
Проверка наличия ветки не закрывает семантическую приёмку или стадии роудмапа.

### v0.294 — atomic native MovieClip detachment

[MOVIECLIP_REMOVAL_ATOMICITY.md](MOVIECLIP_REMOVAL_ATOMICITY.md) records prepared-map
publication, counted current-name cleanup, missing-name safety and slot identity
checks. C89 host/SWF4–8 fixtures and allocation/GC tests cover the change while
preserving existing authored-depth native policy. Action RemoveSprite and full
rename/unload/removed-target semantics remain open. Схема переноса не меняется.

### v0.295 — counted clip replacement / Полные имена при замене клипа

Depth replacement and scene rename preserve embedded-NUL binding lengths and
unrelated prefix aliases; replacement tolerates a missing _name. Covered by
movieclip_remove_names and movieclip_remove_allocation, including host/SWF4–8
replacement and direct scene rename. See MOVIECLIP_REMOVAL_ATOMICITY.md.
Поиск связи при замене и переименовании больше не усекает имя по NUL.
Full rename semantics, RemoveSprite and all roadmap stages remain open.

### v0.297 — explicit forced clip detachment / Явное отсоединение клипа

The C89 [fvm_exec_detach_clip](../include/flashvm/execution.h#L237) service exposes native forced scene removal without
bytecode or script method lookup. Host/SWF4–8 and positive/negative-depth fixtures
cover override bypass, invalid objects and repeated calls; allocation injection
checks link/root preservation through this public entry. See CLIP_DETACH_SERVICE.md.
Принудительное отсоединение доступно из C; Action RemoveSprite, исторические
ограничения глубин, события и восстановление цели остаются открытыми.

### v0.298 — shared removal implementation / Декомпозиция отсоединения

The native callback and C service now share the single transaction in
movieclip_remove.cpp, using common raw-state/depth helpers in movieclip_storage.hpp.
No graph/callback identifiers, depth policies or Action coverage change.
Транзакция вынесена из основного MovieClip-файла; проверки состояния и глубин общие.
See CLIP_DETACH_SERVICE.md for scope and validation limits.

### v0.299 — named execution roots / Имена корней общих обработчиков

Nine remaining handlers use documented role/count macros; recursive equality has
its own root layout and named recursion bound. Call-frame's unused third root is
removed. Callback arities remain separate from GC sizes; C ABI and action coverage
are unchanged. See EXECUTION_ROOT_LAYOUTS.md for roles and validation boundaries.
Оставшиеся числовые индексы корней заменены смысловыми именами; все стадии открыты.

### v0.300 — missing unary AVM1 operands / Унарные операции на пустом стеке

Audited loaded unary operations now convert undefined instead of failing before
the shared C service. Raw-byte fixtures cover SWF4–8 and strict host execution;
allocation injection verifies root/execution restoration. See AVM1_UNARY_MISSING.md.
Загруженный SWF получает исторические преобразования undefined; другие случаи
недостатка операндов, полная семантика AVM1 и все стадии остаются открытыми.

### v0.301 — missing binary operands / Недостающие бинарные значения

Twenty-four audited loaded binary actions now supply undefined below existing
top operands, preserving right/left position and shared C coercion semantics.
Raw-byte fixtures cover zero/one values, SWF4–8, strict host mode, collecting
valueOf and allocation failures. See AVM1_BINARY_MISSING.md. Other stack policies
and all roadmap stages remain open. Порядок аргументов проверен отдельно.

### v0.302 — missing stack/control operands / Стек и управление без значения

Loaded Pop/Duplicate/Swap/Return/Throw/If/Trace now follow audited undefined
missing-pop behavior. Raw bytes cover host strictness and SWF4–8, pending thrown
undefined, function return, trace text and malformed If records.
See AVM1_CONTROL_MISSING.md. Остальные политики стека и все стадии открыты.

### v0.303 — missing member/string operands / Члены и строки без значений

Loaded StringExtract/MBStringExtract, GetMember/SetMember and Delete now prepare
undefined below existing operands. Fixtures cover all missing counts, host/SWF4–8,
collecting name coercion after dropping globals, and three-operand allocation
failures. See AVM1_MEMBER_STRING_MISSING.md. Full semantics and all stages stay open.

### v0.304 — missing variable/target operands / Имена переменных и целей

Six loaded variable/declaration/target actions now supply undefined on missing
operands; host name/stack checks stay strict. Raw SWF4–8 fixtures observe live
SetTarget2 reset/invalidation and caller restoration, plus named scope operations.
See AVM1_VARIABLE_TARGET_MISSING.md. Full scope/target semantics remain open.

### v0.305 — missing display/URL/With operands / Свойства, URL и With

Four loaded actions prepare missing undefined values while retaining payload
validation order and host strictness. Raw SWF4–8 tests distinguish target paths,
URL sink arguments and skipped With bodies. Display/URL layout constants are
named; an unused URL root is removed. See AVM1_DISPLAY_URL_WITH_MISSING.md.
Полные семантики и все стадии роудмапа остаются открытыми.

### v0.306 — shared frame-request preparation / Общая подготовка кадра

GotoFrame2 conversion now uses the C89 [fvm_exec_frame_request](../include/flashvm/execution.h#L336) service with rooted
coercion, alias/error preservation and separate scene-bias policy. Timeline wire
widths/root layouts use named macros. Raw Call/GotoFrame2 underflow fixtures cover
SWF4–8 alongside direct C, collecting/throwing callbacks and allocation injection.
See FRAME_REQUEST_SERVICE.md. Complete frame semantics and all stages remain open.

### v0.307 — missing class operands / Недостающие операнды классов

Loaded Extends/CastOp/InstanceOf supply missing undefined values with preserved
operand roles. Raw host/SWF4–8 fixtures verify primitive boxing, superclass getter
GC/throws and root/execution restoration. C services, interface counts and ABI
remain unchanged. See AVM1_CLASS_MISSING.md. All stages remain open.

### v0.308 — missing ImplementsOp operands / ImplementsOp без фиксированных значений

Loaded missing count/constructor values become undefined; zero count preserves
prototype snapshots and does not box primitive constructors. Raw host/SWF4–8 and
allocation-failure fixtures verify continuation, root restoration and no Number
callback. See AVM1_IMPLEMENTS_MISSING.md. Historical policy gaps/all stages stay open.

### v0.331 Transform schema constants

[TRANSFORM_CONSTANTS.md](TRANSFORM_CONSTANTS.md) records named native property
selectors, affine snapshot slots and preserved callback transport identities.
The raw-ID regression protects historical identities and rejects unused ranges;
this refactoring adds no compatibility capability and closes no roadmap stage.

В v0.331 числовая схема Transform получила смысловые макросы в отдельном .h.
Проверка исторических ID независима от новых макросов. Возможности и формат
переноса не изменены; стадии роудмапа остаются открытыми.

### v0.332 MovieClip.transform assignment

Implemented: typed Transform-to-native-clip assignment and explicit C89 local
matrix/color copy. Source/destination identity and unrelated scene state are
preserved; copying does not retain the source. New setter callback 1402 is
transportable. Source queries retain the existing native geometry getter policy,
so exact raw Flash display-state snapshot parity is still open. Historical
version/attribute availability and full Transform semantics are not certified.
See [CLIP_COPY_TRANSFORM.md](CLIP_COPY_TRANSFORM.md).

Реализовано присваивание типизированного Transform через общий C89-сервис;
матрица и цвет публикуются вместе, источник не удерживается. Точное совпадение
с сырыми снимками Flash, версиями доступности и всеми атрибутами ещё открыто.

### v0.333 Native point execution service

Implemented extraction: localToGlobal/globalToLocal share the C89
[fvm_exec_clip_point](../include/flashvm/execution.h#L65) service with generated callers. Separate native adapters keep
the same callback-table slots; field/getter coercion, physical ancestry and ordered
setter failures retain previous behavior. This is not a compatibility upgrade to
Flash's own-stored numeric field, twip or singular-fallback policies. See
[CLIP_POINT_SERVICE.md](CLIP_POINT_SERVICE.md); all roadmap stages remain open.

Реализована декомпозиция преобразования точки и общий интерфейс для C/JIT.
Прежняя семантика сохранена; точное поведение Flash для полей точки и twips
по-прежнему требует реализации и эталонных проверок.

### v0.334 Loaded point-field policy

Implemented: own stored numeric x/y without getter/inherited/coercion lookup,
invalid/missing-argument no-op and singular inverse identity fallback in loaded
SWF4–8. Shared direct C/native entry; host-only strict behavior retained.
Exact twips, nonfinite conversion, old case-folded spelling and raw geometry
snapshot parity remain open. See AVM1_POINT_FIELDS.md; no stage is closed.

Загруженные SWF используют собственные числовые поля точки, без getters и
преобразования типов; неподходящие аргументы игнорируются. Хостовый режим
сохранён. Точные twips, NaN/Infinity и старые правила регистра ещё открыты.

### v0.335 Loaded point input representation

Implemented: truncating/saturating signed 32-bit input twips and NaN/Infinity
normalization for loaded point conversion, with host-double policy preserved.
This replaces the preceding input-nonfinite limitation; full matrix/output twip
arithmetic and Flash Player oracle comparison remain open. Constants use scoped
macros. See AVM1_POINT_TWIPS.md; no roadmap stage is closed.

Числовой вход точки теперь нормализуется в signed 32-bit twips, включая
NaN/Infinity. Матрица и результат ещё не имеют полной fixed-point семантики;
эталонные проверки Flash и остальные стадии остаются открытыми.

### v0.336 Loaded point coordinate spelling

Implemented: pinned pre-SWF7 case folding of stored own coordinate names and
preserving their spelling on ordered writes; no inherited/getter lookup, no
prefix/NUL matching. SWF7–8 and host exact spelling remain. Case-equivalent host
slot ambiguity, geometry-driven name replacement, fixed-point world matrices
and output twips remain open. See AVM1_POINT_CASE.md; all stages stay open.

Исправлен регистр полей точки SWF4–6 и сохранение написания при записи через
общий C-сервис. Точные матрицы/выходные twips и перечисленные сложные случаи
по-прежнему открыты; завершение стадий не заявляется.

### v0.337 Context-free fixed matrix services

Implemented native C89 arithmetic: signed 16.16 matrix composition and signed
twip point application, with explicit modular sums, floor-shifted products,
range/alias validation and no context/allocation/GC. Independent integer-oracle
fixtures and 32-bit/wide-word checks accompany the services. See FIXED_MATRIX.md.
These primitives do not yet change scene storage or loaded point behavior;
full fixed-point scene integration and all roadmap stages remain open.

Реализован независимый C89-слой exact fixed-point произведений для матриц и
точек, без VM-контекста и аллокаций. Сцена и интерпретатор пока его не вызывают;
совместимость Flash и завершение стадий этим шагом не объявляются.

### v0.338 Binary32 scene point application and inverse

Implemented: context-free C89 binary32 point/inverse services, nearest/even
linear/translation twips, modular translation and checked singular epsilon.
Loaded point conversion uses them after input twip normalization; failed inverse
preserves the original normalized integer point. Host double semantics stay.
This corrects the representation distinction: raw SWF 16.16 and runtime scene
binary32 are separate models, not interchangeable helpers. Native ancestry/source
snapshots still use the old double model. See SCENE_MATRIX.md; no stage is closed.

Реализованы отдельные C89-сервисы binary32 application/inverse, подключённые к
загруженным точкам. Уточнено различие сырых SWF 16.16 и матриц сцены binary32.
Композиция предков, raw snapshots, другие подсистемы и стадии ещё открыты.

### v0.339 Per-ancestor scene matrix composition

Implemented: context-free C89 binary32 composition and rooted physical world
query [fvm_exec_clip_world_matrix](../include/flashvm/execution.h#L55). Loaded point ancestry now composes per level;
host point conversion keeps double behavior. Root-lock independence, bounded
cycles, alias/errors and getter-driven load mode stability are covered by new
fixtures. Local getter/decomposition snapshots, scrollRect/Stage, other display
objects and remaining subsystems/stages stay open. See CLIP_WORLD_MATRIX.md.

Композиция предков точки вынесена в общие C-сервисы; loaded-путь теперь
округляет каждый уровень. Хостовый режим сохранён, выбор режима защищён от
загрузки SWF из getter. Raw snapshots и перечисленные ограничения не закрыты.

## v0.341: common host inverse projection

Host pointer and drag adapters share the context-free C89 inverse point/vector
service; see AFFINE_PROJECTION.md for contracts and numerical limits. Direct
C89, alias/error and allocation-disabled cases supplement existing adapter
coverage. Перетаскивание сохраняет прежнюю double-семантику; точная совместимость
Flash и завершение этапов roadmap этим изменением не подтверждаются.

## v0.342: host point algebra extraction

Host point conversion now calls common C affine inversion/application while
retaining inverse-then-apply rounding and overflow propagation. Standalone
C89 and direct/native host regression cases are in AFFINE_PROJECTION.md.
Семантика загруженного SWF не меняется; этапы roadmap остаются открытыми.

## v0.343: literal execution services

Array/Object literal construction is shared by the interpreter and direct C
callers. Host/SWF4–8, own slots, counted keys, duplicate/empty keys, collecting
name callbacks, input-buffer mutation, exceptions and allocation failures have
regression cases in LITERAL_SERVICES.md. Семантика счётчика/стека остаётся
в адаптере; полной совместимости Flash и закрытия этапов это не означает.

## v0.344: literal count service

Versioned literal-count coercion is shared with C/generated callers, including
signed rejection, NaN/fractions and the materialization bound. C89 cases plus
collecting valueOf, exceptions, aliases and allocation-failure coverage are
recorded in LITERAL_SERVICES.md. Чтение операндов и изменение стека остаются
в интерпретаторе; этапы roadmap не закрыты.

## v0.345: named-constructor count preparation

C89 count preparation is shared with NewObject: host bound 65536, loaded UInt32
conversion/capping and bound 65535. Boundary construction, callbacks/GC, errors
and allocation failures are covered in NEW_OBJECT_NAMES.md. Сохранён порядок
преобразования имени и счётчика; этапы roadmap остаются открытыми.

## v0.346: experimental native compiler

First implemented/locally verified Linux x86-64 System V call-based JIT subset:
explicit C89 compilation/binding, native branches/loops, shared numeric/variable
services, rooted transient stack, checkpoints and W^X memory port. JIT.md lists
accepted operations, executable regressions and limitations. This supersedes
older historical statements that no native backend exists; full JIT coverage,
automatic selection, other CPU/OS backends and roadmap stages remain open.
Первый backend исполняет машинный код, но готовность полного JIT не заявляется.

## v0.347: runtime execution modes and diagnostic replay

Interpreter/JIT entry options and a same-thread switching API now select root
and function execution. A bounded context-owned native cache falls back on
unsupported bodies. Opt-in [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48) replay recreates interpreter activation,
logs checkpoints and preserves the original thrown graph/fault/budget after
retry success, a different exception or OOM. EXECUTION_MODES.md records contracts,
executable cases, duplicated-effect semantics and backend/profile limitations.
Режимы и диагностика реализованы для первого backend; полный JIT остаётся открытым.

Контрольная точка v0.348: добавлены Linux ABI encoders x86, ARMv7 A32, ARM64,
RV32I и RV64I, а также исключение тел с breakpoint из JIT, примеры workers и
нескольких VM. См. JIT_ARCHITECTURES.md. Новые ISA проверены в Unicorn; полные
сборки/запуски VM на чужих CPU и ОС не проверены. Стадии 8/9 остаются открытыми.

v0.348 adds five Linux machine encoders and interpreter-only breakpoint bodies,
plus worker/multiple-VM examples. New ISA evidence is synthetic-helper emulator
execution, not full foreign-CPU/OS acceptance; stages 8/9 remain open.

## v0.349: platform validation infrastructure

Implemented: native AGG/Cairo/minimal release runner, failure-aware evidence,
explicit-device Android CTest/ADB adapter and cross-build exclusion of host-only
Python process harnesses. Host adapter tests cover quoting, failed push, timeout,
remote nonzero status and invalid paths. Cross-configuration inspection proves
all 675 minimal-profile target commands use the selected adapter and file-process
fixture ordering remains present. This is not Android execution evidence.
Real Windows/macOS/Android acceptance and all roadmap stages remain open.
See PLATFORM_VALIDATION.md for use and exact runtime evidence.

Добавлены повторяемые прогоны и адаптер Android; проверены ошибки транспорта
на хосте и регистрация кросс-тестов. Реальный запуск Android этим не подтверждён,
полнота Flash и стадии 1–9 не объявлены завершёнными.

## v0.350: native references

Implemented and verified on native Linux x64: SetVariable, DefineLocal/DefineLocal2,
GetMember/SetMember, Delete/Delete2. Shared C services preserve interpreter target,
activation, versioned coercion and operand policy. Differential host/SWF4–8,
setter GC/exception and allocation-boundary tests are in JIT_REFERENCES.md.
Six restricted heap/global ASan+UBSan checks pass; full Action coverage and real
foreign-target/platform acceptance remain open. Все стадии roadmap ещё открыты.

## v0.351: native numbers and unary operations

Implemented/locally verified: float/double Push and ToInteger, ToNumber, ToString,
TypeOf, Increment, Decrement, Not. Interpreter/native IEEE decoding now shares the
same arithmetic helpers. Raw independent literal values and host/SWF4–8 comparisons
cover manual/automatic native entries, missing inputs, collecting/throwing coercion
and allocation failures. See JIT_NUMBERS.md. Nine restricted ASan+UBSan checks
pass. Full Action/Flash coverage, foreign-runtime execution and stages 1–9 remain
open. Это расширение нативного подмножества, а не завершение JIT или выпуска.

## v0.352: native register banks

Implemented: Push-register and StoreRegister over the prepared private/global
banks, including Function2 preloads and explicit rooted host/native root banks.
Host/SWF4–8, manual/automatic, boundary/missing-value, GC-only-register retention
and counted-string allocation tests are in JIT_REGISTERS.md. Constant pools,
function definitions/calls, complete Action coverage and foreign runtime/platform
acceptance remain open. Все девять стадий не объявляются завершёнными.

## v0.353: native constant pools

Implemented/tested: ConstantPool and constant8/constant16 Push, captured activation
pools, invocation-local runtime selection and nested native/root scope restoration.
Host/SWF4–8 differential, branch/cache isolation, returned-string lifetime,
allocation and malformed/bounded-input fixtures are in JIT_POOLS.md. Ten restricted
ASan+UBSan checks pass. Other Actions, full Flash/reference equivalence and real
foreign-target/platform acceptance remain open. Все стадии остаются открытыми.

## v0.354: native method dispatch

CallMethod/NewMethod use shared rooted AVM1 services, preserving count/name
validation, ordered arguments, nullish-owner behavior and constructor semantics.
JIT_METHODS.md records host/SWF4–8, explicit/automatic, GC, exception and
allocation checks. Именованные вызовы, остальные инструкции и все стадии
роудмапа, включая реальные запуски других платформ, остаются открытыми.

## v0.355: native named calls

CallFunction/NewObject use shared rooted lexical/target lookup and invocation.
JIT_NAMED_CALLS.md records versioned host/SWF4–8 comparisons, local/qualified
function names, literal loaded constructor names, collecting name/count coercion,
exceptions and allocation failures. Автоматический JIT проверен для обоих тел
вложенного вызова. Все стадии и реальные запуски других платформ ещё открыты.

## v0.356: native target selection

SetTarget/SetTarget2 use shared versioned selection and scene validation.
JIT_TARGETS.md records host/SWF4–8 comparisons, literal/dynamic/missing/removed
paths, collecting path coercion, exceptions, compile/invoke allocation failures
and suspended-target restoration. Все стадии и приёмка других CPU/ОС открыты.

## v0.357: native string Actions

Twelve string Actions now share counted/versioned interpreter services, preserving
conversion order, rooted operands and existing SWF4–5 ASCII/character limits.
JIT_STRINGS.md records exact UTF-16, missing-input, manual/automatic, collecting/
throwing coercion, allocation and returned-text lifetime checks. Полная строковая
совместимость Flash, другие инструкции и все стадии приёмки ещё открыты.

## v0.358: native Array/Object initializers

InitArray/InitObject use shared count normalization and construction, preserving
entry order, independent missing pair members, own Array slots and counted keys.
JIT_LITERALS.md records versioned native/interpreted, collecting/throwing callback,
allocation and result-lifetime checks. Определения функций, остальные инструкции
и полная совместимость и приёмка всех стадий ещё открыты.

## v0.359: native legacy comparisons and logic

Equals/Less and And/Or use shared versioned coercion and metamethod services.
JIT_LEGACY.md records 1944 primitive combinations in three execution modes,
missing inputs, numeric conversion order, collecting/throwing left/right hooks
and allocation failures. Остальные инструкции, полная совместимость и все стадии
приёмки, включая реальные запуски других CPU/ОС, остаются открытыми.

## v0.360: native class Actions

Extends/Implements/InstanceOf/Cast share rooted class/interface services.
JIT_CLASSES.md records isolated versioned execution, interface snapshots,
collecting/throwing accessors and primitive boxing, missing operands and allocation
failures. Полная семантика Flash, остальные инструкции и все стадии открыты.

## v0.361: repeatable encoder validation gate

validate_platform.py --jit-emulator resolves the configured Release generator
from CTest, records Unicorn version and runs freshly generated ISA/ABI fixtures.
Failures and optimized Python cannot produce passing emulation evidence; five
host-only orchestration tests cover failure states and stale-fixture isolation.
PLATFORM_VALIDATION.md records usage and scope. Это инфраструктура стадий 8/9,
а не реальный запуск полной VM на других CPU/ОС или завершение стадий.

## v0.362: native display-property and path Actions

GetProperty/SetProperty/TargetPath share rooted scene and counted-path services.
JIT_DISPLAY.md records isolated host/SWF4–8 execution, target selection, ordered
collecting/throwing coercion, skipped readonly/invalid writes, counted native paths
and allocation failures. Остальные поля, полная семантика Flash и все стадии
приёмки, включая реальные запуски других CPU/ОС, остаются открытыми.

## v0.363: native effect Actions

Trace/Throw/GetTime/RandomNumber share rooted execution and exception services.
JIT_EFFECTS.md records host/SWF4–8 equivalence, collecting/throwing conversions,
trace sink failures, exact random state, clock samples and allocation failures.
Полная совместимость Flash, остальные инструкции и реальные запуски на других
ОС остаются открытыми; ни одна стадия не закрыта.

## v0.364: native enumeration Actions

Enumerate/Enumerate2 share rooted qualified/plain lookup and property-name snapshots.
JIT_ENUMERATE.md records host/SWF4–8 equivalence, counted/hidden/inherited keys,
MovieClip lookup, collecting/throwing conversions/getters, allocation failures and
exact stack bounds. Полная совместимость Flash и реальные запуски других ОС/CPU
остаются открытыми; ни одна стадия не закрыта.

## v0.365: native timeline Actions

NextFrame/PreviousFrame/Play/Stop/GotoFrame/GotoLabel/GotoFrame2 share intrinsic
timeline services, rooted request conversion and versioned target selection.
JIT_TIMELINE.md records isolated host/SWF4–8 equivalence, bounds/labels/bias,
collecting/throwing coercion, changed targets, malformed records and compile/invoke
allocation failures. Полная сцена SWF, совместимость Flash и реальные запуски
других ОС/CPU остаются открытыми; ни одна стадия не закрыта.

## v0.366: native clone, remove and drag Actions

CloneSprite/RemoveSprite/StartDrag/EndDrag share rooted native scene services.
JIT_SCENE.md records isolated host/SWF4–8 equivalence, intrinsic clone/removal,
selected-target repair, ordered collecting/throwing coercion, detached drag roots,
EndDrag exception/retry and allocation failures. Полная совместимость сцены Flash,
оставшиеся инструкции и реальные запуски других ОС/CPU остаются открытыми.
Ни одна стадия не закрыта.

## v0.367: native frame waits and counted skips

WaitForFrame/WaitForFrame2 share rooted availability queries and native conditional
branches over complete source records. JIT_WAIT_FRAMES.md records host/SWF4–8
equivalence, strict text/missing inputs, collecting/throwing conversion, compile/
invoke allocation failures, invalid targets and exact early/implicit-End budgets.
Неподдерживаемые блоки, полная совместимость Flash и реальные запуски других
ОС/CPU остаются открытыми; ни одна стадия не закрыта.

## v0.368: native URL and stored-frame dispatch

GetURL/GetURL2/Call share rooted host/frame services. Validated stored frame
blocks now follow automatic JIT selection with named-breakpoint exclusion.
JIT_DISPATCH.md records host/SWF4–8 behavior, counted requests, ordered GC/throws,
nested frame/pool/target preservation, bounded recursion/budget, fallback and
compile/invoke allocation failures. Полная совместимость Flash, browser/streaming
и реальные запуски других ОС/CPU остаются открытыми; ни одна стадия не закрыта.


v0.369: bounded stored-frame diagnostic replay preserves implicit End framing;
`execution_mode_frame_replay` checks host/SWF4–8 and success/throw retries.
Диагностический повтор сохраняет границы кадра с неявным End, исходное исключение,
бюджет и корни. This correction does not close any roadmap stage; все стадии открыты.


v0.370: DefineFunction/DefineFunction2 compile to rooted native creation/publication.
Shared interpreter/native materialization retains executed constant pools, lexical
closures, parameter registers and preload metadata; nested unsupported bodies fall
back independently. JIT_DEFINITIONS.md / .ru.md record tests and remaining gaps.
Нативные определения функций проверяются с GC/setter-исключениями, повреждёнными
записями и отказами памяти. With/Try и полная совместимость остаются открытыми;
ни одна стадия не закрыта.


v0.371: native With owns compiled child bodies and shares rooted operands,
registers, executed pools and instruction accounting. Nested Return propagates;
lexical scopes and code activity unwind on errors/GC/OOM. Aggregate native bytes
include children; 64-level nesting and breakpoint/cache behavior are tested.
JIT_WITH.md / .ru.md record evidence and limits. Try/remaining Actions, full
conformance and real foreign-platform acceptance remain open; все стадии открыты.


v0.372: native Try/Catch/Finally shares compiled block state with With, roots
suspended baselines/returns/exceptions/host bindings, restores rich faults and
preserves versioned catch-name/register rules. Root JIT selection now contains
allocation failure before breakpoint lookup. JIT_TRY.md / .ru.md document tests,
non-script error behavior and remaining conformance/platform gaps. Ни одна стадия
не закрыта; полная совместимость и реальные запуски других ОС остаются открытыми.


v0.373: borrowed loaded negative-length splice uses signed own insertion keys,
captured conversion bounds and rooted empty removals; native negative storage
and signed overflow remain unsupported. Этапы остаются открытыми.
See [EN](ARRAY_NEGATIVE_SPLICE.md) / [RU](ARRAY_NEGATIVE_SPLICE.ru.md).

Validation v0.373: 769 AGG / 769 Cairo / 750 minimal Release checks pass after
private-header rebuild and ELF permission repairs/reruns; five restricted
heap/global ASan+UBSan splice checks pass (stack ASan/LSan disabled).

Validation v0.374: all 771 AGG / 771 Cairo / 752 minimal Linux Release checks
have passing evidence after ELF execution-permission repairs and failed-test
reruns. Five restricted heap/global ASan+UBSan checks pass (stack ASan and LSan
disabled): both new end-operation tests, legacy push/unshift and negative splice.

Validation v0.375: 773 AGG / 773 Cairo / 754 minimal Linux Release checks
have passing evidence (AGG/Cairo ELF permission repairs and failed-test reruns;
minimal passed the full run directly). Five restricted heap/global ASan+UBSan
checks pass: both negative-slice tests, both negative-end tests and legacy slice.
Stack ASan and LeakSanitizer remain disabled in this environment.

Validation v0.376: all 775 AGG / 775 Cairo / 756 minimal Linux Release checks
have passing evidence after nine ELF permission repairs and failed-test reruns
(4 AGG, 2 Cairo, 3 minimal). Five restricted heap/global ASan+UBSan checks pass,
including both new large-mutation checks, legacy unshift/splice flags and negative
end operations. Stack ASan and LeakSanitizer remain disabled. The new scoped
constant header also passes a strict C89 syntax probe.

Validation v0.377: all 777 AGG / 777 Cairo / 758 minimal Linux Release checks
have passing evidence after five ELF permission repairs (2/2/1) and six failed-test
reruns, including the dependent timezone profile runner. Six restricted heap/
global ASan+UBSan checks pass: both new large-sort checks, legacy allocation,
comparator, snapshot and no-op checks. Stack ASan and LeakSanitizer remain
disabled. The revised scoped constant header passes strict C89 syntax checking.
Original external SWF traces were not rerun; no new external oracle pass is claimed.

Validation v0.378: all 781 AGG / 781 Cairo / 762 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and failed-test
reruns (two per profile). Seven restricted heap/global ASan+UBSan checks pass:
four new Unicode path/target checks plus existing resolver, JIT target and point
case checks. Stack ASan and LeakSanitizer remain disabled. Both revised scoped
constant headers pass strict C89 syntax checking. No new external SWF oracle
or foreign-OS runtime acceptance is claimed.

Validation v0.379: all 785 AGG / 785 Cairo / 766 minimal Linux Release checks
have passing evidence after nine executable-permission repairs on eight ELF
files and targeted reruns (four repair operations AGG, three Cairo, two minimal).
Eight restricted heap/global ASan+UBSan checks pass: the four new dimension
checks plus standard MovieClip, AVM1/JIT display properties and geometry
allocation. Stack ASan and LeakSanitizer remain disabled. No new external SWF
oracle or foreign-OS runtime acceptance is claimed. All nine stages remain open.

Validation v0.380: all 789 AGG / 789 Cairo / 770 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new registry/scene/JIT cases plus existing
C89 registry, name/transfer, sprite resources and duplicate-version checks.
Stack ASan and LeakSanitizer remain disabled. No new original external SWF
oracle or foreign-OS run is claimed. All nine roadmap stages remain open.

Validation v0.381: all 792 AGG / 792 Cairo / 773 minimal Linux Release checks
have passing evidence; AGG/Cairo passed initially, minimal required one ELF
execution-permission repair and targeted rerun. Eight restricted heap/global
ASan+UBSan checks pass: three new graph/native checks, existing membership
callbacks/allocation/data and large-sort/negative-end Array regressions.
Stack ASan and LeakSanitizer remain disabled. The new native polling and revised
membership constants headers pass a strict C89 syntax probe. No new original
external SWF oracle or foreign-OS acceptance is claimed. All nine stages remain open.

Validation v0.382: all 796 AGG / 796 Cairo / 777 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (two AGG, one minimal; Cairo passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new work/allocation/callback/native cases
plus legacy split/search/receiver and character-allocation checks. Stack ASan
and LeakSanitizer remain disabled. The revised String constants header passes
a strict C89 syntax probe. No new original external SWF oracle or foreign-OS
run is claimed. All nine roadmap stages remain open.

Validation v0.383: all 799 AGG / 799 Cairo / 780 minimal Linux Release checks
have passing evidence after five ELF execution-permission repairs and targeted
reruns (two AGG, three Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: three new declaration/work cases, existing
snapshot allocation/transport, membership data/large-graph and collecting
membership-callback regressions. Stack ASan and LeakSanitizer remain disabled.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

Validation v0.384: all 803 AGG / 803 Cairo / 784 minimal Linux Release checks
have passing evidence after nine ELF execution-permission repairs and targeted
reruns (four AGG, four Cairo, one minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new radix/callback/allocation/native cases plus
legacy radix/conversion, C89 standard Number and native numeric regressions.
Stack ASan and LeakSanitizer remain disabled. The new scoped radix constants
header passes a strict C89 syntax probe. No original external SWF oracle or
foreign-OS acceptance was added. All nine roadmap stages remain open.

Validation v0.385: all 807 AGG / 807 Cairo / 788 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo, three minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new dimension write/allocation/callback/native
cases plus previous dimension reads, AVM1 display properties, native indexed
reads and geometry allocation. Stack ASan and LeakSanitizer remain disabled.
The revised MovieClip constants header passes a strict C89 syntax probe.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

Documentation v0.404: standalone EN/RU README files in
`examples/embedding/function_rpc/` describe purpose, build, process/thread/LAN
execution and local CTest commands. No runtime compatibility status changes.

### v0.409 — UTF-16 count memory / Память подсчёта UTF-16

Implemented allocation-free count queries and boxed String length validation;
see [UTF16_STRINGS.md](UTF16_STRINGS.md). Проверка без выделений добавлена;
этапы 1–9 остаются открытыми. Linux minimal Release: 822/822 CTests passed. Generated native-test launchers
restore executable mode before exec because the workspace resets file modes;
test bodies and assertions are unchanged. Other-platform acceptance remains open.

### v0.410 — UTF-16 buffer preflight / Проверка ёмкости UTF-16

Valid short buffers now publish required length without allocation or writes;
malformed input and overlapping copy behavior are preserved.
[Details / Подробности](UTF16_STRINGS.md). All nine stages remain open.

Validation v0.410 / Проверка: Linux minimal Release 822/822; AGG Release
4/4 focused string tests; heap/global ASan+UBSan 4/4. Generated native launchers
restore workspace executable permissions; stack ASan and LSan remain disabled.
No other-platform runtime or complete stage acceptance is claimed.

### v0.411 — one UTF-16 snapshot allocation / Одно выделение снимка

The sufficient-capacity C API copy reserves the measured UTF-16 size exactly,
removing geometric reallocation and transient old/new decoded arrays. Snapshot
semantics for overlapping input/output remain. A 257-unit regression allowing
exactly one allocation fails on v0.410 and passes after this change.
Копирование резервирует точный размер снимка одним выделением памяти; перекрытие
буферов поддерживается. Все девять стадий остаются открытыми.

Validation v0.411 / Проверка: minimal Release 822/822; AGG Release 4/4;
heap/global ASan+UBSan 4/4 string tests. Strict C89 public-header syntax passed.
Native test launchers restore executable modes; stack ASan/LSan are disabled.
Other platforms and complete Flash compatibility remain unverified.

### v0.412 — loaded parseInt / Загруженный parseInt

Separate SWF4–8 parsing now uses radix-before-text coercion, explicit-base
validation, raw hex/octal and counted-prefix rules; unloaded host behavior stays.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All stages remain open.
User reconfirmed SWF ≤8 only on 16 October; AVM2/AS3/SWF9+ are outside this scope.

Validation v0.412 / Проверка: Linux minimal Release 823/823 CTests; AGG Release
and heap/global ASan+UBSan each passed all three global-parsing executables.
Native launchers restore workspace executable modes; stack ASan/LSan disabled.
Other-platform runtime and all nine stage acceptance remain open.

### v0.413 — loaded parseFloat / Загруженный parseFloat

Separate SWF4–8 decimal accumulation now preserves pinned decimal rounding,
multiple-dot, missing-argument and wrapped-exponent behavior with counted scans.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All stages remain open.

Validation v0.413 / Проверка: minimal Release 824/824; AGG Release and
heap/global ASan+UBSan each passed all four global-parsing executables.
Native mode-restoring launchers used; stack ASan/LSan disabled.
Other-platform runtime acceptance and all nine stages remain open.

### v0.414 — parsing work/GC/JIT / Бюджет, GC и JIT разбора

Loaded numeric scans share nested AVM1 work and saturated counters. Exact quota,
allocation-failure cleanup and real interpreted/attached/automatic native call
regressions added. [EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md).
All nine stages remain open; no complete decoder/coercion budget is claimed.

Validation v0.414 / Проверка: minimal Release 827/827; AGG Release 7/7 and
heap/global ASan+UBSan 7/7 parsing/JIT tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. All nine stage acceptances remain open.

### v0.415 — numeric leading whitespace / Ведущие пробелы чисел

Loaded Number/ToNumber and parseFloat now share Flash's four-unit leading-space
policy; host rules stay. SWF4 invalid-zero vs SWF5–8 NaN, counted surrogate/NUL,
traced GC and actual native entry regressions added.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All nine stages remain open.

Validation v0.415 / Проверка: minimal Release 829/829; AGG Release 9/9 and
heap/global ASan+UBSan 9/9 parsing/whitespace/JIT tests. Native mode-restoring
launchers used; stack ASan/LSan disabled. Other-platform acceptance remains open.

### v0.416 — shared decimal coercion / Общее десятичное преобразование

Loaded Number/ToNumber/numeric operands share reference-ordered decimal
accumulation with parseFloat; strict SWF5–8, relaxed SWF4 and raw integer radix
policies retained. Primitive decimal conversion removes temporary allocations.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All nine stages remain open.

Validation v0.416 / Проверка: minimal Release 832/832; AGG Release 12/12 and
heap/global ASan+UBSan 12/12 parsing/decimal/whitespace/JIT tests. Native launchers
restore workspace executable modes; stack ASan/LSan disabled. No other-platform
runtime acceptance or complete stage closure is claimed.

### v0.417 — apply preparation roots / Корни подготовки apply

Boxed receiver and partially captured arguments survive real script-getter GC.
Counted strings, cycles, errors/exceptions and retry covered in host/SWF4–8.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.417 / Проверка: minimal Release 833/833; AGG Release 2/2 and
heap/global ASan+UBSan 2/2 apply/function tests. Generated native launchers restore
workspace executable modes; stack ASan/LSan disabled. Foreign runtime acceptance
and all nine stages remain open.

### v0.418 — loaded apply length / Длина загруженного apply

Primitive lists supply zero arguments; own raw length uses version casing and
AVM1 Int32 conversion, with rooted collecting callbacks. Loaded negative/large
lengths remain explicitly bounded. [EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md).
All nine stages remain open.

Validation v0.418 / Проверка: minimal Release 834/834; AGG Release 3/3 and
heap/global ASan+UBSan 3/3 apply/function tests. Native launchers restore workspace
executable modes; stack ASan/LSan disabled. Other-platform acceptance remains open.

### v0.419 — apply property capture / Чтение свойств apply

Loaded native Number/Boolean/String/Date lists use ordinary named properties,
original getter receiver and retained snapshots across mutation/GC/failure.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.419 / Проверка: minimal Release 835/835; AGG Release 4/4 and
heap/global ASan+UBSan 4/4 apply/function tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. Other-platform runtime acceptance remains open.

### v0.420 — incremental apply work / Постепенный apply с бюджетом

Loaded positive Int32 lists grow rooted capture under shared work; the former
65536 cap is removed. C/script 65537 slots, huge bounded allocation, OOM/unwind
and real native CallMethod checks added. Host/negative-length policy retained.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.420: Linux minimal 838/838; AGG 7/7; heap/global ASan+UBSan 7/7.
Generated native-mode launchers compensate for workspace executable-mode resets.
Stack ASan and LeakSanitizer are disabled in this environment. Other platforms
and completion of all nine stages remain unverified/open.

## v0.421 — borrowed adapters and non-callable targets

Loaded SWF4–8 Function.call/apply return undefined when their borrowed receiver
has no invocation callback. Receiver boxing and apply length/index preparation
still happen first: getter errors, counted exceptions and quota exhaustion are
preserved. Host API behavior remains UNSUPPORTED for the same non-callable target;
public direct invocation and callable C/script/remote interfaces retain dispatch.
The behavior follows the pinned Ruffle function.rs reference linked above.

`function_borrowed_adapters` reproduces the old failure and checks all loaded
versions plus host behavior, actual GC in a script getter, counted exception
forwarding, retry, callable dispatch and quota-before-result ordering.
`jit_function_apply` also executes both borrowed adapters through interpreted,
attached and automatic native CallMethod paths. Complete super-adapter semantics,
full Function compatibility and all nine roadmap stages remain open.

Validation v0.421: Linux minimal 839/839; AGG 9/9; heap/global ASan+UBSan
9/9, including remote-function regression. Generated executable-mode launchers
were used; stack ASan/LeakSanitizer remain disabled. No other OS certification.

## v0.422 — super facades require direct dispatch

Loaded Function.call/apply treat a retained super facade as a non-function even
though FlashVM exposes a native callback for direct superclass invocation.
Argument preparation still precedes undefined: script index getters run, may
collect, and propagate errors/counted exceptions. Host borrowed invocation and
public direct invocation of the facade keep their previous parent dispatch.
This follows the pinned Ruffle function.rs explicit as_function/direct-super
separation; it does not certify every super or Function behavior.

`function_super_adapters` first failed against v0.421, then checks loaded SWF5–8
and host mode, both adapters, parent side effects, real getter GC, errors,
embedded-NUL exceptions, retry, direct invocation and root/work cleanup.
Negative signed apply lengths remain explicitly UNSUPPORTED: the reference casts
them to an allocation size, so treating them as zero is not supported by that
source. All roadmap stages remain open.

Validation v0.422: Linux minimal 840/840; AGG 13/13; heap/global ASan+UBSan
13/13, including direct super/owner/prototype and remote-function regressions.
Generated executable-mode launchers were used. Stack ASan/LeakSanitizer remain
disabled; no additional operating system or complete-stage acceptance is claimed.

## v0.423

v0.423 adds a shared allocation-free stored-name selector using the existing
pinned Flash UTF-16 lowercase comparator. Loaded SWF4–6 own membership, attribute
queries and ordinary prototype-chain reads accept equivalent spelling. Host and
SWF7–8 keep exact names. Data, accessor and native method tables are covered;
queries never execute getters. Wrapped native methods use their stored spelling,
so reading a case variant preserves Function identity. Existing own assignments
select stored spelling; default intrinsic __proto__ aliases select its canonical
name. This avoids introducing a duplicate during a legacy update.

`object_property_case_queries` failed on v0.422 and covers SWF4–8/host, ASCII,
U+00C4/U+00E4, embedded NUL and lone-surrogate names, inherited reads vs own
membership, hidden flags, no getter/method execution, wrapped method identity,
legacy updates and real GC. Existing prototype/query tests remain regressions.
Reference: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
[script_object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
uses activation.is_case_sensitive() for property membership and access.

This is a tested subset, not full name compatibility. Case-equivalent duplicate
slots supplied through raw/internal APIs prefer exact spelling and then map order;
Flash collision order is not certified. Flag mutation/deletion, inherited setter
spelling, watchers, definition APIs, extension metatables and full super membership
still need coordinated work. Linear folded lookup is not fully work-budgeted.
The original binary SWF corpus could not be restored through the text-only reader;
no new original-SWF pass count is claimed. All nine stages remain open.

Loaded SWF5+ no longer stores a synthetic ordinary this slot: activation-cell
lookup preserves the SWF5 local special-name rule (pinned function.rs load_this
sets registers only). Existing this/interpreter/JIT tests remain unchanged. The
coordinate test now expects folded public reads in SWF4–6; its coordinate
conversion assertions are retained. The class GC fixture uses ParentClass instead
of Super, which now correctly collides with the local implicit super name.

Validation v0.423: Linux minimal 841/841; AGG 8/8; heap/global ASan+UBSan
8/8. Generated native-mode launchers compensate for workspace mode resets.
Stack ASan/LeakSanitizer remain disabled. Other platforms and all stages open.

## v0.424 — mutate the selected own spelling

Flag updates and deletion select the own stored spelling before mutation. A
legacy alternate name now modifies/removes the same data, accessor or method
as its read and membership query. DONT_DELETE is checked before erasure; successful
deletion clears matching attributes, and inherited properties remain untouched.
Selection snapshots the name before erasing tables and remains inside the C API
allocation-failure boundary. Assignment shares the selector introduced here.
Host/SWF7–8 exact spelling and the pre-existing flags-only API policy remain.

`object_property_case_mutation` reproduced v0.423's mismatched flag update. It
checks SWF4–8/host, data/accessors/methods, NUL/Unicode names, DONT_DELETE failure,
clear/retry/delete, stale flags, inherited ownership, no callback execution and GC.
Inherited setter/watch/definition/collision/budget and full super semantics are
still open; this does not complete a roadmap stage.

Validation v0.424: all 842 Linux minimal cases passed (840 initial successes;
two process tests initially hit workspace child-executable PermissionError, then
passed 2/2 after restoring mode). AGG 6/6; heap/global ASan+UBSan 6/6. Generated
mode launchers used; stack ASan/LeakSanitizer disabled. Other OS/stages open.

## v0.425 — inherited setter selection and native execution

The ordinary setter chain now finds data/accessors with the same versioned name
selector as reads. Own data still shadows inherited setters; host/SWF7–8 exact
matching remains. Setter activation snapshots the accessor's stored name, not the
requested variant. Script setter re-resolution and root ownership therefore refer
to the actual descriptor even when callbacks delete it and detach the prototype.

`object_property_case_setter` first failed on v0.424. It checks counted names with
NUL across SWF4–8/host, receiver/owner/name identity, real GC, owner detachment and
descriptor deletion, plain errors, counted exceptions and invocation/root cleanup.
`jit_property_case_setter` executes the same cases via SetMember in interpreted,
attached and automatic JIT modes and asserts native entries. The allocator sweep
`object_property_case_allocation` covers flags/delete/own data updates, failures
through success, unchanged values/flags on OOM and restored root/setter metadata.
Watchers, definition APIs, duplicate-order semantics, complete scope/name rules
and folded traversal work accounting remain open. All nine stages remain open.

Validation v0.425: Linux minimal 845/845; AGG, NO_OS_PORTS and heap/global
ASan+UBSan 7/7 each. Native entry assertions run on available Linux x64; the
NO_OS_PORTS configuration is a Linux-hosted build, not another OS run. Generated
mode launchers used; stack ASan/LeakSanitizer disabled. All stages remain open.

## v0.426 — own membership through super

Loaded super facade own-membership now delegates to its retained receiver,
matching pinned script_object.rs has_own_property. The query does not inspect
private facade metadata or enumerate/read parent properties. It preserves the
receiver's SWF casing. Host facade behavior remains unchanged. Nested/malformed
receiver-facade chains have a bounded iterative traversal instead of recursive
C++ calls; malformed cycles return [FVM_ERROR](../include/flashvm/flashvm.h#L43).

`object_super_membership` failed against v0.425 and covers loaded SWF5–8 plus
host, direct C and borrowed Object.hasOwnProperty.call, counted NUL names,
exact/folded spelling, inherited/missing properties and a throwing AVM1 getter
which membership never invokes. Real GC, malformed-cycle rejection/recovery and
queue transfer to another loaded context are checked. The destination repeats
the queries after collecting. Full super enumeration/flags/deletion/definitions,
independent Flash validation, other transports/platforms and all stages remain
open. This is a query-only compatibility change.

Validation v0.426: Linux minimal 846/846; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack
ASan/LeakSanitizer disabled. No independent Flash or other OS certification.

## v0.427 — watcher name selection

Loaded SWF4–6 watch replacement, dispatch, raw old-value lookup and unwatch use
Flash name equivalence. The callback receives the requested counted spelling,
not the stored property spelling. Own-property selection now happens after the
watcher returns, so callback mutation is resolved against the current table.
Host/SWF7–8 keep exact watcher names. Getter-free old-value lookup still preserves
virtual stored data, numeric slots and lazy native methods.

`object_property_case_watch` failed on v0.426's callback-name assertion. It checks
SWF4–8/host, NUL names, old/new/userData arguments, single-record replacement,
case-varied unwatch, self-removal followed by real GC, errors/counting exceptions
and running/root cleanup. Primary reference: pinned script_object.rs watch,
unwatch and call_watcher, which passes the requested name to Watcher.call.
Exception-side writeback, watcher recursion identity/collision details, complete
movie-clip watch behavior and budgeted folded traversal remain open. The existing
exception policy is retained here; no complete stage is claimed.

Validation v0.427: Linux minimal 847/847; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS execution and all-stage acceptance remain open.

## v0.428 — case-aware virtual replacement

Loaded SWF4–6 addProperty selects an existing own stored spelling before checking
attributes and replacing its descriptor. Stored data and flags survive replacement;
a variant does not create a second virtual property beside the original data.
Host/SWF7–8 preserve distinct names. The existing null-setter and rollback rules
are retained. This follows pinned script_object.rs add_property_with_case.

`object_property_case_definition` first failed on v0.427. It checks SWF4–8/host,
NUL names, retained flags/data, a collecting script getter, no premature getter
execution and writable retained data with a null setter. An allocation sweep runs
through failure to successful definition, checking previous values/flags and roots.
`jit_property_case_watch` additionally checks requested watcher names, collecting
self-removal and errors via interpreted/attached/automatic native SetMember, with
actual native-entry assertions. Native C definitions, collisions, watcher throw
writeback, complete property semantics and all stages remain open.

Validation v0.428: Linux minimal 850/850; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 8/8 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. No other OS or complete-stage certification.

## v0.429 — rooted C definitions and stored names

Rooted native method/accessor definition selects the existing own spelling in
loaded SWF4–6 before applying the existing conflict policy. Replacing a method or
accessor no longer introduces a hidden case-equivalent duplicate; a data/accessor
conflict which fails for exact spelling also fails for its equivalent name.
Host/SWF7–8 keep distinct names. C89 signatures and ABI are unchanged.

`object_property_case_native` reproduced v0.428's duplicate-method failure. It
checks SWF4–8/host, counted NUL names, replacement callbacks/capture roots, method
and accessor table identity, rejected data conflicts, actual GC and invocation of
a previously pinned Function which retains its old callback/capture. Native API
policy is an embedding contract aligned with loaded lookup; it is not an Adobe C
API claim. Stale unexported method caches, full callback lifetime/collision rules,
watcher exception semantics and all stages remain open.

Validation v0.429: Linux minimal 851/851; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS and complete-stage acceptance remain open.

## v0.430 — method cache capture release

v0.430 invalidates stale lazy Function wrappers after a successful native method
replacement, method deletion, ordinary data overwrite or native accessor
replacement. Method-table entries shadowed by published data/accessors are also
removed. Cleanup happens after fallible publication, preserving the old entry and
wrapper on OOM. Identical callback/userdata/root definitions keep Function identity.

[GC:] A retained or pinned Function owns its own callback capture. Removing the
owner's cache does not change that exported Function or its callable behavior.
An unexported wrapper no longer keeps its obsolete capture alive solely through
a pinned owner object. This applies to host and loaded contexts; existing naming,
readonly and deletion checks still run first. No public ABI change.

`gc_method_cache` first failed on v0.429. Across host/SWF4–8 it tests all four
operations with and without a pinned old Function, identical-definition identity,
actual collection, safe live-registry checks before dereferencing, old callback
invocation while pinned and release after unpin. `gc_method_cache_allocation`
sweeps every failure through success in host/SWF6/SWF8, checking old value,
Function identity and capture retention on OOM, then release on success.

This is not complete callback or GC certification. Global registration caches,
other wrapper/resource subsystems, finalizer resurrection and concurrent RPC
lifetimes require their own acceptance. All nine roadmap stages remain open.

Validation v0.430: Linux minimal 853/853; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS and complete-stage acceptance remain open.

## v0.431 — raw Array length in case-varied watchers

Getter-free watcher old-value lookup recognizes Array resource length with the
same counted Flash case rule as ordinary loaded names. SWF4–6 writing LENGTH
while watching length receives old length 3, requested name LENGTH and new value
5; returning 4 resizes the Array to four elements. SWF7–8 preserve distinct names;
host native-length watch policy is unchanged. NUL suffixes remain distinct.

`array_case_watch_length` failed on v0.430's old-value assertion, then checks
host/SWF4–8, callback arguments, actual getter-free resource value, real GC,
resize and NUL-suffix isolation. `jit_array_case_watch_length` repeats resize via
interpreted, attached and automatic native SetMember with native-entry assertions.
This supplements the pinned watcher/property-name reference; complete Array
accessor overrides, exception writeback, budgets and all stages remain open.

Validation v0.431: Linux minimal 855/855; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS execution and complete-stage acceptance open.

## v0.432 — thrown watcher assignment

v0.432 completes loaded Object/Array assignment after a script watcher throws:
the assigned value becomes undefined. A successful own write then returns the
original exception; readonly data stays unchanged and still returns the exception.
Own virtual setters run with undefined and the original watcher exception wins
over a later setter exception. Inherited virtual dispatch returns the setter's
result, consuming the watcher exception when the setter succeeds; its own throw
is propagated. Host watcher failure policy is unchanged.

The source basis is Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd:
[object.rs set_internal](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs)
and [script_object.rs call_watcher](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
The latter changes a thrown watch value to undefined; set_internal continues
assignment and treats inherited setters separately from own set_local.

[GC:] [FVM_WatchWriteScope](../src/core/watch_write_scope.hpp#L8) saves the thrown graph before clearing pending state.
Its explicit root frame protects counted strings, objects and cycles through
setter execution and collection. Destruction restores pending ownership before
unlinking the saved frame. It unwinds before the enclosing lookup-owner root.
Saving may allocate; failure before suspension leaves the original pending
exception intact. Ordinary non-throwing writes allocate no saved-value vector.

`object_watch_throw` first failed on v0.431. Host/SWF4–8 cover named data,
Object/Array numeric slots, readonly data, own/inherited script accessors, setter
success/throw and error priority. A self-cyclic object thrown by the watcher
survives a real collecting setter; pending state is suspended inside that setter.
Existing counted-string/interpreter/native watch fixtures now expect undefined
writeback for loaded throws. Raw runtime [FVM_ERROR](../include/flashvm/flashvm.h#L43) and allocation failures retain
fail-fast handling; their equivalence to reference errors is not certified.
Custom native length/resource setters, metatables, recursion limits and complete
watcher/SWF compatibility require further checks. All nine stages remain open.

Validation v0.432: Linux minimal 856/856; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other operating systems and all stages remain open.

## v0.433 — Array growth excludes inherited virtual dispatch

An indexed loaded Array write now resolves whether a virtual property is inherited
before growing length. Inherited setters, including getter-only virtual properties,
do not enter Array.set_local, so they do not grow the receiver. Own writes retain
growth before readonly checks and own setter invocation. This refines the earlier
blanket growth-before-setter policy. Pinned object.rs set_internal dispatches
inherited setters before script_object.rs set_local performs native Array growth.

`array_inherited_setter_length` failed on v0.432: an inherited index 5 setter saw
length 6 instead of 0. Host/SWF4–8 checks own/inherited dispatch, real GC, setter
throws, thrown watchers and priority/consumption rules; inherited writes leave no
own slot. The native fixture repeats all loaded cases in interpreter/attached/
automatic JIT modes, asserting native entry counts and length inside the setter.
Full Array side effects, resource-specific errors, malformed graphs and work
accounting remain open. No complete stage or platform certification is implied.

Validation v0.433: Linux minimal 858/858; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS and all-stage acceptance remain open.

## v0.434 — getter-only dispatch and deferred-error acceptance

A loaded named write through an inherited native getter-only virtual property
now consumes the watcher exception, as inherited call_setter with no setter does.
It leaves no own value and does not execute the getter. Own getter-only properties
still return the watcher exception. Numeric dispatch already followed this rule.
`object_watch_getter_only` reproduced v0.433's named-path mismatch and checks
SWF4–8, own/inherited/numeric/named variants, counted exceptions and real watch GC.

`jit_object_watch_throw` executes the complete data/readonly/own/inherited setter
matrix with cyclic thrown objects and collecting setters in interpreter, attached
and automatic JIT, asserting actual native entry counts. Allocation failures are
swept until the first completed throw-writeback in `object_watch_throw_allocation`:
host/SWF4–8 retain old data on OOM, preserve an already pending exception when
suspension or subsequent preparation fails, clear state and retry. Saved exception
payload and root/watch/setter cleanup are checked after every boundary. These
checks cover own raw writes, not every allocator failure inside a setter.
Other error mappings, resource setters, recursion/collision limits and all nine
roadmap stages remain open. Native-accessor policy remains an embedding contract.

Validation v0.434: Linux minimal 861/861; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS execution and all-stage acceptance open.

## v0.435 — borrowed definitions on super

Loaded borrowed Object.addProperty and Object.watch acknowledge valid arguments
on a super facade without storing descriptors/watchers on it. Object.unwatch
returns false for that facade. Name coercion and argument validation still run,
including errors from a coercion callback. Loaded watch rejects a super facade
as its callback: super has direct-call machinery, not ordinary Function identity.
Host behavior is retained. Direct super.method calls still bind the underlying
receiver; this change concerns explicitly borrowed methods with this = super.

`object_super_definitions` reproduced v0.434's stored accessor. It checks SWF5–8
and host, counted NUL names, accepted/rejected arguments, exact coercion counts,
coercion exceptions, descriptor/watcher absence, callback rejection and GC.
Primary references: pinned globals/object.rs add_property/watch/unwatch and
script_object.rs add_property_with_case/watch/unwatch, which decline mutation
of NativeObject::Super. Full facade reflection, native definition APIs, accessor
callback projection and all roadmap stages remain open.

Validation v0.435: Linux minimal 862/862; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS execution and all-stage acceptance open.

## v0.436 — super is not a virtual Function callback

Loaded script-property getter/setter dispatch now excludes super facades from
ordinary Function callbacks. A non-function getter uses retained property data;
a non-function setter has no executable side effect and writable data still
updates. The facade remains valid as an Object argument to addProperty. Host
behavior and explicit direct superclass invocation remain unchanged. Pinned
script_object.rs call_getter/call_setter require as_function before execution;
Property retains data independently of getter/setter objects.

`object_super_accessors` first failed on v0.435's retained-data read. Across
host/SWF5–8, a constructor side effect detects accidental parent execution during
get/set, real GC checks descriptor-held facade graphs, direct super still executes
the parent, and queue transfer to another context repeats the retained-data and
side-effect checks after GC. `jit_object_super_accessors` executes GetMember and
SetMember in interpreter, attached and automatic JIT, asserting native entries.
This covers ordinary descriptor dispatch, not complete super/reflection or RPC
semantics. Callback/resource equivalence and all nine stages remain open.

Validation v0.436: Linux minimal 864/864; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS and all-stage acceptance remain open.

## v0.453 — resolver status and original native interfaces

Resolver discovery now reports presence separately from callback status. An
actual [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47) from a resolver, returned method or revoked RPC interface
propagates unchanged; it cannot masquerade as an absent hook or trigger fallback.
Existing interpreter/JIT tests cover resolver and callee NOT_FOUND, including
super dispatch and callback counts. object_resolve_rpc checks SWF6/8 with two
independent runtimes, source context served exclusively by the broker thread,
original native callback/userdata, cyclic receivers, counted NUL/Unicode names,
exceptions, unsupported/not-found errors, recovery and revocation over one port.
NO_OS_PORTS skips the socket-specific test; it is not network evidence there.

The C89 property_resolve embedding tutorial returns an original C Function for
an absent sum method, checks GC capture ownership and prints sum = 42. Its RU/EN
READMEs explain building/running and link the separate process/thread/LAN RPC
tutorial. This does not establish full distributed graph or variable resolution
semantics. All nine stages and foreign OS acceptance remain open.

Validation v0.453: complete Linux minimal 895/895; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 7/7 selected checks each. The socket
case is skipped in NO_OS_PORTS. Executable-mode launchers applied; stack ASan
and LeakSanitizer disabled. Other OS runs and all stages remain open.

## v0.454 — bare global scope excludes missing-property resolvers

Loaded ordinary variable lookup checks global property presence before reading.
Missing bare names no longer execute global __resolve; explicit C/global member
reads retain resolver behavior. Registered native fallback remains available,
while inherited undefined is an existing binding and suppresses that fallback.
The unloaded host path is unchanged. Reference: Ruffle scope.rs resolve at
[2372a4e](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/scope.rs).
object_resolve_global_scope first failed on the previous callback invocation;
it checks SWF4–8, direct lookup/GetVariable/CallFunction, explicit global reads,
native fallback, inherited undefined and interpreter/attached/automatic JIT with
actual native entries. This is scope exclusion, not full scope compatibility.

Validation v0.454: Linux minimal 896/896; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 targeted checks each. Stack ASan/LeakSanitizer
disabled; all stages and foreign OS acceptance remain open.

## v0.455 — global undefined retains binding identity

The C global getter snapshots property-chain presence before invoking a getter.
Own/inherited undefined and a getter deleting its own binding suppress registered
native fallback for that read. A later read after deletion can find the native
Function. Versioned property selection is used instead of an exact own-data-only
map test. Missing-property resolver behavior remains available in this explicit
C API. global_undefined_binding reproduced the old fallback and checks host and
SWF4–8, own/inherited data, native getters, deletion and folded spelling. This
is a C namespace precedence fix, not complete global/variable compatibility.

Validation v0.455: complete Linux minimal 897/897; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 3/3 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.456 — native global names use loaded case selection

Registered native global reads, ordinary variable fallback and direct C/global
member calls use the existing versioned property selector: SWF4–6 fold UTF-16
case; host and SWF7–8 stay exact. Wrapping uses the stored registration name so
aliases share one Function identity. global_native_case first failed on an alias
read; it checks host/SWF4–8, ASCII and counted NUL/Unicode, lookup/call APIs,
callback counts and CallFunction in interpreter/attached/automatic JIT. This
covers reads/calls; registration collision/update policy remains separate.

Validation v0.456: complete Linux minimal 898/898; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 3/3 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.457 — snapshot direct global-call arguments

[fvm_call_bytes](../include/flashvm/value.h#L98) copies arguments into a rooted vector before clearing the output
or resolving a global Function. A result may alias an argument, and callbacks
may mutate the caller's value array without changing the active invocation.
Caller-owned external string buffers must still remain valid for the call.
Registered native callbacks retain their normal GC barrier.
global_call_arguments first failed after caller mutation; host/SWF4–8 check
native/script/getter/resolver routes, counted NUL names and traced strings,
cyclic unpinned arguments, aliased outputs, caller mutation, collecting callbacks,
plain errors/exceptions and root/depth cleanup/reclamation. Host skips __resolve.
This does not establish complete C embedding or concurrent-context safety.

Validation v0.457: complete Linux minimal 899/899; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 3/3 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.458 — replace native registrations without stale capture roots

Native registration updates retain the selected existing spelling in SWF4–6.
Publication removes a cached wrapper only when callback, userdata, capture or
collector-entry policy changed. An identical registration keeps Function identity;
an explicitly saved old Function retains its original callback/capture. Stale
cache entries no longer keep an otherwise unreachable old capture alive.
The stored name is copied before publication, so allocation failure preserves
the previous registration and cache. Host/SWF7–8 alternate names remain distinct.
global_native_replacement reproduced the stale root and covers versioned updates,
real GC, cycles, saved old Functions, new callbacks and identical registration.
global_native_replacement_allocation sweeps new/new[] boundaries through first
successful replacement, checks old identity/root retention on OOM and retry/GC.
This is not a full allocator/concurrent registration proof; all stages remain open.

Validation v0.458: complete Linux minimal 901/901; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.459 — wrapper identity includes collector-entry policy

Cached native Function reuse compares collector_entry as well as callback,
userdata and capture. New reads after an internal policy change get a wrapper
with the current policy; saved Functions retain their original policy. Public
native callbacks keep the normal collection barrier. native_wrapper_policy first
failed on stale cache reuse and checks host/SWF4–8, global/method wrappers,
ordinary-to-trusted-to-ordinary changes, saved Function invocation, actual GC
from the trusted fixture and clean roots/depth. The internal fixture does not
expose a public barrier opt-out or establish full concurrent collector safety.

Validation v0.459: complete Linux minimal 902/902; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.460 — root named-constructor arguments before global lookup

[fvm_construct_bytes](../include/flashvm/value.h#L155) snapshots and roots caller arguments before resolving a
constructor name. Global getters/resolvers can collect or mutate the caller's
array before Function construction; registered C factories also receive the
stable snapshot. The ordinary native barrier remains. Constructor output is
still an Object pointer; no overlapping output-pointer contract is added.
global_construct_arguments first failed after a native factory mutated the
caller array. Host/SWF4–8 cover native/script/getter/resolver factories, counted
NUL names/strings, cyclic unpinned arguments, mutation, collecting lookup/body,
object results, plain errors/exceptions and root cleanup/reclamation. Host skips
__resolve. Complete constructor/allocator/concurrency compatibility remains open.

Validation v0.460: complete Linux minimal 903/903 and AGG 922/922; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.461 — native constructor names use versioned selection

Native constructor lookup and replacement use the same loaded case selector as
properties/native global functions. SWF4–6 alternate spelling selects and updates
the existing registration; host/SWF7–8 remain exact. All allocating stored-name
copying occurs before publication, retaining the old registration/capture on OOM.
native_constructor_case reproduced unsupported alias lookup and checks host,
SWF4–8, ASCII/Unicode/NUL, registration count, factory results and cyclic capture
reclamation after replacement. native_constructor_replacement_allocation sweeps
new/new[] boundaries through first success, verifies the old factory and root on
failure, then retry and GC. This is the native embedding registry, not completion
of ActionNewObject, registerClass, allocators or concurrent context access.

Validation v0.461: complete Linux minimal 905/905. AGG passed 923/924 in
the full run; the remaining timezone driver failed to launch its binary with
PermissionError after an executable-mode reset. Restoring that mode and rerunning
the driver passed 1/1, covering all 924 cases across these runs. Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.462 — direct Function construction snapshots arguments

[fvm_function_construct](../include/flashvm/function.h#L69) retains an unpinned input Function and snapshots/roots
arguments before native factory dispatch or script prototype access. A prototype
getter may collect or mutate the caller's array before constructor invocation.
External borrowed string buffers remain caller-owned for the complete call;
ordinary native callbacks retain their GC barrier. No overlapping output-pointer
contract is introduced. function_construct_arguments first failed after native
factory mutation and checks host/SWF4–8, direct script/native construction,
collecting prototype getters, traced NUL strings, cyclic arguments, mutation,
object returns, errors/exceptions and root/depth cleanup/reclamation. The native
constructable entry is an internal fixture for built-in factory dispatch.
Full constructor/allocator/concurrency compatibility and all stages remain open.

Validation v0.462: complete Linux minimal 906/906; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.463 — validate direct native factory results

Direct Function construction initializes the native factory result to undefined
and publishes only a non-null Object belonging to the calling context. Untouched,
scalar, null-Object and foreign-context results produce ERROR with a null output;
actual callback errors/exceptions remain unchanged. This matches the existing
named-factory object ownership boundary. function_construct_native_result first
failed on an invalid Object and checks host/SWF4–8, valid/invalid output shapes,
untouched output, callback errors, retained exception payloads and GC/root cleanup.
This validates well-formed API pointers, not arbitrary invalid host addresses.
All constructor, allocator, concurrency and foreign OS acceptance remains open.

Validation v0.463: complete Linux minimal 907/907; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 3/3 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.464 — invoke the retained Function argument snapshot

Direct [fvm_function_invoke](../include/flashvm/function.h#L65) now passes its retained copy to the callback, rather
than the caller's mutable array. The output is cleared after copying, so it may
alias any argument. Validation/OOM errors still clear the output; external string
buffers remain caller-owned. Existing single-frame root layout and native barrier
policy are retained. function_invoke_arguments first failed after callback mutation
and checks host/SWF4–8, native/script/trusted-internal callbacks, implicit global
or explicit unpinned receiver, both aliased argument slots, cyclic inputs, traced
NUL strings, actual GC where permitted, retained successful strings, errors,
pending cyclic exception payloads and post-call reclamation/root cleanup.
Full C embedding, allocator and concurrent collector acceptance remains open.

Validation v0.464: complete Linux minimal 908/908; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. The duplicate
error-output clear was removed afterwards and the invocation test reran successfully.
Stack ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.465 — loaded script constructors ignore returned values

Loaded AVM1 script construction returns its newly allocated receiver regardless
of the script return value. Only the native factory path propagates its Object
result. The unloaded host namespace retains its previous Object substitution.
This follows FunctionObject::construct in pinned Ruffle
[function.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/function.rs).
function_constructor_return first reproduced replacement of the allocated this.
Host/SWF4–8 test real script bytecode returning undefined/Number/Object/Function/
Array/this, constructor side effects, direct/named C construction, NewObject and
NewMethod in interpreter/attached/automatic JIT with actual native entries and GC.
Two earlier argument-lifetime tests now expect the allocated loaded receiver;
their argument/mutation/GC assertions remain. Full class construction, constructor
identity properties, native callable construction and all stages remain open.

The first full run found two older boxing fixtures relying on script return
substitution. TargetPath now expects undefined for the ignored returned clip and
retains a direct MovieClip positive case; the instanceof callback links the tested
prototype to the actual allocated receiver. GC, exception and callback-count
assertions remain. Final validation v0.465: complete Linux minimal 909/909; AGG,
Linux-hosted NO_OS_PORTS and heap/global ASan+UBSan 8/8 targeted checks each.
Stack ASan/LeakSanitizer disabled; all stages and foreign OS acceptance open.

## v0.466 — script instance constructor identities

Before a loaded script constructor body runs, the new receiver gets own
__constructor__ and, before SWF7, constructor properties pointing to the callee.
Flags are DONT_ENUM only: both are writable/deletable. Forced stored definitions
bypass inherited setters/watchers. A shared private helper stages property/flag
maps before publication and removes exact obsolete own virtual slots afterwards;
OOM preserves the observable receiver's previous metadata. The unloaded host
namespace is unchanged. Reference: FunctionObject::define_constructor_props in
[pinned function.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/function.rs).
function_constructor_identity reproduced missing own metadata and checks host/
SWF4–8, pre-body visibility, inherited accessor traps, flags, hidden enumeration,
write/delete, real GC, serialized graph identity without destination installation,
and actual NewObject entries in interpreter/attached/automatic JIT. Registered
MovieClip construction, native factory identities and all stages remain open.

Validation v0.466: complete Linux minimal 910/910; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 6/6 targeted checks each. Stack ASan/LeakSanitizer
disabled; all stages and foreign OS acceptance remain open.

## v0.467 — registered MovieClip constructor identities

Attach, duplicate and authored sprite placement now define constructor identities
before invoking a registered class, after initObject processing. The explicit
symbol-movie version selects the pre-SWF7 own constructor property, independently
of ambient SWF and destination context; old libraries keep ambient fallback.
SWF7+ initObject constructor data remains untouched, while __constructor__ and
legacy constructor are forced hidden identities. Constructor exceptions retain
the published clip and metadata. Unregistered/default clip construction is not
changed. Reference: post_instantiation / construct_on_existing in pinned Ruffle
[display_object/movie_clip.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object/movie_clip.rs).
movieclip_constructor_identity reproduced missing pre-body metadata. It checks
source SWF5–8 vs ambient6/8, both registration environments, init collisions,
attach/duplicate/authored placement, callback exceptions, GC and graph transfer
without destination standard installation. constructor_identity_allocation sweeps
new/new[] failures through first success for SWF4–8 data/accessor/native-method
slots, preserving old values/flags/cache identity/captures on OOM, retry, forced
replacement, no callbacks, counted strings and release of obsolete cyclic roots.
This is not full display lifecycle or all-allocator/concurrent acceptance.

Validation v0.467: complete Linux minimal 912/912 and AGG 931/931; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 6/6 selected checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.468 — stored script constructor prototypes

Loaded Function construction reads own stored prototype data, skipping accessors,
inherited fields and __resolve. The shared class-prototype reader now includes
retained script-virtual data, so instanceof/Implements use the same raw value.
A C native accessor without retained data yields undefined. Host construction
keeps ordinary prototype lookup. Explicit fn.prototype still executes getters.
Reference: ScriptObject::prototype/get_data in pinned Ruffle
[script_object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
function_constructor_stored_prototype reproduced the wrong constructed prototype.
It checks host/SWF4–8, retained Object/primitive virtual data, throwing getters,
native accessors, inheritance, legacy spelling, resolver exclusion, GC, instanceof
and actual NewObject entries in three modes. The old NewObject fixture now checks
that explicit reads invoke its getter while construction does not; all argument,
GC and exception assertions remain. This supersedes earlier assumptions about
loaded prototype-getter execution, while their argument snapshots remain needed.
Native-method-backed prototype fields, registered clip prototype selection and
full class compatibility remain open.

Validation v0.468: complete Linux minimal 913/913; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 8/8 targeted checks each. Stack ASan/LeakSanitizer
disabled; all stages and foreign OS acceptance remain open.

## v0.469 — registered clip stored prototypes use source-movie policy

Known-SWF registered clip construction reads the class's own stored prototype
value, bypassing accessors/inherited fields/resolvers. Retained virtual Object
data remains the live prototype; absent/primitive data yields no Object prototype
instead of rejecting the attachment. The raw field selector accepts an explicit
source-movie version, so SWF6 alternate spelling works under an ambient SWF8 and
SWF8 remains exact under SWF6. Unknown-version unloaded host libraries retain
ordinary lookup. Reference: post_instantiation's constructor.prototype call in
pinned display_object/movie_clip.rs and ScriptObject::prototype/get_data.
movieclip_constructor_stored_prototype reproduced the incorrect receiver chain.
Source6/8 × ambient6/8 cover retained virtual Object/Number, native accessor,
inherited/mixed/missing fields, direct primitive data and throwing getter/resolver
traps across attach/duplicate/authored placement, callback counts, GC and clean
state. Transfer to an opposite-version context without standard installation
checks the new clip against the independently cloned existing clip's prototype;
the custom C native accessor case is excluded from graph transfer. The original
script attachment is dropped, as required by the existing graph contract.
This does not certify primitive __proto__ reflection, native-method-backed
prototype fields, all class lifecycle behavior or any foreign OS.

Validation v0.469: complete Linux minimal 914/914 and AGG 933/933; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 6/6 selected checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

### v0.470 — own native-method class prototype bridge

Getter-free own prototype reads now materialize the same cached Function value
as public member reads for native-method-backed fields, without invoking the
callback. Data/accessor precedence, source-version spelling and own-only lookup
remain unchanged. The helper returns [FVM_Result](../include/flashvm/flashvm.h#L40) so wrapper failures propagate
through construction, InstanceOf, ImplementsOp and registered clip selection.
Extended function_constructor_stored_prototype covers cold/warm caches, host
and SWF4–8, GC, instanceof and three NewObject modes. Extended
movieclip_constructor_stored_prototype covers source6/8 × ambient6/8,
attach/duplicate/authored placement and zero callback invocations. This custom
C method is excluded from graph-transfer assertions. Extends deliberately
retains ordinary superclass.prototype reads, as pinned Ruffle action_extends
requires. Primitive __proto__ reflection, all stages and foreign OS acceptance
remain open.

Validation v0.470: complete Linux minimal 914/914 and AGG 933/933; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 selected checks each. Stack
ASan/LeakSanitizer disabled.

### v0.471 — loaded Number receiver before ToNumber

Loaded Number now allocates its receiver using own stored prototype data and
forced constructor identities before converting its input. Prototype accessors
are not invoked, primitive/missing prototype data gives no Object prototype,
and a valueOf mutation cannot replace the selected chain. The unloaded host
path keeps its existing lookup order. A shared private allocator roots the
class/receiver and publishes only a fully prepared object. Reference: pinned
Ruffle FunctionObject::construct/define_constructor_props and number.rs
constructor, which initializes the preallocated this.
number_constructor_stored_prototype covers SWF4–8, direct/named C construction,
three NewObject modes, valueOf mutation, native getter/method, primitive, absent
and mixed spelling. The native factory barrier rejects in-callback collection;
post-return GC is exercised. native_instance_allocation sweeps new/new[] failures
across a cold wrapper and receiver metadata, checking null output, clean state,
GC and retry. Other native constructors, including String/Boolean, require
separate validation; all stages remain open.

Validation v0.471: complete Linux minimal 916/916 and AGG 935/935; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 selected checks each. Stack
ASan/LeakSanitizer disabled; foreign OS acceptance remains open.

## v0.472 — loaded String and Boolean receiver preparation

Loaded String and Boolean now use the same prepared-receiver allocator as Number:
select own stored prototype data and define hidden constructor identities before
conversion. Primitive/missing prototype data yields an empty Object chain;
accessor callbacks and inherited lookup do not select the prototype. A String
argument's toString mutation cannot change the already selected receiver chain.
String keeps counted text and its own nonenumerable/nondelete length. Boolean
keeps versioned truth conversion without invoking object coercion callbacks.
Unloaded host constructor lookup order remains unchanged.

Reference: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
FunctionObject::construct/define_constructor_props and globals/string.rs and
boolean.rs constructors, which populate the preallocated receiver.
string_constructor_stored_prototype and boolean_constructor_stored_prototype
cover SWF4–8, direct/named C construction and NewObject/NewMethod in interpreter,
explicit native and automatic JIT modes, native accessor/method, stored script-accessor
data, inherited/missing/primitive/mixed-case prototype, skipped __resolve,
identities and post-return GC. String additionally checks
mutation during coercion, embedded NUL text/length, errors and exceptions.
native_instance_allocation now sweeps new/new[] failure boundaries through actual
Number/String/Boolean factories, including length publication, cold method
wrappers, null failure output, clean root/native scopes, collection and retry.
The native barrier still rejects collection inside native factories. Both new
regressions fail against the original v0.471 implementation (missing constructor
identity), and pass after the fix. This does not close constructor
compatibility or any roadmap stage.

### Русский

Загруженные String/Boolean выбирают собственный сохранённый prototype и получают
скрытые свойства конструктора до преобразования аргумента. Getter не вызывается;
примитивное/отсутствующее значение даёт пустую Object-цепочку. Изменение prototype
из toString не меняет выбранную цепь String; сохраняются NUL и собственный length.
Boolean сохраняет правила truth по версии SWF и не вызывает методы объекта.
Проверяются SWF4–8, C API, три режима NewObject/NewMethod, GC после возврата, ошибки и
исключения String и отказы выделения памяти трёх фабрик. Полная совместимость
конструкторов, другие ОС/CPU и все стадии остаются открытыми.

Validation / Проверка v0.472: complete Linux minimal 918/918 and AGG 937/937;
selected heap/global ASan+UBSan 4/4 (stack ASan and LeakSanitizer disabled).
Unicorn 2.1.4: all 26 synthetic encoder success/error checks passed for x86,
ARM/Thumb, ARM64, RV32 and RV64, including far branches and high helper addresses.
These are synthetic ABI checks, not full VM execution on foreign CPUs/OSes.

The final Boolean callback-trap refinement was rerun in minimal, AGG and sanitizer
profiles. Interrupted build outputs/executable permissions were repaired before
the final complete native runs; no behavior failures remain in those runs.

## v0.473 — loaded builtin construction and function-call semantics

Date/Error/Object/Array prepare stored prototypes and hidden constructor identities
before coercion/setters. Date snapshots its receiver before numeric projection;
Error setters see complete metadata. Loaded Object() is bare for nullish inputs;
Array() uses its retained builtin prototype; Error() initializes its caller and
returns undefined, and Error.toString returns raw message data. Appended callback
IDs preserve invocation distinctions across graph transfer. Internal Function
metadata arrays no longer execute public Array.prototype getters.

Nine targeted regressions reproduce failures on v0.472 and pass after the fixes;
seven factories now have cold-wrapper allocation-failure sweeps. See
[BUILTIN_CONSTRUCTION.md](BUILTIN_CONSTRUCTION.md) for tests, reference sources,
callback/GC policy, transfer fallback and remaining numeric-length/boxing limits.
Все стадии остаются открытыми: Date/Error/Object/Array получают prototype и
метаданные до callbacks; разделены обычные вызовы, исправлен raw message Error
и исключён getter Array.prototype из метаданных Function. Полная совместимость,
другие фабрики и реальные запуски других CPU/ОС не заявлены.

Validation v0.473: complete Linux minimal 926/926 and AGG 945/945; nine
selected heap/global ASan+UBSan checks passed (eight together, then the new
Function2 metadata check). Stack ASan and LeakSanitizer remain disabled.
Three scoped constant headers passed a strict C89 syntax probe. Executable
permissions were repaired before final native runs. A one-record malformed-wire
fixture explicitly strips new constructor edges; normal roundtrip tests keep them.
Foreign CPU/OS acceptance and all nine roadmap stages remain open.

Проверка v0.473: полные Linux minimal 926/926, AGG 945/945; девять выбранных
проверок heap/global ASan+UBSan прошли. Stack ASan/LeakSanitizer отключены.
Три новых заголовка констант прошли C89. Приёмка других CPU/ОС и все стадии открыты.

## v0.474 — primitive boxing uses current global constructors

Loaded Object(value), Function.call/apply share AVM1 current-global boxing instead
of stale native-registry/captured constructors. Missing/unusable bindings return
a bare Object for Object conversion and the global receiver for call/apply.
Inherited/case-selected bindings work; resolver lookup stays excluded; getter and
constructor errors/GC obey existing native/trusted barriers. Host behavior and
wire IDs stay unchanged, including transferred host adapters in loaded contexts
without destination standard installation. See [PRIMITIVE_GLOBAL_BOXING.md](PRIMITIVE_GLOBAL_BOXING.md).
Four drivers reproduce failures on v0.473: host/SWF4–8 matrices, three primitive
types/fourteen binding forms, all Action routes, real native entries, GC/exception
lifetime, transfer and new/new[] allocation sweeps. Full compatibility remains open.

Object(value) и call/apply в SWF используют текущие глобальные классы примитивов.
Fallback — новый bare Object либо глобальный this; __resolve пропускается.
Проверены подмены, getters, исключения, GC, перенос и OOM. Другие CPU/ОС и все
девять стадий остаются открытыми.

Validation v0.474: complete Linux minimal 930/930. AGG passed 935/949 in
the full run; 14 binaries could not launch after executable-mode resets.
After restoring their modes, the selected rerun passed 14/14: all 949 checks
have passing evidence across these runs. Four selected heap/global ASan+UBSan
checks passed, including the full allocation sweep. Stack ASan and LeakSanitizer
remain disabled. Generated logs and baseline binaries stay outside committed
source. No new foreign-OS/device or full foreign-CPU run is claimed.

Проверка v0.474: Linux minimal 930/930; все 949 AGG-проверок прошли в полном
прогоне и повторе 14 запусков после восстановления прав исполнения. Выбранные
heap/global ASan+UBSan — 4/4. Stack ASan/LeakSanitizer отключены; все стадии открыты.

### v0.475 — Function class call boundary

RU: Исправлены вызовы `Function()` для SWF 4–8 и создание без аргументов;
первый аргумент обычного вызова сохраняет тип и значение. Отдельный callback
сериализуется под ID 809. Host без SWF сохраняет прежнюю пустую функцию.
AVM1 сохраняет примитив из `new Function(value)`; object-only C API по-прежнему
отклоняет такой результат.

EN: Loaded Function calls preserve the first argument, or create a bare object
when absent. Empty construction uses the stored prototype and constructor
metadata, without creating a callable script body. Unloaded host behavior is
preserved. AVM1 native construction preserves primitive results; public object-only C
construction still rejects them.

### v0.476 — Function/Error prototype attributes

RU: call/apply в загруженном SWF имеют флаги DONT_ENUM|DONT_DELETE;
Error.toString — обычные флаги 0. Переход от host к SWF атомарно изменяет только
канонические native-методы с исходными флагами; замены, удалённые свойства,
аксессоры, изменённые атрибуты и заменённые прототипы сохраняются.

EN: Loaded Function adapters use nonenumerable/nondeletable writable slots;
Error.toString is enumerable, deletable and writable. Host installation defaults
remain unchanged. Flag activation prepares copied maps before publication and
preserves custom state. Prototype serialization retains the resulting attributes.
This is focused compatibility evidence, not full standard-library acceptance.

### v0.477 — isPrototypeOf argument conversion and ToObject normalization

RU: Проверяется преобразование аргумента isPrototypeOf через текущие глобалы,
включая аксессоры, наследование, регистр SWF4–6 и ошибки. ToObject отбрасывает
необъектные результаты нативных конструкторов. Без SWF поведение прежнее.

EN: isPrototypeOf boxes supplied primitives using current globals, preserving
side effects and failures. ToObject normalizes successful nonobject native
constructor results to undefined; NewObject/NewMethod retain raw native values.
Tests: object_prototype_global_boxing, to_object_native_primitive,
object_prototype_boxing_allocation. All roadmap stages remain open.

### v0.478 — watch callback object projection

RU: watch в SWF преобразует callback через текущий глобальный конструктор после
имени. Возвращённая Function становится наблюдателем; остальные результаты,
ошибки и исключения не заменяют старый обработчик. Host-контракт прежний.

EN: Loaded watch performs current-global callback boxing in name-first order.
Actual Function results are rooted and installed with userData, survive clearing
the boxing global and GC, and travel with the watched graph. Tests cover 14 global
variants, C/saved-method/three AVM1/JIT routes, ordering and OOM replacement.
Native registration keeps its collector barrier. Full version visibility,
resource transport and roadmap acceptance remain open.

### v0.479 — ASSetPropFlags projection and numeric operands

RU: Проверены текущие boxing-глобалы для цели, AVM1-преобразования set/clear,
порядок target→set→clear→names, ошибки, исключения, встроенный NUL и OOM без
частичного изменения атрибутов. Насыщение UInt16 не добавляет поддержку битов >7.

EN: ASSetPropFlags now projects loaded targets, evaluates set/clear through AVM1,
then converts the property list. Fractional/negative/NaN narrowing follows the
pinned reference within the supported mask. Extended version flags remain
rejected. Tests: property_flags_global_boxing/conversion/allocation.
See PROPERTY_FLAGS.md; all roadmap stages remain open.

### v0.480 — ASSetPropFlags own-name selection

RU: В SWF4–8 список имён ASSetPropFlags сравнивается без учёта регистра;
атрибуты сохраняют написание выбранного собственного поля. Проверены данные,
методы, аксессоры без вызова getters, NUL/Unicode, дубли, наследование,
перенос в SWF8 и атомарность при OOM. Обычный lookup не менялся.

EN: Flag-only lookup now folds every loaded version and stores canonical own
names, preserving attributes across case-sensitive graph import. Host matching
remains exact. Tests: property_flags_case and property_flags_case_allocation.
Case-collision ordering and extended version flags remain unverified/unsupported;
this does not close any roadmap stage.

### v0.481 — Math property metadata

RU: Методы/константы Math в SWF4–8 имеют атрибуты 7. Нетронутые host-поля
переводятся атомарно при первой загрузке; замены, аксессоры, изменённые флаги,
удалённые поля и заменённый Math не перезаписываются. Повторная загрузка
сохраняет защиту, явно снятую через ASSetPropFlags. Числовые вычисления прежние.

EN: Math metadata now protects all 26 loaded fields, with prepared first-load
activation and preservation of custom host/reload state. Tests:
math_loaded_attributes, math_custom_attributes, math_activation_allocation.
Transport preserves flags; numerical exactness and all roadmap stages remain open.

### v0.482 — сохранение встроенного состояния при reload / builtin reload state

Переход host → loaded теперь выполняется один раз: повторная загрузка SWF4–8
сохраняет изменённые флаги встроенных прототипов. Проверены все пары версий,
C/interpreter/explicit/auto JIT, GC и отказ выделения памяти с повтором.
See [BUILTIN_RELOAD.md](BUILTIN_RELOAD.md). This closes the repeated attribute
activation regression only; all nine stages and full movie lifecycle semantics
remain open.

### v0.483 — фиксированные Math constants / fixed Math constants

Loaded Math uses canonical binary64 constants, correcting LOG10E's libm-derived
one-bit rounding discrepancy. Host values and custom state remain preserved;
first-load updates are transactional under OOM. Проверены побайтовые значения,
C/AVM1/JIT, перенос графа, GC и повтор после отказа памяти.
See [MATH_ATTRIBUTES.md](MATH_ATTRIBUTES.md). Exact Math functions, cross-platform
runtime acceptance and all nine roadmap stages remain open.

### v0.484 — class attributes independent of prototypes

Number constants and Date.UTC now activate independently of customized public
prototypes. Сохраняются изменённые поля/флаги, primitive metatable и прототипы;
проверяются OOM, GC, перенос и C/AVM1/JIT. See
[STATIC_CLASS_ACTIVATION.md](STATIC_CLASS_ACTIVATION.md). This is a first-load
metadata fix, not completion of stage 1 or any other roadmap stage.

### v0.485 — independent String/Object class activation

First-load String.fromCharCode and Object.registerClass attributes now activate
independently of public prototype state. Подготовка атомарна при OOM, getter
prototype не вызывается, изменённые поля/флаги и reload сохраняются.
See [STATIC_CLASS_ACTIVATION.md](STATIC_CLASS_ACTIVATION.md). All nine stages
remain open; full customized prototype and platform acceptance is not claimed.

### v0.486 — independent prototype method attributes

Number/Date/String/Array now prepare each canonical native method's default
SWF attributes independently of optional prototype payload conversion.
Изменённый anchor-метод не мешает остальным: проверяются пользовательские
флаги/data/accessor, OOM, GC, перенос и C/AVM1/JIT. See
[PROTOTYPE_METHOD_ACTIVATION.md](PROTOTYPE_METHOD_ACTIVATION.md). Customized
payload/layout policy is preserved; all nine roadmap stages remain open.

### v0.487 — UInt16 ASSetPropFlags and version visibility

Реализованы UInt16-атрибуты и маски видимости SWF4–8 для обычных свойств,
getter, нативных методов, __resolve и super; запись снимает версионные биты,
перенос сохраняет их. Membership/enumeration и raw Array имеют отдельные правила.
See [PROPERTY_VERSION_FLAGS.md](PROPERTY_VERSION_FLAGS.md) for tests, embedding
contracts and old-reader limitations. The builtin version-annotation audit,
intrinsic field interactions and all nine roadmap stages remain open.


### v0.488 — exhaustive property attribute regression coverage

Добавлены полный перебор 65 536 UInt16-наборов атрибутов для SWF4–8,
проверки записи/readonly/удаления и повторных вызовов интерпретатора,
явного и автоматического JIT. See [PROPERTY_VERSION_FLAGS.md](PROPERTY_VERSION_FLAGS.md).
This strengthens regression evidence for stages 1/7/8; it adds no new player
feature and closes none of the nine stages. Foreign OS/architecture runtime
acceptance and the builtin version-annotation audit remain open.


### v0.489 — signed Array constructor length

Загруженные Array(number)/new Array(number) используют clamped Int32,
включая дробные/отрицательные значения, NaN и бесконечности. Знаковая длина
сохраняется в native storage и графовом переносе, читается без unsigned-cast;
проверены readonly, рост индексов, GC и OOM/retry. See
[ARRAY_SIGNED_LENGTH.md](ARRAY_SIGNED_LENGTH.md) for RU/EN contracts and limits.
Raw unsigned length assignment remains distinct. Negative native end/splice
operations, virtual length and all nine roadmap stages remain open.


### v0.490 — native Array operations on negative length

Native push/pop/shift/unshift теперь используют знаковую длину, raw данные,
рост до записи и последующее сокращение length с отдельными readonly/nondelete
правилами. Проверяются virtual data, отсутствие getter/setter/watch вызовов,
версионные флаги, GC/перенос, C/AVM1/JIT, OOM и бюджет. See the v0.490 section
of [ARRAY_NEGATIVE_ENDS.md](ARRAY_NEGATIVE_ENDS.md). Negative native splice,
signed overflow and all nine roadmap stages remain open.


### v0.491 — native negative splice and captured-bound routing

Native splice поддерживает отрицательную длину и знаковые ключи. Captured
negative bounds принудительно выбирают generic путь даже после coercion,
изменившего живую длину, или при unsigned raw storage. Проверяются readonly/
nondelete, virtual data, version bits, C/AVM1/JIT, GC/перенос, OOM и бюджет.
See [ARRAY_NEGATIVE_SPLICE.md](ARRAY_NEGATIVE_SPLICE.md). Signed overflow,
arbitrary/virtual native length and all nine roadmap stages remain open.


### v0.492 — retained numeric Array length

Loaded numeric length assignment retains fractional, signed, nonfinite and
out-of-Uint32 Numbers, with separate wrapping Int32 traversal/truncation bounds.
Сохраняются primitive metatable/GC-циклы и графовый перенос; readonly/nondelete,
version flags и методы, записывающие/не записывающие length, проверены отдельно.
See [ARRAY_NUMERIC_LENGTH.md](ARRAY_NUMERIC_LENGTH.md) for RU/EN contracts,
coverage and wire-reader compatibility. Nonnumeric/virtual length and all nine
roadmap stages remain open.


### v0.493 — budgeted fast native splice staging

Fast loaded splice now charges stored-index copying and inserted arguments,
sharing the AVM1 budget and worker cancellation scope. Проверены C/direct invoke/
интерпретатор/JIT, sparse/ignored tails, real worker stop, staged-source preservation,
GC и OOM/retry. See [ARRAY_SPLICE_BUDGET.md](ARRAY_SPLICE_BUDGET.md).
Argument-coercion side effects remain visible after a staging error. Generic
mutations retain their existing partial-write policy; all nine stages remain open.

### v0.526 — Android parallel test transport

Tested / проверено: unique per-invocation executable staging with shared fixture
CWD, cleanup after partial push/execution failure, preserved remote nonzero exit
status, and failure on cleanup errors after successful execution. Nine mocked
host tests include overlapping invocations of the same target. See
PLATFORM_VALIDATION.md / PLATFORM_VALIDATION.ru.md. No new SWF behavior; actual
Android/Windows/macOS acceptance and all nine completion stages remain open.

### v0.527 — typed native Array length

Implemented / реализовано: original native Array.length retains String, Boolean,
null, undefined and Object, with separate AVM1 Int32 truncation, readonly/version
policy, rooted coercion and graph identity. Tests: array_typed_length and roots,
allocation and wire variants; see [RU/EN contract](ARRAY_TYPED_LENGTH.md).
Deleted/recreated/virtual length assignment and full roadmap acceptance remain open.

### v0.528 — exact recreated/virtual length writes

Implemented / реализовано: Array length preparation before readonly/setters,
virtual length definition, retained raw data and live descriptor dispatch.
Tests: array_length_reassignment and roots/allocation variants; see
[RU/EN contract](ARRAY_LENGTH_REASSIGNMENT.md). All nine roadmap stages remain open.

### v0.529 — original native length descriptor reentry and GC

Tested / проверено: live descriptor dispatch after original native coercion,
obsolete cache clearing on deletion, independent alias/shared-edge controls,
C/interpreter/JIT and errors. Minimal 1095/1095 after generated-ELF permission
repair/rerun; nine targeted ASan/UBSan cases (stack instrumentation on, LSan off).
See [RU/EN contract](ARRAY_LENGTH_CACHE_GC.md). All nine stages remain open.

### v0.530 — recorded v0.529 Linux matrix

Tested / проверено: minimal 1095, AGG/Cairo 1115 each; fresh 26-case Unicorn
gates and direct ELF audits per profile; nine targeted ASan/UBSan cases. See
[RU/EN evidence and scope](LINUX_CHECKPOINT_V529.md). Documentation-only version;
full SWF compatibility and real foreign-OS acceptance remain open.

### v0.531 — build-selected automatic JIT

Implemented / реализовано: DEFS JIT seeds new contexts with automatic native
compilation and interpreter fallback; INTERPRETER retains the manual default.
Explicit options, independent contexts and destination graph policy remain
available. Tests / проверки: execution_build_default and execution_modes;
30 definition configurations. Native receiver frame metadata also fixes
BitmapData.loadBitmap resource selection; its GC/transport scenarios run in
three modes. See [RU/EN contract](JIT_BUILD_DEFAULT.md).
Full backend/platform acceptance and all nine stages remain open.


### v0.532 — automatic-JIT Linux validation checkpoint

Recorded / зафиксировано: clean v0.531 minimal-jit release-runner validation,
1097 CTest cases, 26 fresh Unicorn cases and 1086 direct ELF artifacts;
precommit minimal/JIT 1096 each, 30 definition cases and three ASan/UBSan cases.
See [RU/EN evidence and limits](LINUX_JIT_CHECKPOINT_V531.md). Documentation
only; real foreign OS/CPU acceptance and all nine stages remain open.


### v0.533 — VM fault diagnostics and preservation policies

Implemented / реализовано: independent NONE/BACKTRACE/ZIP/ZIP_CODE/DEBUGGER
fault actions and NATURAL/FORCE detail. Copied source/body reports, synchronous
live argument/local/register inspection, raw heap/class snapshots, CRC ZIPs,
active structured JIT bytes and original-report preservation across replay.
Tests: crash_c_api, crash_diagnostics, crash_allocation, crash_archive_host and
execution_mode_frame_replay; targeted ASan/UBSan. See
[RU/EN contracts and limits](CRASH_DIAGNOSTICS.md). First-chance VM statuses
are supported; asynchronous OS signals/SEH and native-PC unwinding remain open.
SWF/SWD source import is the next user-requested debugger work. No roadmap stage
is closed / все этапы остаются открытыми.
### v0.549 — SWF ADPCM resource decoding

Read/submit now decode format 1 with 2–5-bit codes, independent mono/stereo
state, 4096-frame blocks and signed saturation. Full declared-stream validation
precedes publication; no allocation or decoder cache is added. Metadata remains
nominal. Other compressed codecs, eight-bit PCM, automatic timeline playback,
mixing and native-platform acceptance remain open. No stage is closed.

ADPCM format 1 реализован для явного чтения/отправки ресурса. Проверяется весь
заявленный поток до записи; этапы роудмапа остаются открытыми.
Validation v0.549: minimal suite 1142 passed plus one existing named-Unix-socket
skip (1143 total). Three affected tests passed ASAN/UBSAN (LSAN disabled;
external SDK/zlib uninstrumented). Eight independently packed FLV/SWF fixtures
matched system FFmpeg byte-for-byte across widths 2–5 and mono/stereo, including
saturation, adaptive indices and block transitions. Truncation leaves output
unchanged; zero-length requests still reject truncated data. Catalog 90 entries
and source/anchor links checked. This is not original Flash Player acceptance.

### v0.550 — complete raw eight-bit sound conversion

Implemented / реализовано: DefineSound PCM formats 0/3 now accept eight-bit
unsigned input and expand it to native S16 using (byte - 128) * 256. Metadata
retains its original width; range, memory and callback contracts are unchanged.
All 256 values, mono/stereo, SWF1–8 (format 3 from SWF4), slices, invalid capacity,
empty ranges and actual channel submission are covered by swf_sound_u8.cpp.
Four independent raw-U8 FFmpeg oracle comparisons are byte-exact. Three affected
tests pass ASAN/UBSAN (external SDK not instrumented; LSAN disabled). Original
Flash amplitude matching, other codecs and timeline sound playback remain open.
Minimal full suite: 1143 passed plus one existing Unix-endpoint skip (1144 total).
No roadmap stage is closed. Восьмибитный PCM реализован, стадии остаются открытыми.

### v0.551 — copied root StartSound/SOUNDINFO inspection

Implemented / реализовано: C89 event count/ordinal/envelope APIs; copied
one-based frame, flags, in/out positions, loop count and independent 44100 Hz
envelope points. All controls, reserved bits, gain bounds, exact record endings
and root frame structure validate before publication. Selected events resolve
unique DefineSound IDs; count deliberately performs syntax-only reference
inspection. No allocation, borrowed output pointers, playback or scheduling.
The line-commented exporter now prints controls before explicit PCM output.

Validation covers all 64 flags in SWF1–8, every truncated prefix, absent/duplicate
IDs, forward references, multiple frames, CWS, 255-point/long-tag envelopes,
wrong gain/ABI sizes and unchanged outputs. Allocation denial verifies all three
queries are allocation-free. Four focused tests passed ASAN/UBSAN (LSAN disabled,
external SDK not instrumented) and the JIT build profile. The actual C89 tutorial
prints independent event/envelope bytes, exports expected U8 PCM and rejects
truncated controls before creating output. Twelve ADPCM/U8 FFmpeg oracle cases
still match exactly. These are metadata APIs, not automatic timeline playback.
Minimal full suite: 1144 passed and one existing Unix-endpoint skip (1145 total).
StartSound2/buttons/streaming, applying loops/envelopes, mixing and final native
platform acceptance remain open. No roadmap stage is closed.

API StartSound/SOUNDINFO копирует метаданные без проигрывания: кадры, флаги,
границы, повторы, огибающие. Повреждение не меняет назначения; каждый запрос
проверяет текущий файл целиком, без кеша/выделения памяти. Полнота стадий не заявлена.

### v0.552 — explicit styled StartSound voice output

Implemented / реализовано: [fvm_swf_sound_event_submit](../include/flashvm/swf_sound.h#L97) renders PCM/ADPCM in/out
ranges, repetitions and linear mono/stereo envelopes before one guarded output
callback. Fixed-clock 44100 Hz positions convert without integer overflow;
explicit zero loops produce no voice. Source/output/envelope storage share a
checked temporary budget. Successful voices bind a dedicated channel resource;
failed preparation/output cannot commit binding. SyncStop/no-multiple currently
apply to that dedicated channel only. The C89 exporter has explicit --event mode.

Validation: minimal full suite 1147 passed plus one existing named Unix endpoint
skip (1148 total). Five focused checks passed in the native static ALSA null-PCM
profile, five in JIT profile and five under ASAN/UBSAN (external SDK uninstrumented,
LSAN disabled). SWF1–8 requested interpreter/JIT, gain/loop/range samples, 11025 Hz
control positions, zero loops, OOM boundaries, failed native write/stop, reentry,
GC barriers and callback reload are covered. The real tutorial exports expected
styled bytes and rejects bad styles before file creation. Twelve raw PCM/ADPCM
FFmpeg oracles remain byte-exact; source/tutorial/catalog links pass.

Явный вывод одного StartSound реализован через реальный host callback с полной
подготовкой до вывода; метаданные и данные не заимствуются через callback reload.
Автоматический MovieClip clock, несколько голосов/смешивание, streaming, остальные
кодеки, оригинальный Flash Player и foreign native acceptance ещё открыты.
No roadmap stage is closed. См. SWF_SOUND_PLAYBACK.md.

### v0.553 — synchronize every bound voice of a resource

Implemented / реализовано: StartSound no-multiple checks all active bound
same-ID voices in the context before allocation/output. A suppressed unbound
target remains reusable. SyncStop attempts every matching voice in registration
order, returns the first error after all attempts and retains only failed voices
as active. Other IDs, unbound host output and other contexts are preserved.
A single native barrier keeps registrations stable across callback reload and
rejects nested audio mutation/GC. Ordinary starts use separate inactive channels
for overlapping instances; source binding remains until close.

Validation: minimal full suite 1149 passed and one existing Unix-endpoint skip
(1150 total). Six focused checks passed in native static ALSA null output,
ASAN/UBSAN and JIT profiles (external SDK uninstrumented, LSAN disabled). Tests
cover overlap, suppression without binding, first-error/retry, independent IDs/
contexts, reload before a later matching stop, callback barriers and allocation-
free synchronization. The new fully annotated C89 swf_polyphony tutorial writes
three real four-byte PCM files and reports expected write/stop/active counts;
its no-argument/identical-path gates create no files. Both actual exporter
scripts and all twelve raw PCM/ADPCM FFmpeg oracle cases pass. Catalog now has
91 documented examples; source/anchor/line-comment checks pass.

Глобальная синхронизация относится к связанным StartSound voices текущего context;
смешивание PCM и автоматический MovieClip clock ещё не реализованы. Стриминг,
прочие кодеки, original-player equivalence и native foreign-platform acceptance
остаются открытыми. No roadmap stage is closed.

### v0.554 — prepared incremental sound voice snapshots

Implemented / реализовано: four C89 snapshot APIs capture one decoded source
interval/envelope, read bounded styled S16 chunks, inspect progress and explicitly
close native storage. Snapshots contain no VM/managed references and survive
GC/reload/context/runtime destruction. Read/info/close allocate nothing; play
counts do not expand source storage. Shared style arithmetic preserves full-render
output. The existing exporter adds --stream-event and explicit native cleanup.

Validation: recovered the exact v0.553 mirror after workspace cleanup and rebuilt
from source. Minimal full suite 1152 passed plus one existing named Unix endpoint
skip (1153 total). Six focused checks passed in JIT profile and six ASAN/UBSAN
(LSAN disabled). Five chunk partitions match full rendering for SWF1–8 U8/S16 and
all ADPCM widths, including block transitions. Allocation-denied reads drain
65535 plays after VM destruction with six source-PCM bytes. OOM publication,
fixed-clock cropped ranges, EOF/empty/zero plays, malformed controls/points, C89
calls, copied geometry and unchanged state on argument failure are covered.
Actual chunked tutorial output equals complete styled output across 257 repeats;
stop/zero-play/file failure gates behave as documented. All twelve FFmpeg raw
PCM/ADPCM oracles and source/tutorial/catalog links pass (91 examples).

Подготовленный голос читается без повторных аллокаций/разбора SWF; полный source
всё ещё должен поместиться в native budget. Snapshot не исполняет stop/no-multiple
и не является SoundStreamBlock decoder. Автоматический MovieClip clock, mixer,
прочие кодеки и foreign native acceptance остаются открытыми. No stage is closed.

### v0.555 — allocation-free native S16 block mixing

Implemented / реализовано: [fvm_audio_mix_s16](../include/flashvm/multimedia.h#L130) validates every input before output,
sums equal-rate/channel interleaved S16 exactly in a bounded signed-long
accumulator and saturates once. Short/exhausted inputs become silence; a caller
chooses output duration. No allocation, context, device, callbacks, resampling or
implicit clock is involved. Exact-start in-place and unaligned source byte views
are supported. The new swf_mix C89 tutorial reads two independent prepared voices
in small blocks and writes one actual mixed FILE output. Multimedia examples now
have an alphabetic mixing subgroup containing mix and polyphony tutorials.

Validation: full minimal suite 1156 passed plus one existing named Unix endpoint
skip (1157 total). Six focused checks passed in Release, JIT and ASAN/UBSAN profiles
(LSAN disabled). C89 cases cover final-only clipping, input-order cancellation,
stereo, empty/short tails, zero-input silence, byte/capacity overflow, unchanged
output on malformed later inputs, exact-start aliasing, unaligned bytes and all
1024 inputs. Allocation denial confirms no mixer allocation. The actual example
passes in all three profiles, creates the exact six-frame mixed file and handles
native output failure. An independent installed FFmpeg amix oracle is byte-exact;
all twelve raw PCM/ADPCM oracles and chunked exporter checks still pass. A
source-link sorting tie between uppercase macros and lowercase functions is fixed
with a deterministic secondary key; a six-hash-seed idempotence regression test
passes and demonstrably rejects the previous ordering. Catalog
has 92 entries; source/anchor and line-comment/API checks pass.

Mixing реализован как явный caller-driven block helper. Он не меняет snapshot
positions/bindings и не включает автоматический MovieClip scheduler. Разные
частоты/layouts требуют отдельной конверсии; прочие кодеки и foreign native
platform acceptance остаются открытыми. No roadmap stage is closed.


Prepared PCM stream snapshots / Независимые индексированные снимки PCM: [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm). Six C89 functions copy/index one validated root PCM segment; indexed reads survive source VM destruction without allocation. Preparation is explicitly budgeted; compressed codecs and automatic MovieClip audio clocks remain unsupported.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:130](../include/flashvm/multimedia.h#L130) |
| `fvm_call_bytes` | function | [include/flashvm/value.h:98](../include/flashvm/value.h#L98) |
| `fvm_construct_bytes` | function | [include/flashvm/value.h:155](../include/flashvm/value.h#L155) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `fvm_exec_clip_point` | function | [include/flashvm/execution.h:65](../include/flashvm/execution.h#L65) |
| `fvm_exec_clip_world_matrix` | function | [include/flashvm/execution.h:55](../include/flashvm/execution.h#L55) |
| `fvm_exec_delete_path` | function | [include/flashvm/execution.h:371](../include/flashvm/execution.h#L371) |
| `fvm_exec_detach_clip` | function | [include/flashvm/execution.h:237](../include/flashvm/execution.h#L237) |
| `fvm_exec_frame_request` | function | [include/flashvm/execution.h:336](../include/flashvm/execution.h#L336) |
| `fvm_function_construct` | function | [include/flashvm/function.h:69](../include/flashvm/function.h#L69) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../include/flashvm/function.h#L65) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `FVM_WatchWriteScope` | class | [src/core/watch_write_scope.hpp:8](../src/core/watch_write_scope.hpp#L8) |
<!-- source-links:end -->
