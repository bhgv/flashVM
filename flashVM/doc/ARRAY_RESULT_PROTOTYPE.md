# Прототип результатов Array / Array result prototype — v0.120

После успешной установки стандартной библиотеки контекст сохраняет встроенный
прототип Array как отдельный корень GC. Построители результатов slice/splice/
concat, индексного результата sort, String.split и AVM1 InitArray используют
его, а не пользовательский прототип массива-источника или заменённое поле
публичного конструктора. Getter `Array.prototype` не вызывается для этих
результатов. Кэш публикуется только при успешном завершении установки.

Явный конструктор Array по-прежнему читает публичный `prototype`; его повторное
чтение при внутреннем выделении массива устранено. Сам вызов `Array()` как
функции пока использует тот же путь конструктора: различие исторических правил
обычного и конструктивного вызовов остаётся открытым. Контексты без установленной
стандартной библиотеки сохраняют прежний fallback для импортированных методов;
полное согласование скрытых прототипов с графовым переносом ещё не заявляется.

`array_result_prototype` удаляет публичную ссылку на исходный прототип, заменяет
её getter и выполняет настоящую GC до получения методов. Проверяются жизнь
прототипа и единственная финализация при уничтожении контекста, отсутствие
посторонних полей источника у результата, шесть построителей и единственное
чтение getter явным конструктором. Порядок построения сопоставлен с
[ArrayBuilder primary source](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Публичный C ABI не изменён. Исторические размеры/дырки, остальные классы и все
стадии роудмапа остаются открытыми.

## English

Successful standard installation retains the original Array prototype as an
explicit context GC root. Slice/splice/concat, sort index previews, String.split
and InitArray result builders use it rather than a custom source prototype or
replacement public Array.prototype, without invoking that public getter.
The cache is published only when standard installation succeeds.

Explicit Array construction still honors the public prototype and now reads
its getter once. Plain Array() calls still share the constructor path; that
historic distinction remains open. Uninstalled contexts retain existing
imported-method fallback; complete hidden-prototype transport is not claimed.

The named test removes the public original-prototype edge and runs real GC
before fetching methods, checking survival and one finalization at context
teardown. It checks all six builders, result isolation and constructor reads.
The pinned primary builder source is linked above. Public C ABI is unchanged;
historic sizes/holes, other class behavior and every roadmap stage remain open.
