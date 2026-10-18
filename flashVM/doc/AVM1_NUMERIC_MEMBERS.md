# Numeric member keys / Числовые имена членов

AVM1 GetMember/SetMember now use ordinary string-key coercion for the global
object and standard resources such as Number and MovieClip. They no longer
redirect every nonnegative integral operand into a separate native integer map.
Standard Object/Array/arguments keep their existing canonical indexed bridge,
including prototype accessors and property flags. Explicit raw native tables
retain their historical integer/named-slot distinction as a FlashVM extension.

Числовое имя члена глобального объекта и встроенных ресурсов преобразуется
в строковый ключ. Прежний общий быстрый путь мог записать `_global[7]` в
отдельный слот, невидимый для свойства "7". Теперь эти обращения совпадают.
Общий мост Object/Array сохраняет прототипы, getter/setter и READ_ONLY;
у нативных таблиц целочисленные слоты остаются отдельным расширением.

Regression avm1_numeric_members first reproduced the global lookup mismatch.
It covers Object own/prototype/accessor/flag behavior, negative zero and Uint32
max names, NUL-name distinction, channel graph restore, global/Number/MovieClip
properties and legacy native-table separation. Native table C APIs remain
available; full primitive/resource property conformance is still open.
