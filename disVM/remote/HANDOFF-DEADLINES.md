# One deadline through local handoffs / Один дедлайн на локальные передачи

An absolute deadline was already stored in owner and broker requests. Several
intermediate paths nevertheless converted it back to a relative timeout before
calling the next adapter. If that handoff paused, the next adapter created a fresh
deadline from its later clock reading. A known-expired request could execute an
effect or create a retained source owner after its original admission budget.

Now the existing local deadline stays attached through request allocation and
offer to the actual execution owner. The private matched libraries add:

| Adapter | Absolute entry | Used by |
| --- | --- | --- |
| Proxynet | calluntil | Ref Read/Write/Add/Close forwarding |
| Proxynet | cloneuntil | Source actor startup Retain |
| Proxyowner | calluntil | Server scalar/lifecycle and registry Retain/Drop |
| Proxyowner | fetchuntil | Server variable/function result callbacks |

The network offer accepts either a newly established relative budget or the
provided absolute deadline. Until entries preserve the deadline through capability
validation, allocations, supervisor, broker queue and owner admission. Existing
pre-offer, queued and actual-owner expiry checks remain in place. The route actor
also applies its configured source timeout as one minimum absolute limit; it does
not recreate that limit after the handoff.

Это локальные absolute deadlines: значения не сериализуются между разными VM
и не предполагают синхронизацию часов разных машин. Public relative-timeout APIs
сохраняют прежний контракт нового запроса. Внутренние передачи уже начатого
запроса используют until API и не получают новый бюджет. Private DVP3 остаётся
184 bytes; public Dis64/Sys IDs и vendor snapshot не меняются.

## Effect and ownership boundary / Граница эффекта и владения

Known expiry before offer returns NotSent without a new wire effect, native
owner execution or startup Retain. An outer Ref caller that already offered its
access can still receive Unknown while the actor finishes refusing the late
inner offer. Unknown never authorizes replay. Work admitted before expiry can
still have partial effects; no transaction rollback or forced cancellation is
introduced.

Source templates and in-flight requests retain their existing roots. An expired
startup closes its inactive local wrapper queue without retaining the remote
owner. A refused late Write leaves the live source Ref usable for Read/Close.
Ordinary and forwarded results keep their existing native Retain/Drop ownership.
Original caller canwait policy, terminal Completing/Closed phases, permanent
native thread identities and TLS REG are unchanged.

Known expiry до offer означает NotSent без нового wire/native effect либо
startup Retain. Уже предложенный внешний Ref access может завершиться Unknown;
это не повод повторять эффект. Просроченный Write не отзывает живой source Ref,
а отказ startup освобождает локальные inactive metadata. Принятые вовремя
callbacks по-прежнему могут иметь partial effects; rollback не добавлен.

## Deterministic acceptance / Проверка настоящих эффектов

`test_proxy_handoffs.py` widens only private test handoff windows. A one-second
original budget is captured before a 1.5-second sleep; production has no such
sleep. The tests use actual native VM owners, not a simulated result:

- Ref Write pauses after budget capture. The caller reports offered Unknown;
  a completion gate then allows a Read which must see original A=10. The wire
  observer must see no Write and normal Close/Drop/Stop must still complete.
- Startup pauses before clone. It must return NotSent/nil with no wire Retain;
  original greeting owners remain usable and are released normally.
- The server pauses before ordinary owner Write and before each typed result
  callback. The original frame deadline expires; A stays 10 and no late reply
  is emitted. The original owner and all exports join on the terminal path.

Separate private omissions restore only the old relative handoff. They must
prove an actual late Write A=999, a late wire Retain and published Ref, or actual
result callback mutation A=11. The expired owner frame still emits no reply:
checking only wire silence would miss those old effects. Source omissions raise
the exact semantic diagnostic only after exercise roots/actors retire. Every
case requires matching native resident lifetimes and drained RC/proxy GC/export
ownership with strict concurrent-GC counters. Crashes, compiler errors, timeouts
and sanitizer reports cannot satisfy these controls.

C/JIT × fast/locked GC polling проверяют каждый путь. Omissions возвращают только
старую relative-timeout передачу и обязаны показать точные поздние effects либо
Retain. Проверка owner смотрит original Cell, а не только отсутствие reply.
Все тесты используют dedicated native OS threads без global stop/exclusive.

## Verified series / Проверенная серия

2026-11-14: complete ordinary CTest passes 92/92. Final selected TSan passes 8/8:
network, forwarding, routed results, original queue deadlines, retirement, new
handoffs and selected semantic controls. All new positive roles pass C/JIT with
fast/locked polling. Renewed-budget controls prove late Write=999, Retain and
both result callback effects=11 with native/root/ownership cleanup.

Полный ordinary CTest: 92/92; затронутые TSan группы: 8/8. Новый workload
`proxy-handoffs` расширяет native architecture matrix до 55. Полный 93-test TSan
результат не заявлен; новая CI проверяется отдельно от локальных результатов.
