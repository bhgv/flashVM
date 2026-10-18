# Point conversion C service / C-сервис преобразования точки

v0.333 extracts the existing localToGlobal/globalToLocal operation from the large
MovieClip implementation into handlers/clip_point.cpp. The value-based C89 entry
[fvm_exec_clip_point](../include/flashvm/execution.h#L65)(context, clip, reverse, point) accepts an object-valued point;
reverse=0 projects local pixels to physical world pixels, nonzero performs the
inverse. The methods retain their existing positions in the native callback table,
so serialized identities do not change. Each native adapter occupies its own file.

The service roots clip and point, holds a native collection barrier, reads/coerces
x then y, queries the physical world matrix and writes x then y. Callback errors
propagate. A y-setter failure leaves an already-completed x write intact; this is
an ordered mutation, not an atomic pair. Source coordinate values are captured
before setters run. Singular inverse fails before coordinate writes. Direct C
and generated code do not need bytecode, stack layout or Action numbers.

This step preserves the existing native policy, including inherited/virtual point
fields and coercion, invalid/missing argument errors and singular-matrix errors.
Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/movie_clip.rs local_to_global/global_to_local instead reads
raw numeric own fields without coercion and uses a singular inverse fallback.
Those loaded-Flash policy differences and exact twip conversion remain open;
this extraction makes no new Flash semantic compatibility claim.

В v0.333 обработчики localToGlobal/globalToLocal вынесены в общий C89-сервис
[fvm_exec_clip_point](../include/flashvm/execution.h#L65) и два отдельных нативных адаптера. Прежние позиции callbacks
сохранены. Сервис принимает клип, направление и объект-точку, без байткода и
стека интерпретатора; координаты задаются в пикселях физической сцены.

Сначала читаются и преобразуются x/y, затем запрашивается матрица, затем
последовательно записываются x/y. Ошибка setter y сохраняет завершённую запись
x. Корни и нативный барьер защищают callbacks; ошибки и вырожденная обратная
матрица возвращаются вызывающему коду. Это декомпозиция прежнего поведения.
Различия с сырыми числовыми собственными полями Flash, округлением twips и
историческим fallback вырожденной матрицы ещё не устранены; стадии открыты.

Validation: Linux GCC 13.3 Release/default AGG passed 655/655 CTest tests.
execution_clip_point is C89 and checks host/SWF4–8 direct/native projections,
nested scale/rotation/translation, inverse roundtrip, singular matrices and
invalid arguments. execution_clip_point_callbacks checks the shared getter/setter
order, exception propagation, partial x publication and GC/native/root restoration.
execution_clip_point_allocation injects all direct/native allocation failures,
accepting only the documented prefix of coordinate writes and checking state
restoration and later collection. All three passed minimal Debug ASan heap/global
+ UBSan with stack instrumentation/LeakSanitizer disabled as established by the
independent exception probe. No other-platform/Flash Player run is claimed.

Проверки: 655/655 Linux Release/AGG и 3/3 новых тестов минимального Debug
ASan heap/global + UBSan. Проверены C89, host/SWF4–8, матрицы, ошибки,
порядок callbacks, GC-барьер и отказы памяти. Проверки стека ASan, утечек,
других платформ и Flash Player в этом прогоне не заявляются.

## v0.334 follow-up

Loaded point fields, invalid/missing no-ops and singular fallback now follow the
bounded policy in AVM1_POINT_FIELDS.md. The original v0.333 extraction behavior
above remains historical; host-only behavior is retained. Exact twips/nonfinite
conversion and old case-folded property spelling are still open.

В v0.334 политика загруженных SWF уточнена: собственные числовые поля,
no-op для отсутствующих/неподходящих аргументов и fallback вырожденной матрицы.
Описание v0.333 выше историческое; точность twips и другие перечисленные
границы не объявляются завершёнными.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_point` | function | [include/flashvm/execution.h:65](../include/flashvm/execution.h#L65) |
<!-- source-links:end -->
