# Private Function2 count / Размер частного набора Function2

v0.239 treats loaded Function2 RegisterCount as the complete number of private
slots, including slot zero. For a positive count N, indices 0..N-1 exist; index N
is missing and does not fall back to the context-global bank in the supported
old-Player policy. Count zero uses global registers for ordinary instructions,
while private preload/parameter assignments remain discarded.

В v0.239 поле RegisterCount задаёт полное число частных ячеек, включая нулевую.
При N>0 допустимы индексы 0..N-1; чтение N возвращает undefined и не использует
общий регистр с таким индексом. Нулевой частный набор использует общие регистры
для инструкций, но не превращает private preloads в записи общего набора.

Prepared native bodies receive that exact private span, or the live four-value
global span when appropriate. Unloaded compatibility frames retain their old
highest-index contract, with the extra slot described by
FVM_AVM1_HOST_REGISTER_EXTRA_SLOT. No public struct or wire-layout change is made.

The new fixture crosses SWF4–8, counts 0/1/2/255, the last valid and first invalid
parameter destination, global fallback suppression and native frame sizes.
Three generated semantic fixtures previously declared one slot while expecting
register 1, or seven slots while expecting register 7. Their headers now reserve
valid spans for the intended preload/root scenarios; original SWFs and external
oracle output files are unchanged.

Primary reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
`core/src/avm1/function.rs` allocates `0..register_count()`, leaves register zero
free from preloads, and assigns arguments through local-register access.
`core/src/avm1/activation.rs` does not use global fallback when a private bank
exists in old Player versions. Broader Player-version policy, complete AVM1
compatibility and all roadmap stages remain open.

Validation: Linux AGG 443/443 and Cairo Release 443/443, plus six focused
heap/global ASan+UBSan checks with stack/leak instrumentation disabled. The 23
unchanged original SWFs retain exact output, zero exit and empty stderr in each
profile. Non-Linux acceptance remains open.
