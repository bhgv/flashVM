# AVM1 Number text and strict NaN / Числовой текст и строгий NaN

v0.229 replaces host shortest-roundtrip spelling with the loaded AVM1 decimal
algorithm: fifteen significant digits, fixed notation through exponent 14 and
down to exponent -5, explicit signed exponents outside that interval, and the
player's digit extraction/carry behavior. The host formatter is unchanged.
The managed Number-to-string C bridge shares this with interpreter conversions;
no bytecode-only formatting path exists.

Загруженный AVM1 выводит 15 значащих цифр и использует границы обычной/экспоненциальной
записи Flash. Исправлены дробные результаты деления, например
`3 / 12.34` → `0.243111831442464`. Воспроизведены reference-quirks при переносе
цифр, включая отрицательную строку `-e+16` в указанном случае округления.
Незагруженный host продолжает использовать прежнее roundtrip-представление.

The implementation adapts Ruffle `core/src/avm1/value.rs` f64_to_string and
decimal_shift at commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`; MIT attribution
and license are recorded in the source and external/flash_case/LICENSE.md.
Base-2 exponent extraction uses portable frexp rather than integer aliasing.
[GC:] A bounded local text buffer is copied into context string storage before
publication; allocation/adoption failure preserves collector root chains.

Loaded StrictEquals now treats two Number NaNs as equal, without coercing either
operand. Different types remain unequal, and host strict equality retains IEEE
NaN inequality. Primary source: the same pinned activation.rs action_strict_equals
uses Value equality; value.rs PartialEq explicitly recognizes paired NaNs.
This corrects the earlier StrictEquals assumption in the numeric-action tests.

StrictEquals для пары числовых NaN возвращает true в загруженном AVM1;
`NaN === "NaN"` остаётся false, а host-политика NaN не меняется. Никакие
valueOf/toString callbacks в этой операции не вызываются.

Evidence: all 432 Linux default AGG tests/examples passed. avm1_number_text checks
37 primary-reference scalar cases across SWF4–8: signs, notation boundaries,
rounding, subnormals, extreme finite values, infinity/NaN and historical carry
quirks. It retains text across traced-string GC and checks direct C strict
comparisons/host behavior. avm1_number_text_allocation injects allocation failure
through the context string publication path. Five selected tests pass restricted
heap/global ASan + UBSan with the previously documented stack/leak exclusions.

The unchanged original divide_swf4 and strictequals_swf6 SWFs now both produce
byte-exact expected traces, zero exits and empty stderr. The retrieved SWF<=8
set grows from 15/22 passing to 17/22, using the same explicit root-timeline and
quit-acknowledgement policy. Remaining failures: object_prototypes and four
Function2/this probes listed in EXECUTION_CHECKPOINT.md. No input/oracle edits.
No complete Flash Player, roadmap-stage or non-Linux runtime certification.

Исходные divide_swf4 и strictequals_swf6 проходят целиком; в восстановленной
выборке теперь 17/22 совпадений. Остальные пять проб не скрыты и остаются
работой. Windows/macOS/Android и JIT backend ещё не подтверждены.
