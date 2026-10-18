# ASSetPropFlags conversion / Преобразования ASSetPropFlags

In loaded SWF 4–8, ASSetPropFlags first projects a supplied target through current
global boxing constructors. An object remains unchanged; nullish, missing-class
and nonobject constructor results become a fresh bare object. With no target
argument it returns undefined immediately. The unloaded host API still requires
an object and retains its existing numeric validation.

Operands are evaluated in target, set flags, clear flags, property-list order.
Set/clear objects use AVM1 numeric conversion. Loaded flag Numbers narrow using
saturating UInt16 conversion: NaN, negative values and negative infinity become
zero; positive fractions truncate; positive overflow/infinity saturate. The
implementation still rejects bits outside DONT_ENUM/DONT_DELETE/READ_ONLY (0–7).
Version/visibility bits and full extended ASSetPropFlags behavior remain open;
narrowing does not imply those bits have been implemented.

A null list selects current own data/method/accessor/index slots. Other lists
use versioned string conversion and split at commas without losing embedded NUL.
Flag/name conversion side effects occur before the target attribute snapshot.
Set bits take precedence over clear bits. The copied flag table is published
only after every selected update succeeds. Conversion side effects are not
rolled back, but an allocation failure does not publish partial flag updates.

The projected target is rooted through argument callbacks. Registration retains
its existing native collector barrier; collection attempts inside conversion
callbacks are rejected. Tests separately collect after an error before retry.
No public ABI or serialized callback identity changes.

В SWF 4–8 сначала преобразуется целевой объект через текущий глобальный
конструктор, затем set-флаги, clear-флаги и список свойств. Для nullish или
необъектного результата создаётся объект без прототипа. Без аргументов результат
сразу undefined. Host без SWF сохраняет строгие прежние правила.

Числовые флаги сужаются с насыщением до UInt16: NaN и отрицательные значения
дают ноль, дробные значения усекаются, переполнение насыщается. Поддержаны
только биты 0–7; version/visibility-флаги пока отклоняются. Побочные эффекты
преобразования списка предшествуют снимку атрибутов. Сначала очищаются биты,
затем устанавливаются; новая таблица публикуется целиком. При отказе памяти
старые атрибуты сохраняются, но побочные эффекты пользовательских функций
не отменяются. Нативный GC-барьер во время преобразований сохранён.

Tests: property_flags_global_boxing, property_flags_conversion,
property_flags_allocation, standard_property_flags. These checks do not close
any roadmap stage. Reference: pinned Ruffle AVM1 globals/object.rs
as_set_prop_flags, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.

## v0.480: stored spelling and flag-only case folding

ASSetPropFlags property-list lookup is case-insensitive in every loaded SWF,
including versions 7–8. This differs from ordinary property reads. The selector
finds own data/accessor/native-method slots without evaluating a getter and stores
attributes under the selected slot's actual name. Duplicate aliases update the
same staged entry; an inherited-only name remains untouched. The unloaded host
path retains exact matching. Ordinary property lookup is unchanged.

Canonical storage prevents an update under `mixed` from leaving stale flags on
`MiXeD` or creating a detached attribute entry. It also preserves the flags when
a legacy-context graph is deserialized into a case-sensitive SWF8 context. Tests
cover initially absent/existing/cleared attributes, set-bit precedence, ASCII,
counted NUL/Unicode and long names, data/accessor/method slots, enumeration,
transport/GC, C and three AVM1/JIT modes, plus allocation failures before commit.
Ordering when multiple distinct own names collide under case folding remains an
unverified compatibility case; the implementation retains its existing own-slot
selection preference. Extended version flags remain unsupported.

В ASSetPropFlags имена сравниваются без учёта регистра во всех загруженных SWF,
включая 7–8. Атрибуты записываются под сохранённым именем выбранного собственного
свойства. Повторные псевдонимы не создают отдельные записи, свойства прототипа
не изменяются, getters не вызываются. Поэтому перенос в SWF8 сохраняет флаги.
Обычный доступ к свойствам и host-поведение не менялись. При отказе памяти новая
таблица не публикуется. Порядок выбора при нескольких конфликтующих собственных
именах и дополнительные version-флаги остаются отдельными задачами.

Reference: pinned Ruffle ScriptObject.set_attributes uses
PropertyMap.get_mut(name, false), independent of activation version.
Tests: property_flags_case, property_flags_case_allocation.
