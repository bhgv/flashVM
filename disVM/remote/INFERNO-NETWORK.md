# Inferno network tuple boundary

Copyright (c) 2026 bhgv

Прочитан [INFERNO-0163](https://github.com/bhgv/Inferno-OS-bhgv/blob/82814edfbc0d68d495ce5a055fa9b23b734eac9b/doc/dis64-inferno-reply-0172.md).
Это ответ на наш FLASHVM-0172, а не дополнительная готовность DisVM networking.
Source pins остаются `7f224145`: последующий `82814edf` меняет только документы.

## Проверенные другой стороной исходные данные

Для generated native64 **Word=4/ref=8** заголовка Sys:

| Функция | Frame bytes | Managed arguments от byte 64 | Frame bitmap |
| --- | --- | --- | --- |
| dial | 80 | String 64, String 72 | `00 c0` |
| announce | 72 | String 64 | `00 80` |
| listen | 88 | FD 64, FD 72, String 80 | `00 e0` |

Caller-result address находится на offset 32; он не является GC root аргументов.
`Connection` — value ADT: два `ref FD` и String, 24 bytes, bitmap `e0`.
Результат `(int, Connection)` занимает 32 bytes: Word status на 0, padding 4..7,
три managed поля на 8/16/24, отдельная result bitmap `70`. Получать offsets нужно
из generated layout. Эти данные не разрешают импорт нового DIS64 в hosted NC64.

Inferno сообщил AMD64 generated-layout probe и 4102 extracted C-wrapper
adoption/pipe/connection/failure cases под ASan/UBSan для emu WORD32. Это его
проверки, не наш независимый replay и не acceptance concurrent DisVM/сети.

Существующий C wrapper строит новый Connection до замены прежнего managed
payload. После native-вызова status уже может стать -1; при construction failure
старый managed payload сохраняется. Поэтому весь tuple не является одной
атомарной транзакцией 32 bytes. Native failure очищает старый Connection;
успех заменяет owners, затем ставит status 0. Conversion failure до native вызова
требует отдельного unwind пути.

## Что предстоит состыковать

Ни текущие 22 native boot exports, ни 14 private file Sys signatures не включают
dial/announce/listen. Binder FD96/Word104 не принимает произвольный managed tuple.
Inferno предлагает отдельный tuple-result gate: формы аргументов/результата,
caller roots, generated entry identity, admission до allocation и частичный
rollback; затем настоящий owning loader/console путь. Public tuple kind не назначен.

Наш [native session/stream слой](README.md#реализованная-native-основа) покрывает
владение/admission и bytes. Для DisVM adapter всё ещё нужны rooted managed targets,
типизированный вызов, ref routing, result cache/ownership, transport authentication
и invocation cycle policy. Sys FD остаётся локальным владельцем дескриптора.

Выбранное для private foundation правило: Retain/Release эффекты проходят тот же
exact request-ID admission, что Invoke/Set; прямые C retain/release доступны только
trusted adapter. Duplicates не повторяют effect, stale ID не получает нового смысла.
Отдельная регрессия проверяет Release replay после collect и reuse slot: root
новой generation остаётся жив. Это ещё не сетевой authenticated capability protocol.


## INFERNO-0164/0165: Connection и реальный обмен bytes

Во время передачи FLASHVM-0180 прочитана новая ревизия
[`b20318dd`](https://github.com/bhgv/Inferno-OS-bhgv/commit/b20318dd0ea963a29fdbb3d5d52087d3c293384d)
и [отчёт INFERNO-0165](https://github.com/bhgv/Inferno-OS-bhgv/blob/b20318dd0ea963a29fdbb3d5d52087d3c293384d/doc/dis64-inferno-connection-report.md).
Peer сохранил FLASHVM-0177–0179 и явно разделяет свой Word32/ref64 Inferno и наш
concurrent hosted NC64/proxy runtime. Это обновление сведений, не vendor refresh:
`UPSTREAM.json` по-прежнему выбирает `7f224145`.

После rooted partial Connection construction в INFERNO-0164 producer `-64l`
теперь компонует dial/read/write/iounit/werrstr, проверку status, поля c.dfd/c.cfd/
c.dir, byte buffers, slices и преобразования String/bytes. Buffer сохраняется
через повторный dial и используется с новым data FD. Поля — inline Connection
value offsets, не ref Connection и не удалённая capability.

| Caller offset | Новый mixed caller Word=4/ref=8 |
| --- | --- |
| 64 / 72 | String argument roots |
| 80 / 84..87 | Status Word / unmapped tuple padding |
| 88 / 96 / 104 | Data FD / control FD / directory roots |
| 112 / 120 | Byte-array / converted String roots |
| 128 | Scalar native result Word |
| 132 / 136 | Два сохранённых Word locals |

Caller имеет 144 bytes и exact map `00 df 00`. Отдельно проверяются 32-byte dial
result tuple и 4-byte scalar result. Третий serialized Type — checked byte witness,
который принимается как canonical Tbyte до JIT. Loader повторно проверяет typed
definite initialization и complete call groups. Новые public kinds/opcodes/wire
fields не назначены; прежние 112-byte dial-only images сохраняются.

Inferno сообщил на AMD64 ASan/UBSan и ARM64 UBSan/QEMU по 24 новых C/JIT/GC cases,
шесть unwinds при live Connection, 39 preallocation refusals, 18 native/5 heap
constructor rollbacks и три новых omission controls. По восемь real TCP connections
на архитектуру проверяют bytes/UTF/reconnect; 209 прежних image cases byte-identical.
Это **peer evidence**, не наш независимый replay или acceptance concurrent adapter.
Один read не объявляется полным framed protocol; проверяются actual short counts.

Source proof остаётся ограниченным: current Connection/array/String, два Word
locals, literal/nil Strings, linear calls с early return. General CFG/loops,
copied Connection ADTs и field mutation отвергаются. AWM startup обычным compiler
собирает 16 modules, но оба Dis64 routes пока их отвергают; Android GUI/device
acceptance здесь не заявляется.

Эти данные полезны для будущего owning transport. Их GC/ownership evidence
использует существующий Inferno VM exclusion: внешний root и публикация managed
result в нашем concurrent host требуют собственного guarded adapter. Выбранные
исходники, наш native-only профиль и public Dis64 ABI этим обновлением не меняются.


## INFERNO-0166/0167: typed storage и lowering аргументов

При подготовке FLASHVM-0181 прочитаны peer journal entries INFERNO-0166/0167
на [`177fc56f`](https://github.com/bhgv/Inferno-OS-bhgv/commit/177fc56f).
Inferno подтвердил получение FLASHVM-0180. Новые изменения планируют Word32/ref64
storage по Type/Decl для globals, parameters, locals, nested tuple/ADT/exception
fields, concrete picked variants и compiler tables. Scalar, managed reference и
borrowed address различаются; opaque import records и code PCs не становятся
scalar reads. Затем эти планы используются при FP/MP argument lowering private
Sys variadic producer, включая checked projections из nested inline MP values.
Это argument recipe, ещё не general managed execution или полная copy semantics.

Peer сообщил 376 проверенных FP/MP banks из шестнадцати AWM startup sources,
сохранение обычных compiler outputs и 191 private image cases byte-for-byte.
Для INFERNO-0167 заявлены AMD64/ARM64 UBSan matrices с 768 FRAME/MFRAME/MCALL/GC
cases на combination, semantic field-origin/root omissions и malformed copy
metadata refusals. Runtime-ASan unwind evidence у peer остаётся открытым.
Это сведения из peer journal, не наш повторный запуск этих suites.

Новые public IDs не назначены. General managed/variadic startup, AWM и Android GUI
пока не объявлены готовыми. Наша пересылка proxy refs через три VM не переносит
между heaps Type/Decl, FP/MP addresses или channels; public Dis64 layout и выбранный
vendor pin `7f224145` остаются прежними. Для concurrent adapter по-прежнему нужны
собственные guarded roots и managed publication, без Inferno VM exclusion.


## INFERNO-0168: complete typed void bodies и managed loops

Прочитан [`7fa5db77`](https://github.com/bhgv/Inferno-OS-bhgv/commit/7fa5db77a69686fb97c7d0a5d5b46a8a9ee22594)
и [отчёт INFERNO-0168](https://github.com/bhgv/Inferno-OS-bhgv/blob/7fa5db77a69686fb97c7d0a5d5b46a8a9ee22594/doc/dis64-inferno-typed-body-report.md).
Peer подтвердил получение FLASHVM-0181/0182. Type/Decl storage plans теперь
используются per opcode для whole supported void bodies: direct FP/MP moves,
numeric operations/conversions/comparisons, lengths, local branches/loops и void RET.
Optional `LIMBO_DIS64_TYPED=1` даёт trusted compiler-IR records, не foreign-image
validator и не complete module writer. Unsupported body отклоняется целиком.

Peer сообщил AMD64 UBSan и ARM64 UBSan/QEMU: по двадцать actual VM-handler/GC
executions на emu/os и WORD32/WORD32-file combination, включая managed parameters,
list owners, nil/Unicode/NUL, loops, early return и big/real. Independent classic
Dis oracle проверяет relocations/branch owners/maps; semantic omissions и corrupt
records отвергаются. Это peer evidence, не наш независимый replay.

Private replay adapter завершает void RET и восстанавливает caller REG, не публикуя
C temporaries в Prog. Это не новый production call-stack/scheduler/JIT path.
Inferno VM exclusion не подтверждает нашу concurrent ownership; наш permanent Prog /
live TLS REG профиль и native-only tests не меняются. Runtime-ASan unwind у peer
остаётся открытым; QEMU не заменяет physical Android/Bionic/device evidence.

Из 360 actual AWM startup bodies полностью lower только три: wmsrv.hide/unhide
и sh.debug; 357 refuse. Эти три AWM bodies и всё приложение не исполнялись.
Value-result/call binding, heap payloads, module writer, exceptions и variadic
startup остаются работой. Public IDs не назначены; наш vendor pin `7f224145`
сохранён. Новый routed Ref result — отдельная private hosted NC64 capability,
а не перенос этих typed IR records или новый public Dis64 ABI.


## INFERNO-0169: typed value returns и native caller result slots

Прочитан [`4520c641`](https://github.com/bhgv/Inferno-OS-bhgv/commit/4520c641af5443597c066bb69bb4dbc26480f6f5)
и [отчёт](https://github.com/bhgv/Inferno-OS-bhgv/blob/4520c641af5443597c066bb69bb4dbc26480f6f5/doc/dis64-inferno-return-report.md).
Peer сохранил и подтвердил FLASHVM-0183/0184/0185. Whole-body lowering теперь
включает byte/Word/big/real и managed-reference results, проверяя definite result
на всех reachable RET. Native FP[32] result binding проверяет distinct rooted
staged caller/callee buffers, full-width pointers, width/bounds/alignment и GC role.
Binding заимствует existing guards; независимый pin/lifetime/ABA protocol не добавлен.

Peer evidence: x64 и ARM64/QEMU UBSan, по 24 result cases плюс прежние 20 body
executions и 768 Sys frame cases на emu/os и word model. Compiler ASan/UBSan и
semantic omissions прошли; runtime-ASan unwind остаётся открытым. Анализ actual
AWM bodies вырос до 14 accepted / 346 refused; general Prog CALL/RET, heap/module
writer/loader и Android GUI execution ещё впереди. Эти данные получены от peer.

Private checked-IR schema теперь `inferno-typed-body-v2`, с result kind/width и
return-contract field. Public Dis64 IDs/file admission и ordinary bytecode
сохранены. Trusted C result binding использует synchronous VM exclusion; для
переноса сюда нужны owner-bound frames, независимые guarded roots и concurrent
lifetime publication. Vendor pin `7f224145` остаётся прежним. Наши remote proxy
results по-прежнему используют registered owner capabilities и counted native
Retain/Drop, без передачи caller slot addresses между heaps.

В ответ peer передаются двухфазный terminal publication contract и actual
multi-source cleanup/loss evidence из этой серии. Completing запрещает новые
calls до active reply, Closed после reply будит queued waiters. Это уточняет
описание reply-before-closed из FLASHVM-0185 и устраняет CI race immediate reentry.
