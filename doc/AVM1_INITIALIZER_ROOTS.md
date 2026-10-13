# Корни и имена инициализаторов / Initializer roots and names — v0.127

InitObject удерживает частично построенный результат отдельным корнем GC
до помещения на стек. Временные значение, имя и преобразованная строка также
закреплены: преобразование имени и setter назначения могут вызвать скрипт.
InitArray удерживает созданный результат и копию элементов при назначении
индексов. В загруженном SWF имя InitObject преобразуется общим AVM1-помощником,
включая undefined → пустую строку в SWF6 и undefined → «undefined» в SWF7.
Режим C без загруженного SWF сохраняет прежнее преобразование.

Тест gc_avm1_initializers выполняет 20 вызовов скриптового toString, каждый
вызывает gc.collect и проверяет число собранных объектов не менее одного.
После возврата проверяются результат и оба свойства; проверяется восстановление
цепочки корней. Отдельно проверяются имена undefined для SWF6/7. Тест
подтверждает реальную сборку во время преобразования, а не только её запрос.

Остаются поведение нехватки операндов, предел материализации 65 536,
конструкторы и происхождение версии. Исторические setter/own-data правила
литералов ещё требуют полной сверки. Все стадии роудмапа открыты.

## English

InitObject roots its partially constructed result until publication on the
operand stack. Temporary values, names and converted strings also remain rooted
through script coercion and destination setters. InitArray roots its result and
copied elements during indexed assignment. Loaded InitObject names use the
shared AVM1 string conversion; undefined names are empty in SWF6 and spelled
undefined in SWF7. Unloaded host conversion remains unchanged.

gc_avm1_initializers runs twenty script toString callbacks invoking gc.collect,
asserting at least one collected object on every call. It checks both resulting
properties and root-chain restoration, plus SWF6/7 undefined names. This verifies
actual collection during coercion. Missing operands, the materialization cap,
constructor/version provenance and complete historical setter rules remain open,
as does every roadmap stage.
