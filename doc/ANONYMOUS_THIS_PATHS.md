# Qualified Undefined this paths / Составные пути при Undefined this

v0.513: loaded SWF5–8 path resolution uses the Undefined script this cell even
when an embedding receiver object is supplied. GetVariable, SetVariable and
qualified Delete2 therefore cannot read, write or delete the embedding object's
properties through this.marker or this:child:marker. Delete2's direct this-prefix
shortcut also observes the cell. Named calls continue to use their object-valued
this. The unloaded host policy is preserved; SWF4 path grammar is unchanged.

В v0.513 обработчик путей SWF5–8 учитывает Undefined в ячейке this вместо
объекта моста. GetVariable, SetVariable и составной Delete2 не читают, не пишут
и не удаляют свойства этого объекта через this.marker или this:child:marker.
Отдельная короткая ветка Delete2 также учитывает ячейку. Именные вызовы сохраняют
свой объект this. Поведение host без загруженного SWF и грамматика SWF4 не изменены.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/activation.rs resolve_target_path, get_variable, set_variable and
action_delete_2. The path resolver obtains this_cell and stops when its value
cannot become an object; Delete2 reads the prefix through get_variable.
That source itself notes incomplete this keyword support. This is a narrow
source comparison, not an original Flash Player oracle or complete path acceptance.
Исходный обработчик также отмечает неполную поддержку ключевого слова this;
сравнение не заменяет проверку оригинальным Flash Player.

anonymous_this_paths covers read/write/delete, dot/colon and nested prefixes,
loaded SWF5–8 plus unloaded host controls, named/anonymous calls, direct helper,
interpreter, explicit and automatic JIT, and pure-function graph transport.
anonymous_this_paths_roots checks skipped accessors for Undefined and named
accessor capture/target roots after removing the descriptor, function binding,
child binding and prototype before collection. It covers success, throw and
error. anonymous_this_paths_allocation sweeps allocation failures and retry
with traced counted arguments and a self-linked global child; metadata and
unchanged marker values are checked after each attempt.

Remaining work: arbitrary this-cell assignment, primitive/null receiver coercion,
full Unicode paths and complete SWF/foreign-OS acceptance. All roadmap stages
remain open. Открыты присваивание произвольного this, преобразования примитивного
и null получателя, полные Unicode-пути и общая совместимость SWF/других ОС.

Validation / Проверка: Linux x64 C++98 minimal 1054, AGG 1073 and Cairo 1074
cases passed after execute-permission repairs and failed-case reruns on 3/5/5
binaries. Four targeted ASan/UBSan cases passed. The strengthened fixture binds
both the caller and called path function to explicit native JIT bodies; its
three targets passed in all profiles, with one additional permission repair in
AGG and one in sanitizer. Automatic native compilation is covered separately.
Fresh Cairo GNU readelf evidence passed for 1063 CTest ELF artifacts, with direct
dependencies limited to libc, libgcc_s, libm and libstdc++; bundled dependency
families were not dynamically linked. This audit is direct DT_NEEDED evidence,
not transitive/dlopen coverage or foreign-platform acceptance.
