# v0.12 validation / Проверки v0.12

Adds native String metadata, immutable internal contents, valueOf/toString,
UTF-16 length/indexing/search/ranges, concat, fromCharCode, primitive unwrapping,
instanceof and graph transfer. Property-read/native-method bridges catch allocation
failure during lookup and execution. See STANDARD_STRING.md and its Russian
counterpart for the tested subset and explicit representation limits.

Добавлены нативный String, неизменяемое внутреннее содержимое, valueOf/toString,
длина/индексы/поиск/диапазоны UTF-16, concat, fromCharCode, распаковка, instanceof
и передача графов. Чтение свойств и вызов методов перехватывают отказы памяти.
Ограничения описаны в STANDARD_STRING.md и STANDARD_STRING.ru.md.

Linux x86_64, GCC/G++ 13.3.0, strict C89/C++98, CMake/CTest 3.31.10:

| Profile / Профиль | Passed / Пройдено |
| --- | ---: |
| Headless + SQLite + OS ports | 149/149 |
| AGG + fonts/codecs + SQLite + OS ports | 159/159 |
| Cairo + fonts/codecs + SQLite + OS ports | 159/159 |
| No OS/graphics/fonts/codecs/SQLite | 141/141 |
| Strict runner, ASan heap/global + UBSan, no SQLite | 146/146 |

Tests cover C and executable AVM1 construction/method calls/instanceof, ASCII,
Cyrillic and supplementary characters, surrogate code units, rejected isolated
surrogates/NUL, immutable length, empty/negative/reversed ranges, repeated search,
fromCharCode wrapping, generic receivers and conversion throws, prototype mutation,
GC after global removal, cycles and callback/getter restoration through queues,
independent-process file restoration without installing the destination library,
and malformed string packets with independently repaired valid CRCs. Allocation
injection covers atomic installation, Error/Number/String constructors, String
methods and length lookup, with undefined output on failure and successful retries.

Проверены C и исполняемый AVM1, ASCII/кириллица/символы вне BMP, кодовые единицы
суррогатной пары, отклонение одиночных суррогатов и NUL, длина без записи,
пустые/отрицательные/перепутанные диапазоны, повторяющиеся шаблоны, fromCharCode,
обычные получатели и исключения, изменение прототипа, GC после удаления имени,
циклы и методы/getter в очереди, чтение файла независимым процессом без стандартной
библиотеки и повреждённые строки с корректно пересчитанной CRC. Инъекция отказов
проверяет установку, три конструктора, методы и length, очищенный выход и повтор.

The AGG suite was repeated after two executables lost launch permissions in this
environment; the final full suite passed. Sanitizers retain the explicit v0.11
restrictions: all ASan stack checks and LeakSanitizer disabled, heap/global ASan
and UBSan enabled; container_exceptions is UBSan-only. SQLite is disabled only in
that sanitizer profile and tested normally above. A full sanitizer run on an
unrestricted host and Windows/macOS/Android execution remain outstanding.

AGG проверен повторно после восстановления прав запуска двух файлов; итоговый
набор прошёл. В санитайзерах сохранены ограничения v0.11: выключены стек ASan
и LeakSanitizer, включены куча/глобальная память ASan и UBSan. container_exceptions
проверяется только UBSan. SQLite отключён в этом профиле, но проверен обычными
сборками. Требуются проверки без ограничений среды и на целевых платформах.

Generated logs, copied profiles and JSON are delivered separately in
FlashVM-validation-v0.12.zip and are not committed. The source archive preserves
the complete internal Git history. Before this development step, the authorized
mirror update was fetched and verified at v0.11 commit
fde2f0003b64b450ab2c13491d6cbe3afe8ec945. v0.12 is not pushed automatically.

Логи/профили/JSON выдаются отдельным архивом и не коммитятся. Основной архив
содержит исходники и историю внутреннего Git. Зеркало по команде проверено на
v0.11 до начала этого этапа; v0.12 автоматически не отправляется.

String remains incomplete: a length-aware full UTF-16 representation, split/
Array, Unicode case mapping, primitive dispatch, property attributes and versioned
semantics are pending. Complete SWF 8, all Flash objects and JIT are not claimed.

String ещё не завершён: остаются полное UTF-16 с явной длиной, split/Array,
Unicode-регистр, методы примитивов, атрибуты и семантика версий. Готовность полной
SWF 8 VM, всех объектов Flash и JIT не заявляется.
