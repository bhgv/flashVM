# Длина splice / Loaded splice own-data length — v0.135

Историческая запись v0.135; изменение v0.373 описано ниже.
Historical v0.135 checkpoint; see the v0.373 update below.

Загруженный splice читает собственную хранимую длину и преобразует её в signed
Int32 после start и до deleteCount. Getter и длина прототипа не используются;
положительная wrapped-длина 2^32+3 означает 3. Отсутствующая data-длина означает 0.
Собственная native Array-длина читается из состояния массива.

Отрицательная wrapped-длина пока возвращает UNSUPPORTED до преобразования
счётчика удаления и до мутации. Это явная граница: исторический Flash допускает
отрицательные индексы/длины мутации, а текущая модель native Array хранит длину
в unsigned-диапазоне. Отрицательный случай не превращается в нулевую мутацию
и не называется повреждением данных. Раннее unsupported-отклонение отличается
от полной последовательности исторического исполнения и документировано.
Незагруженный C-режим сохраняет прежнее property-aware/unsigned чтение.

array_legacy_splice_length проверяет SWF6..8: игнорирование virtual/inherited
length, сохранность существующего индекса при нулевой собственной длине,
положительное оборачивание, отрицательную длину без мутации/вызова deleteCount
valueOf и восстановление корней/глубины. Поддержка отрицательных signed-мутаций,
исторический set_data/move, все версии и все стадии остаются открытыми.

## English

Loaded splice captures own stored length with signed Int32 wrapping after start
and before deleteCount. Virtual/inherited length is ignored; 2^32+3 wraps to
three. Missing data length is zero; native Array length comes from its state.

Negative wrapped lengths explicitly return UNSUPPORTED before deleteCount
coercion or mutation. Historical signed-index mutation needs a different native
length model; this boundary avoids silently substituting a zero mutation or
labelling valid data malformed. Early rejection differs from complete historical
execution ordering. Unloaded host mode keeps property-aware unsigned length.
The SWF6..8 test covers virtual/inherited length, preserved existing indices at
zero own length, positive wrapping, negative no-mutation/no-delete-coercion and
root/depth restoration. Negative signed mutations, set_data/movement, full
version conformance and every roadmap stage remain open.

As of v0.373 the borrowed negative-length restriction is narrowed: see
[negative splice](ARRAY_NEGATIVE_SPLICE.md). Native Array negative storage remains unsupported.

С v0.373 отрицательная длина заимствованного splice поддерживается в пределах
[нового контракта](ARRAY_NEGATIVE_SPLICE.ru.md); ограничение настоящего Array сохраняется.

As of v0.376 loaded generic mutation loops no longer reject lengths above
65536 solely by traversal size. Host and sorting bounds remain.
С v0.376 загруженные мутации общего пути ограничиваются бюджетом вместо
предела длины 65536; [EN](ARRAY_LOADED_LARGE_MUTATION.md) / [RU](ARRAY_LOADED_LARGE_MUTATION.ru.md).
