# Resolved calls / Вызовы найденных значений

v0.246 moves named call/constructor dispatch into `fvm_exec_resolved_call` in a
separate handler file. Its C89 interface receives a counted name, resolved value,
found/missing flag, explicit receiver and ordered argument values. Scope lookup,
name/count operand decoding and stack movement remain in the interpreter adapter.
Generated code can resolve bindings by its own prepared-scope rules and use the
same dispatch service; no bytecode or local-variable container crosses the ABI.

A found undefined/noncallable value remains distinct from an absent name. Current
script/native/constructor fallback rules and status codes are retained, including
legacy named-script registry fallback. Loaded name/count conversion happens in
separate value services. Full missing-binding/noncallable Flash compatibility is
still open; this decomposition makes no claim to resolve it.

[GC:] Dedicated uppercase root macros retain counted name, resolved function,
receiver and arguments across invocation/construction. Outputs may alias inputs
and remain unchanged on failure. The constructor-service bound preserves the
unloaded NewObject bound separately from the loaded method-count bound.

C89 checks cover explicit receiver, embedded-NUL native fallback, aliasing,
found-undefined shadowing, native script collection/exception and constructor
fallback. Allocation injection covers constructor failures, unchanged output and
root-chain restoration. Existing original SWF traces remain unmodified.

Диспетчеризация именованного вызова/конструктора вынесена в общий C-слой.
Он принимает уже найденное значение и признак наличия привязки; поиск области
видимости остаётся у вызывающей стороны. Корни удерживают функцию, имя,
получателя и аргументы. Проверяются C89-вызовы, GC, исключения, встроенный NUL,
сохранение результата при ошибке и отказы памяти. JIT и все стадии остаются
открытыми; общий C-интерфейс только подготавливает повторное использование.

Evidence: 454 tests pass in each Linux AGG/Cairo profile. Seven focused heap/global
ASan+UBSan checks pass; stack/leak instrumentation is unavailable. All 23 unchanged
original SWF traces remain byte-exact in both profiles. All roadmap stages stay open.
