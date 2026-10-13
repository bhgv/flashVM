# Владелец setter / Inherited setter ownership — v0.140

Обычная запись через унаследованный script setter вызывает его с receiver,
но не изменяет retained data дескриптора прототипа и не создаёт own data у
receiver. Собственный setter продолжает обновлять свои данные. Это включает
внутренний null-setter bridge для writable существующих свойств.

CAPI lookup перед callback публикует приватный calling_setter_owner.
Allocation-free scope удерживает lookup owner, receiver и capture/descriptor,
восстанавливает предыдущего владельца при вложенных вызовах и всех выходах.
Метаданные конструктора/метода не меняются. Владение определяется фактическим
lookup, поэтому старым wire-дескрипторам не нужно новое поле имени/владельца.
C89 ABI и структура графа не меняются; временное поле контекста не сериализуется.

object_inherited_setter_data проверяет script setter с вложенным собственным
setter и побочным эффектом у receiver, inherited null setter, counted NUL имя,
отсутствие own индексов, сохранение data прототипа, собственную запись, GC и
перенос через очередь с GC. object_accessor_assignment дополнен проверкой
восстановления owner после UNSUPPORTED setter. Фактическая сборка внутри
native callback не заявляется: barrier сохраняется.

Readonly callback правила, самозамена/удаление свойства внутри setter и последующая
data-запись, Array mutation set_data, версии и все стадии остаются открытыми.

## English

Inherited script setters execute with the receiver without updating prototype
descriptor data or creating own receiver slots. Own setters still store data,
including writable null-setter bridges. CAPI lookup scopes a private setter
owner with allocation-free roots for lookup owner, receiver and capture. Nested
calls/errors restore metadata; constructor/method identity stays unchanged.
Actual lookup ownership works with old descriptors without new wire metadata.
The temporary context field is not serialized; C89 ABI/wire structure is unchanged.

The new test covers nested own setter execution, receiver side effects,
inherited null setter, counted NUL keys, missing own slots, preserved prototype
data, own assignment, GC and queue transfer/GC. The error test verifies restored
owner after an unsupported setter. Native barrier limits remain explicit.
Readonly callback rules, self-replacement/deletion and subsequent data assignment,
Array mutation set_data, version behavior and every roadmap stage remain open.
