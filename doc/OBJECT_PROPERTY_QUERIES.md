# Property query names / Имена в запросах свойств — v0.151

hasOwnProperty и оба enumeration alias приводят имя ровно один раз. В загруженном
SWF используется AVM1 string bridge, включая undefined → empty до SWF7. Без
аргумента возвращается false, даже если объект имеет поле "undefined". Проверка
own presence и flags использует одну counted строку; getter не требуется.
Двойное toString в propertyIsEnumerable/isPropertyEnumerable устранено.

object_property_queries проверяет незагруженный режим и SWF4..8, меняющийся
результат toString, отсутствие аргумента против явного undefined, заранее
созданное empty host поле, hidden flags, оба alias и counted NUL имена. GC
после проверок дополняет существующие rooted-name тесты. Case/version masks,
полные native fields/prototype правила, function provenance и все стадии открыты.

## English

hasOwnProperty and both enumeration aliases convert names exactly once, using
loaded AVM1 string rules (undefined becomes empty before SWF7). Missing arguments
return false even when a field named "undefined" exists. Own presence and flags
use the same counted key without getters. This removes duplicate toString calls
from propertyIsEnumerable/isPropertyEnumerable. Tests cover unloaded mode/SWF4..8,
changing toString results, missing versus explicit undefined, preexisting empty
host data, hidden flags, both aliases and counted NUL names, followed by GC.
Case/version masks, full native fields/prototype rules, function provenance and
all stages remain open.
