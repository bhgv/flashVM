# Host URL requests / Запросы URL приложению

v0.109 adds a synchronous C89 host request bridge for ActionGetURL (0x83) and
ActionGetURL2 (0x9a). `fvm_avm1_set_url_rooted` registers a callback plus optional
same-context capture. The callback receives the global receiver and three
values: counted STRING URL, counted STRING target/FSCommand arguments, NUMBER
raw flags. None/GET/POST use the low two bits; load-target/load-variables use
64/128. Reserved bits and method 3 are rejected. GetURL passes zero flags and
validates exactly two terminated payload strings. GetURL2 coerces target first,
URL second. Callback status propagates and its result is ignored. Without a
sink the actions return FVM_UNSUPPORTED, after required operand coercions.

[GC:] The registration roots its capture; a request root frame retains converted
strings during later script coercion. Delivery uses the ordinary native collector
barrier. Callback replacement affects subsequent requests; null callback,
userdata and capture disable delivery. Borrowed strings must be copied if retained;
do not destroy the context from its callback. C++ allocation exceptions become
FVM_OUT_OF_MEMORY and other exceptions become FVM_ERROR at interpreter entry.

This bridge performs no navigation, network access, FSCommand dispatch, movie
loading or variable loading. The host decides how to handle the raw request.
Automatic form serialization from AVM1 locals is not implemented. `_level`
target policy and completion/event dispatch are also open. Literal non-ASCII
GetURL payloads before SWF 6 are rejected pending locale codecs. This is tested
request dispatch, not complete browser/network AS1/AS2 behavior.

Приложение получает три значения: URL, target/аргументы FSCommand и флаги.
Порядок toString — сначала target, затем URL; NUL в строках GetURL2 сохраняется.
При отсутствии обработчика возвращается UNSUPPORTED. Загрузка ресурсов,
передача локальных переменных формы и асинхронные события ещё не реализованы.

avm1_url exercises counted strings, flags, literal FSCommand requests, capture
retention/release, cross-context rejection, coercion order, actual script GC,
replacement, callback errors/exceptions and malformed input. avm1_url_api verifies
public declarations/constants under C89. All 271 core tests pass in AGG/Cairo,
plus the optional runner test in AGG; one Cairo executable needed its stripped
execute bit restored and passed the rerun. Four focused tests pass in each
portable and ASan/UBSan profile (stack/leak sanitizer restrictions unchanged).
All ten previous original external SWF probes still complete and match exactly.
