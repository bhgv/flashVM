# Повторная загрузка SWF / Reloading SWF

RU: v0.482 выполняет переход встроенных объектов из host-layout в SWF-layout
только при первой успешной загрузке. Последующие загрузки SWF 4–8 в тот же
контекст сохраняют изменённые флаги и идентичность существующих встроенных
объектов. Ошибка выделения памяти при замене фильма сохраняет предыдущие
байты и версию SWF; повторная попытка также сохраняет пользовательские флаги.
Это не утверждение о полной эквивалентности жизненного цикла Flash Player.

EN: v0.482 activates the loaded builtin layout only on the first successful
load. Replacing SWF bytes in an already loaded context preserves live builtin
attributes instead of reinterpreting user edits as untouched host defaults.
The transition remains transactional for first-load activation. A failed
replacement retains the old SWF bytes/version and permits retry after GC.
Full Flash Player movie lifecycle compatibility remains open.

`builtin_reload_attributes` covers every SWF4–8 source/destination pair,
ten fields on Object/Array/String/Number/Boolean/Date/Function/Error/Math,
C calls, interpreter, explicit JIT and automatic JIT, GC and repeated reloads.
`builtin_reload_allocation` sweeps replacement-load allocation failures and
checks old movie state, attribute preservation, retry and object identity.
Both regressions fail with the v0.481 activation implementation.
