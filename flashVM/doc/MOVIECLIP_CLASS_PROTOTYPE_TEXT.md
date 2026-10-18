# MovieClip class-prototype text / Строка прототипа класса MovieClip

v0.252 distinguishes the retained builtin MovieClip.prototype from a live display
instance during loaded string conversion. Its default method produces Object
text, not `_level0`; user toString overrides follow ordinary object conversion.
Real MovieClips retain intrinsic display-reference projection. Unloaded host
conversion keeps its existing policy.

A documented private Boolean marker is stored in managed MovieClip state when
the builtin prototype is staged. The lookup does not allocate or invoke script.
New clips receive fresh state, so the marker is not inherited by instances. GC
and graph transport retain the private value. The public resource kind and other
prototype/typeof/display APIs are unchanged; their complete historical behavior
remains separate compatibility work.

The new fixture covers SWF4–8 default/direct text, real clips, collection, graph
transfer, custom prepared-native-script toString and exceptions with unchanged
C output. The unchanged pinned Ruffle prototype_delete SWF8 now matches exactly;
its source/oracle/header are not rewritten. Source identity is commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`, tests/tests/swfs/avm1/prototype_delete.

Прототип встроенного класса MovieClip при преобразовании в строку отличается
от живого клипа: штатный результат — текст Object, а пользовательский toString
вызывается как у объекта. Обычные клипы сохраняют путь. Признак хранится только
в закрытом состоянии исходного прототипа, удерживается GC и переносится через
канал. Тип ресурса и остальные API не меняются. Полная семантика прототипов,
включая typeof и display-интерфейсы, остаётся открытой.

Evidence: 462 tests pass in each Linux AGG/Cairo profile; five AGG execute-bit
launch failures pass after permission recovery. Six focused heap/global
ASan+UBSan checks pass (no stack/leak instrumentation). The expanded original
SWF<=8 set now matches 25/25 per profile, including three-frame delete and
prototype_delete. This remains a selected corpus, not complete Flash conformance.
