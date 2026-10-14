# Missing register destinations / Отсутствующие регистры

v0.237 makes loaded AVM1 Push of an unavailable register return undefined.
StoreRegister and catch-register assignment discard writes to unavailable slots.
Function2 parameter/preload destinations outside the prepared bank are skipped
without reading or writing outside the value array. A dropped register parameter
is not converted into a named local binding.

В v0.237 загруженный AVM1 возвращает undefined при чтении отсутствующего
регистра и игнорирует запись в него. Это относится к StoreRegister, catch,
параметрам и preloads Function2. Индекс не приводит к выходу за границы памяти;
параметр, назначенный отсутствующему регистру, не создаёт локальную запись имени.

The loaded decoder accepts structurally complete Function2 records whose
preloads or parameter destinations exceed the declared bank. Reserved flag bits,
unterminated names and body boundaries remain validated. Standalone host
inspection and unloaded host invocation retain strict destination checks.
Transferred functions apply the receiving context's policy. Named preload
indices/count and the hexadecimal body bound are documented header macros.

The fixture crosses SWF4–8 and unloaded host calls, high register IDs, discarded
stores, oversized parameter/preload destinations, collection and graph transfer.
Primary behavioral references: pinned Ruffle
2372a4e148a67d68efd796f4477b3cddd4f880cd, `core/src/avm1/activation.rs`
current_register/set_current_register/set_local_register and `core/src/avm1/function.rs`.
Global-register persistence and the exact private-bank count contract remain
follow-up work; this change does not certify complete register compatibility or
close any roadmap stage.

Validation: Linux AGG 439/439; Cairo Release 439/439 after retrying five
execute-bit launch failures; five focused heap/global ASan+UBSan checks pass.
The extended catch-register fixture also passes in each profile. All 23 original
unchanged SWFs retain byte-exact traces, zero exits and empty stderr in both
graphics profiles. Stack/leak instrumentation and non-Linux acceptance remain open.
