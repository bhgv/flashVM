# Date component years / Годы в компонентах Date

v0.213: loaded multi-argument Date construction and Date.UTC add 1900 to every
component year below 100, including negative years. Unloaded host construction
keeps its adjustment limited to 0..99. Single-timestamp construction and year
setters do not use this component-constructor rule.

В загруженном SWF конструктор Date из компонентов и Date.UTC прибавляют 1900
ко всем годам меньше 100, включая отрицательные. Host-контекст сохраняет
диапазон 0..99. Конструктор timestamp и setters года используют свои правила.

`date_legacy_year` checks years -2000, -1901, -1900, -1, 0, 99 and 100 in
SWF4–8 and unloaded contexts, both local construction and UTC results.
Date.UTC(-1,0) has an independent absolute expected timestamp of -2240524800000.
Reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/date.rs, Date::new. The upstream avm1/date/output.txt also
records the negative-year adjustment; its SWF header is 17 and it is not run as
a SWF<=8 oracle. Fractional component rounding and full compatibility remain open.

Тест проверяет указанные отрицательные годы, нулевой год и границу 99/100.
Основание — Date::new в исходниках эталона; upstream Date SWF версии 17 не
используется как исполняемый эталон SWF<=8. Дробные компоненты и полная
совместимость Date остаются открытыми.

Validation / Проверка: Linux GCC/G++ 13.3 C90/C++98: AGG 373, Cairo Release
372 tests passed (six/three execution-permission retries). Seven Date tests
passed in NO_OS_PORTS and heap/global ASan + UBSan; ASan stack and leak checks
remain disabled. No non-Linux runtime equivalence is claimed.
