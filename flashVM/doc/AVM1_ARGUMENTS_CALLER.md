# arguments.caller / Вызывающая функция

v0.215 adds the nonenumerable, writable/deletable `caller` data property to
script arguments objects. It contains the active previous script function
identity, or null outside an identified script-function call. Call scope restores
that identity on success, script exceptions and allocation failures, including
failures while entering lexical scopes. Named legacy fallback records with no
first-class function identity use null. This is a script-frame identity model;
it does not claim complete native caller-chain or SWF scope conformance.

Объект arguments содержит caller с флагом DONT_ENUM: функцию предыдущего
скриптового кадра либо null при отсутствии её идентичности. Значение доступно
для записи/удаления. Выход из вызова, исключение и OOM восстанавливают прежний
кадр. Старые fallback-записи без first-class функции дают null. Полная цепочка
нативных вызовов и правила областей SWF остаются открытыми.

[GC:] The active identity is an explicit context collector root; suspended call
frames retain their previous identities. The arguments property keeps its caller
alive after return. Ordinary graph transport preserves this function edge with
shared callee/caller identities; no graph encoding or public ABI change is needed.

[GC:] Активная функция — явный корень контекста; приостановленные кадры сохраняют
свои функции. caller удерживает объект после возврата и переносится обычным
графом, без изменения формата/ABI.

`avm1_arguments_caller` checks SWF4–8 direct/nested calls, attributes, active
identity observed in a native callback (native GC barrier rejection retained),
escaped arguments after GC, channel transport to another context, pending script
exceptions, subsequent direct calls and an allocation-failure sweep. Depth,
identity, native depth, lexical scopes and root frames are checked after failures.
Reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/function.rs, load_arguments and arguments_caller. This source
comparison is not independent Flash Player runtime verification. Function2 and
roadmap stages remain open.

Тест проверяет перечисленные сценарии, включая восстановление корней и кадра
после отказов памяти. Барьер GC внутри нативного callback сохранён. Основание —
исходники эталона; независимого запуска Flash Player эти проверки не заменяют.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: full AGG 374 and
Cairo Release 373 tests passed (nine/four execution-permission failures retried).
The corrected allocation injector passed in all four profiles; caller,
Function2, functions, inheritance and super passed in NO_OS_PORTS and
heap/global ASan + UBSan. Stack/leak sanitizer coverage remains disabled.
The original SWF6 define_function2_preload_order fixture revealed an independent
parent-preload slot/base-clip mismatch, still open at this commit; no byte-exact
pass is claimed for that fixture. The upstream arguments fixture has header 15
and was not executed as a SWF<=8 oracle.
