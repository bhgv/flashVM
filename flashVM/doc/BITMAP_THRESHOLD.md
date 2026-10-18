# BitmapData.threshold

## Русский

v0.54 добавляет threshold(source, sourceRect, destPoint, operation, threshold,
color=0, mask=0xFFFFFFFF, copySource=false). Пиксели сравниваются как
unsigned 32-bit: (pixel & mask) operation (threshold & mask). Поддержаны
<, <=, >, >=, ==, !=. Совпавшим destination пикселям присваивается color;
несовпавшие получают source при copySource=true, иначе остаются прежними.
Возвращается число прошедших сравнение eligible пикселей, даже если новый
цвет совпадает с текущим. Opaque destination сохраняет alpha=255.

Общий bitmap_region модуль копирует публичные Rectangle/Point поля,
применяет floor и согласованное clipping исходника/получателя. Отбрасываются
координаты вне signed-32 range без пиксельного доступа; empty region даёт 0.
Весь eligible source RGBA snapshot строится до первой записи, поэтому
перекрытие source==destination не распространяет уже записанный пиксель.
Snapshot bytes проверяются против memory_limit; это не полная live VM квота.
После последнего выделения памяти нет callbacks или аллокаций, способных
прервать публикацию. OOM сохраняет destination. User getter/coercion side
 effects не откатываются.

Все параметры читаются до окончательного storage borrow. Disposed/invalid
native source или receiver после coercion дают -1. Ошибки аргументов,
неподдержанный оператор, в том числе строка с NUL/suffix, и nonfinite numeric
значения отклоняются. Native callback ID 1543 сохраняется при переносе bitmap
в receiver без standard_install. Метод работает без графического backend.

bitmap_threshold.cpp проверяет шесть сравнений на переходе unsigned sign bit,
copySource/defaults, opaque alpha, mask=0, aligned negative clipping,
перекрывающийся alias, invalid operator/NUL, snapshot budget, disposed source
и GC/queue. bitmap_threshold_allocation.cpp инъецирует отказы new/new[]
coercion/snapshot, проверяет destination rollback и new/delete balance.
Профили: headless, AGG, Cairo, portable и heap/global ASan+UBSan.

Открыто: точные Flash coercion/fractional clipping/premultiplied-alpha правила,
оставшиеся bitmap/filter/text/media API. Пункт 4 не завершён.

## English

v0.54 adds threshold with six unsigned 32-bit masked comparisons, replacement
color, optional copying of unmatched source pixels and a successful-test
count. Opaque destination alpha stays 255. Defaults are color=0,
mask=0xFFFFFFFF and copySource=false.

The shared region module copies public Rectangle/Point tuples and applies
floor plus aligned source/destination clipping. Out-of-range coordinates
produce an empty no-op. All eligible source pixels are snapshotted before
writes, preserving overlapping aliases and failed-allocation rollback.
Snapshot bytes obey the memory limit without claiming complete live quotas;
user callback side effects are not rolled back.

Storage is refreshed after coercion. Invalid/disposed native buffers return
-1; invalid operators, NUL-suffixed names and nonfinite numbers are rejected.
Callback ID 1543 and captured class graphs support queue transfer without
standard installation. Graphics backends are not required.

Tests cover comparisons across the sign-bit boundary, masks/defaults/copy,
opaque alpha, clipping/aliases, malformed options, budgets, disposal,
GC/queue and allocation rollback/balance. Headless/AGG/Cairo/portable and
heap/global ASan+UBSan are checked. Exact Flash coercion, fractional clipping,
premultiplied alpha and remaining bitmap/filter/text/media APIs remain open.
