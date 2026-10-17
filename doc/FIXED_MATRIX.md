# Context-free fixed matrix arithmetic / Независимая fixed-point арифметика матриц

v0.337 adds include/flashvm/fixed.h with two C89 services:
[fvm_fixed_matrix_point](../include/flashvm/fixed.h#L28) and [fvm_fixed_matrix_multiply](../include/flashvm/fixed.h#L33). No context, values,
bytecode, operand stack, GC, callbacks or allocation are needed. Generated code
and future scene implementations can call the same exported C functions.

[FVM_FixedMatrix](../include/flashvm/fixed.h#L15) carries raw signed 16.16 a/b/c/d coefficients and signed 32-bit
tx/ty translations in twips. Point inputs and outputs are signed integer twips.
long carries these values on both 32-bit and wider-long hosts; values outside
the signed 32-bit range are rejected. This struct is a native C descriptor,
not a cross-platform wire serialization format.

Each scalar product computes floor(a*b/65536), then retains the low 32 bits.
Negative fractional products therefore round down, unlike input pixel-to-twip
truncation toward zero. Products are truncated separately before addition;
coefficients and translations wrap modulo 2^32. Composition computes left*right.
Because of intermediate truncation, applying two matrices to a point in separate
steps need not equal applying their quantized composed matrix once.

The implementation uses unsigned 16-bit limb decomposition and explicit signed
word decoding. It needs neither long long nor implementation-defined signed
right shifts/out-of-range unsigned-to-signed casts. Unsigned wrapping and final
masks work with 32-bit and wider words. Meaningful representation bounds, shifts
and masks use documented uppercase macros in public/private scoped headers.

Point outputs must be distinct and may alias matrix fields. A composed output
may alias either input. Invalid arguments preserve outputs. No input object is
changed before the complete result is computed.

Primary source comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
swf/src/types/fixed.rs wrapping_mul/wrapping_mul_int and
swf/src/types/matrix.rs matrix/point products. The C service deliberately defines
wrapping arithmetic even where Ruffle debug operators may diagnose overflow.
No Flash Player oracle is claimed. These functions are not yet wired into the
native double scene matrix, point conversion, drag or raster paths. Full scene
fixed-point storage/composition, inverse policy, JIT backends and all roadmap
stages remain open.

В v0.337 добавлены два C89-сервиса [fvm_fixed_matrix_point](../include/flashvm/fixed.h#L28) и
[fvm_fixed_matrix_multiply](../include/flashvm/fixed.h#L33). Они не требуют VM-контекста, байткода, стека,
[FVM_Value](../include/flashvm/value.h#L24), GC или выделения памяти и доступны будущему JIT через обычный C ABI.
Коэффициенты — raw signed 16.16, координаты и переносы — signed 32-bit twips.
Тип long служит переносчиком; на широких хостах значения вне 32-битного
диапазона отклоняются. Структура не является форматом сетевой сериализации.

Произведение сдвигается на 16 бит с округлением отрицательной дроби вниз;
каждое слагаемое обрабатывается отдельно, результат и суммы замыкаются по
модулю 2^32. Это отличается от усечения входных пикселей к нулю. Использованы
беззнаковые 16-битные части, без long long, signed shift и небезопасного
приведения unsigned к signed. Границы и маски — пояснённые макросы.

Поддержано наложение выходов на входную матрицу, при ошибках выходы сохраняются.
Проверка по исходникам Ruffle не заменяет Flash Player. Сцена, drag и
преобразование точек пока используют прежние double-матрицы; подключение
нового числового слоя, inverse, JIT и завершение стадий ещё впереди.

Validation: Linux GCC 13.3 Release/default AGG passed 661/661 CTest tests.
fixed_matrix compiles as C89 and checks 297 independent arbitrary-precision
scalar vectors, 26 matrix products, separately shifted half-products, signed
wrap endpoints, aliasing and invalid/range errors. fixed_matrix_words runs the
limb products with unsigned int (32 bits here) and native unsigned long (64 bits).
fixed_matrix_no_alloc calls both APIs without a context while all new/new[]
allocations are forbidden. The fixture generator's --check reproduced the data.

A separate C89 caller linked and ran with only the two C++98 implementation
objects, including successful linking through cc without libflashvm or an
explicit C++ runtime. All three new tests also passed minimal Debug ASan
heap/global + UBSan. Stack instrumentation and LeakSanitizer remain disabled
under the previously independently reproduced environment limitation.

Both implementation units compiled with GCC -m32, strict C++98 and freestanding
flags to verified ELF32 i386 objects. A freestanding 32-bit vector probe linked,
but this environment rejected its execution with Exec format error: no i386
runtime result is claimed. No Windows/macOS/Android, Cairo or Flash Player run.

Проверки: 661/661 Linux Release/AGG, три новых теста ограниченного ASan
heap/global + UBSan. C89-тест проверяет 297 скалярных векторов и 26 матриц
по независимому Python-оракулу; отдельно проверены 32-битные и широкие
беззнаковые промежуточные слова, aliasing, границы и запрет аллокаций.
Воспроизводимость fixtures подтверждена --check. Отдельный C89 caller собран
и выполнен только с двумя C++98 объектными файлами, без библиотеки VM.

Оба файла также компилируются с -m32 в ELF32 i386, но запуск freestanding
32-битного probe отклонён средой (Exec format error). Это не подтверждение
32-битного исполнения. Стек ASan, утечки и остальные платформы не проверены.

## v0.338 scene representation clarification

SCENE_MATRIX.md distinguishes serialized SWF fixed coefficients from the binary32
matrix actually exported by Ruffle core/prelude.rs for display objects. The
v0.337 functions implement their declared raw fixed arithmetic; they are not
wired into the scene point path. v0.338 adds separate binary32 C services for
that purpose. Future scene work should preserve this representation distinction.

В v0.338 уточнено различие сырых SWF 16.16 и binary32 матрицы сцены.
Для точек сцены добавлены отдельные C-функции; механическая подмена их
16.16-функциями v0.337 не выполняется. См. SCENE_MATRIX.md.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_fixed_matrix_multiply` | function | [include/flashvm/fixed.h:33](../include/flashvm/fixed.h#L33) |
| `fvm_fixed_matrix_point` | function | [include/flashvm/fixed.h:28](../include/flashvm/fixed.h#L28) |
| `FVM_FixedMatrix` | struct | [include/flashvm/fixed.h:15](../include/flashvm/fixed.h#L15) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
