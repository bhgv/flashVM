# Counted UTF-16 strings / Строки UTF-16 — v0.16

## English

ABI 3 appends string_size to FVM_Value. Rebuild native clients; initialize values
through the public helpers. Legacy fvm_value_string counts bytes up to NUL;
fvm_value_string_bytes borrows counted UTF-8/WTF-8 and requires no terminator.
fvm_value_string_utf16 copies UTF-16 into context-owned storage. Query/copy code
units with fvm_value_get_utf16; an insufficient buffer is left unchanged.
Returned byte storage is terminated for convenience, but string_size is required
for embedded NUL. Strict utf8.h scalar functions remain strict. VM strings can
also retain unpaired surrogates through the WTF-8 representation.

String length, charAt/charCodeAt, substring/slice/substr, search, concat, split,
fromCharCode and conversion preserve UTF-16 units. Splitting an astral character
can return its individual surrogate units. AVM1 Add2, equality, ordering and truth
conversion use counted strings; scalar/pair storage variants compare by units.
Boolean of a one-unit NUL string is true; Number requires the whole input and
returns NaN for embedded NUL. Array.join and sorting retain full strings.

The bundled Unicode 3.2 data in external/unicode supplies default full upper/lower
mappings, including expansions and contextual final sigma. The methods are
independent of the host locale, fonts and NO_FONTS. Language-specific casing is
not used. CASEINSENSITIVE Array sort/sortOn uses the same lowercase conversion.
Regeneration: python scripts/generate_unicode_case.py. Data, license and source
hashes are bundled; the generated table is compiled statically without Python.
This pinned behavior still needs historical Flash-version reference comparisons.

Graph writer version 4 permits NUL in value strings and WTF-8; identifiers remain
separate validated records. Readers accept versions 1–4. Versions 1–3 do not
support counted NUL values. Object/primitive queue copies, worker input/output,
exceptions and graph restores preserve byte counts. Independent file/TCP graph
process tests restore String instances containing NUL and lone surrogates into
contexts without standard installation.

Outstanding stage-1 gates: length-aware property/function names and equivalent
UTF-16 key representations, exact version-aware case/numeric conversion behavior,
borrowed Array methods, inherited sparse slots, registerClass and the full
compatibility inventory. TextField's scalar text model is still pending in stage
4; it rejects embedded NUL instead of silently truncating it. Existing graphics,
font and image dependencies keep their versions.

Tests: standard_utf16.c, standard_string.c, string_wire.c,
objects_transfer_process.c, serialization_validation.c, array_sort.c and
error_allocation.cpp. No full stage completion is claimed.

## Русский

ABI повышен до 3: FVM_Value содержит длину строкового буфера string_size.
Нативные клиенты нужно пересобрать, значения инициализировать публичными
функциями. fvm_value_string работает с привычной строкой до NUL; новый
fvm_value_string_bytes принимает буфер с явной длиной без требования терминатора.
fvm_value_string_utf16 копирует UTF-16 в память контекста. fvm_value_get_utf16
позволяет запросить размер и получить единицы; слишком короткий буфер не меняется.

Методы String сохраняют NUL и одиночные суррогаты, включая charAt и split внутри
пары. Add2, равенство и сравнение AVM1 учитывают всю длину и порядок UTF-16.
Boolean строки из одного NUL равен true, Number строки со встроенным NUL — NaN.
Array.join и сортировка сохраняют полное содержимое.

Таблица Unicode 3.2 включена в external/unicode с лицензией и источниками;
применяются полные безусловные upper/lower преобразования и финальная сигма.
Результат не зависит от локали хоста и библиотек шрифтов. Языковые варианты
регистра не применяются. Для полной совместимости с версиями Flash ещё нужны
эталонные сравнения. Генератор таблицы воспроизводим; в сборке Python не нужен.

Формат графа 4 передаёт значения с NUL и суррогатами, читатель принимает 1–4.
Очереди, вход/выход потоков, исключения и восстановление сохраняют длины.
Файловые и TCP-тесты восстанавливают String в независимых процессах без установки
стандартной библиотеки у получателя.

Этап 1 остаётся открытым: нужны полноценные строковые имена свойств/функций,
эквивалентность UTF-16 представлений ключа, сверка преобразований по версиям,
заимствованные методы Array, наследуемые индексы, registerClass и полный реестр.
TextField пока использует скалярную модель текста и отклоняет NUL без потери данных;
его полная реализация относится к этапу 4. Список проверок приведён выше.

