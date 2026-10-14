# Function2 inherited this / Унаследованное this в Function2

v0.221 separates the active implicit this identity from the actual call receiver.
In loaded SWF5–8, Function2 with either preload-this or suppress-this (flags 1,
2 or 3) inherits its caller's implicit this. Without those flags it uses its own
receiver. A preload register still contains the actual receiver for flag 1 and
undefined for paired flag 3. Direct host entry has no suspended script frame;
its inherited identity defaults to the context global object.

В загруженном SWF5–8 Function2 с preload-this или suppress-this наследует
неявное this вызывающего кадра. Без этих флагов используется собственный
получатель. При flag 1 preload-регистр содержит собственного получателя;
при паре flag 3 он содержит undefined. При прямом входе из host без вызывающего
скриптового кадра наследуемый получатель — глобальный объект контекста.

The active identity is scoped to execution. Timeline/host bytecode entry selects
its explicit receiver; try/catch/finally blocks inherit the active identity.
Each execution root frame retains both actual and implicit receivers; restoration
runs on return, exceptions and interpreter allocation failure. The context also
marks its active identity. Private context state is not serialized; public ABI
and function wire format are unchanged.

Неявный получатель устанавливается на время исполнения. Внешний вход байткода
выбирает явного получателя; вложенные блоки try/catch/finally сохраняют текущего.
Корни исполнения удерживают обоих получателей; выход и ошибки восстанавливают
вызывающий кадр. Приватное состояние контекста не передаётся через канал;
публичный ABI и формат функций прежние.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
`core/src/avm1/function.rs` local_this selection and load_this.
`avm1_this_inherited` calls a Function2 method on a different object from the
outer function receiver, across SWF5–8 and flags 0–3. It distinguishes named this
from register this, collects pinned graph roots before calls and checks restored
execution state after return and explicit throw. A native callback reenters
bytecode on a different receiver and checks restoration of the caller identity;
GC remains blocked by the normal native-call barrier. The existing arguments
caller allocation-failure sweep also asserts that implicit this is restored.
This source-based test does not
certify all Player behavior. Mutable/non-object this cells, closure/timeline
provenance and complete native-call activation semantics remain open.

Тест различает вызывающий объект и получателя метода, имя и регистр this,
проверяет GC перед вызовами и восстановление после throw и нативного повторного
входа. Проверка отказов выделения памяти также проверяет сброс this. Изменяемые ячейки,
this других типов, полная привязка к timeline и нативным вызовам остаются
открытыми. Завершение стадии не заявляется.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 379 tests passed
(eight execution-permission retries), Cairo Release 378 passed without retries.
Eleven selected AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan;
stack/leak checks remain disabled. All 17 unchanged selected original SWF<=8
probes retained byte-exact traces, zero exits and empty stderr. Separate original
this/AS2 probes still fail and remain excluded from the passing reference corpus.
