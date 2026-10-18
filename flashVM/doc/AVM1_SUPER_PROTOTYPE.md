# Super prototype projection / Проекция прототипа super

v0.225: when loaded super lookup reaches the builtin __proto__ getter, it
returns the selected ancestor's prototype, rather than the current receiver's
prototype. A root ancestor without a prototype produces undefined. Custom
own data/methods/accessors keep their ordinary dispatch and actual receiver.
Unloaded host behavior is unchanged. This covers the builtin projection only;
the complete ordinary raw __proto__ property model, primitive/null distinctions,
name casing and all custom inherited shadowing remain open.

При поиске super.__proto__ встроенный getter теперь проецирует прототип
выбранного предка, а не текущего this. Для корневого предка возвращается
undefined. Пользовательские данные/методы/getters сохраняют обычный dispatch
с исходным получателем. Host-поведение прежнее. Полная модель обычного raw
__proto__, primitive/null, регистр имён и затенение остаются открытыми.

Primary sources: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/object/super_object.rs proto/base_proto and
core/src/avm1/object.rs get_opt forwarding to the selected ancestor chain;
the unchanged original SWF8 as2_super_and_this_v8 checks the projection through
constructors, inherited methods, getters and setters.
avm1_super_prototype checks three inheritance levels across SWF5–8 and host
calls, actual-this custom getters, an escaped facade after GC, transfer to a
loaded context without standard installation, and live parent replacement.
No ABI or wire-schema change.

Тест проверяет три уровня наследования, пользовательский getter с actual this,
GC, перенос и изменение живой родительской цепочки. ABI и схема прежние.

The original AS2 v8 probe now passes completely: 1841 byte-exact trace bytes,
zero exit and empty stderr, without changing its SWF or oracle. It joins the
passing original corpus, now 18 cases. AS2 v6 still lacks parent constructor
calls; this_swf5/6/7 and function_suppress_and_preload also remain failing.
No roadmap stage or non-Linux runtime certification is claimed.

Исходный AS2 v8 проходит целиком: 1841 байт совпадает с эталоном, код выхода
нулевой, stderr пустой; SWF и эталон не менялись. В проходящем корпусе теперь
18 случаев. AS2 v6, this_swf5/6/7 и function_suppress_and_preload ещё не проходят.
Завершение стадии не заявляется.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 384 and Cairo
Release 383 tests passed after execution-permission retries (6 and 3).
Twelve selected AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan;
stack/leak checks remain disabled. All 18 unchanged selected original SWF<=8
probes produced byte-exact traces, zero exits and empty stderr.