## Validation / Проверки

Linux GCC/G++ 13.3, strict C90/C++98: headless 165, static AGG 175, static Cairo
175, minimal portable profile 156; standalone ASan/UBSan runner 162 passed.
Cairo/minimal test binaries intermittently lost execution permission; affected
entries passed after permission restoration and rerun. Generated logs stay out
of Git. Sanitizers use --no-asan-stack: ASan stack checks and LeakSanitizer are
disabled due to the managed environment, container_exceptions.cpp is UBSan only,
SQLite is disabled in that profile. No full stack/leak or other-platform runtime
certification is claimed.

Проверки выполнены на Linux: 165/175/175/156 по четырём профилям, санитайзеры 162.
Отказы запуска из-за прав исполнения исправлены повторным запуском. Стек ASan и
LeakSanitizer отключены, container_exceptions.cpp проверяется только UBSan,
санитайзер-профиль без SQLite. Другие ОС и полная проверка стека/утечек этим
проходом не подтверждаются. Логи не коммитятся.

## v0.81 update / Обновление v0.81

Counted property APIs and AVM1 member operations now preserve full keys; graph 8
transports them. See [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).
Native call/registration and AVM1 variable-name gates remain open.

## v0.409: allocation-free UTF-16 size queries / Подсчёт без выделений

`String.length`, boxed String construction validation and the count-only
`fvm_value_get_utf16(value, NULL, 0, &length)` path now count UTF-16 units directly
from counted WTF-8, without an intermediate vector. Supplementary scalars count
as two; NUL and lone surrogates count as one. Invalid trailing input still fails
and leaves the public count zero. This removes temporary allocation proportional
to input size; construction itself still allocates its object and properties.
The copy-to-buffer path retains its existing behavior. Full Flash string oracle
and all roadmap stage acceptance remain open.

`String.length`, проверка при создании String и запрос только длины через
`fvm_value_get_utf16` считают UTF-16 непосредственно, без временного массива.
Дополнительный символ занимает две единицы, NUL и одиночный суррогат — одну.
Повреждённый хвост отклоняется, публичная длина остаётся нулевой. Создание объекта
и его свойств по-прежнему требует памяти. Полная совместимость Flash не заявлена.

Regression: `flashvm_tests_string_length_allocation` denies every C++ allocation
while checking supplementary/NUL/lone-surrogate, empty and malformed queries.

## v0.410: short UTF-16 buffers / Короткие буферы UTF-16

`fvm_value_get_utf16` validates and measures before allocating copy storage.
A valid string and insufficient capacity return `FVM_INVALID_ARGUMENT`, publish
its required unit count and leave output untouched even when allocations fail.
Malformed input still returns a zero count before checking capacity. Sufficient
buffers retain the decoded snapshot, including overlapping input/output; that
copy path still allocates memory and performs another decoding pass.

При недостаточной ёмкости API сообщает требуемую длину без выделений и без
записи в буфер. Повреждённая строка отклоняется с нулевой длиной. Для достаточного
буфера сохранён промежуточный снимок: перекрытие входа и выхода поддерживается,
но копирование по-прежнему выделяет память и повторяет декодирование.
Regression: `flashvm_tests_string_length_allocation` checks denied-allocation
short buffers, complete malformed-input validation, output sentinels and overlap.

Validation v0.410 / Проверка: Linux minimal Release 822/822; AGG Release
4/4 focused string tests; heap/global ASan+UBSan 4/4. Generated native launchers
restore workspace executable permissions; stack ASan and LSan remain disabled.
No other-platform runtime or complete stage acceptance is claimed.

### v0.411 — one UTF-16 snapshot allocation / Одно выделение снимка

The sufficient-capacity C API copy reserves the measured UTF-16 size exactly,
removing geometric reallocation and transient old/new decoded arrays. Snapshot
semantics for overlapping input/output remain. A 257-unit regression allowing
exactly one allocation fails on v0.410 and passes after this change.
Копирование резервирует точный размер снимка одним выделением памяти; перекрытие
буферов поддерживается. Все девять стадий остаются открытыми.

Validation v0.411 / Проверка: minimal Release 822/822; AGG Release 4/4;
heap/global ASan+UBSan 4/4 string tests. Strict C89 public-header syntax passed.
Native test launchers restore executable modes; stack ASan/LSan are disabled.
Other platforms and complete Flash compatibility remain unverified.
