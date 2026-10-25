# Empty StoreRegister / StoreRegister с пустым стеком — v0.310

Loaded StoreRegister now initializes a missing operand as undefined and leaves
it on the active operand stack before storing through the existing register
policy. In-range global or private registers receive undefined; out-of-range
loaded stores remain ignored. Host-only empty stacks remain errors. Payload
validation precedes preparation and bank writes. The one-byte register selector
width is documented by [FVM_AVM1_STORE_REGISTER_BYTES](../src/avm1/encoding_constants.h#L135) in encoding_constants.h.

The shared C89 [fvm_exec_global_register](../include/flashvm/execution.h#L607) service is unchanged: callers supply
explicit values, with no interpreter or Action dependency. Register-bank sizes,
private/global selection, persistent string ownership and the public ABI are
unchanged. Undefined publication itself invokes no callbacks and allocates no
storage; decoding and operand-vector preparation can still fail allocation.

avm1_store_missing checks host/SWF4–8, global slots 0/1/3 and ignored 4/255,
malformed payloads preserving the old value, and Function2 private register 1
seeded by a parameter before the empty-stack store. The allocation fixture
covers valid/ignored global slots with every preparation failure and GC between
attempts. Root/execution chains must be restored. Existing global-register
allocation tests cover the C service's retained string publication.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs action_store_register pops then pushes the same value;
runtime.rs supplies undefined for an empty pop. Complete register/function
compatibility and all roadmap stages remain open.

StoreRegister в загруженных SWF4–8 теперь подставляет undefined при пустом стеке,
оставляет значение на стеке и записывает его в допустимый регистр. Проверены
глобальные и приватные банки Function2, игнорируемые номера, строгий host-режим,
отказы памяти и GC. Повреждённая длина инструкции проверяется до изменения
регистра. Размер поля вынесен в документированный макрос заглавными буквами.
Общая C-служба сохраняет явные параметры, пригодные для будущего JIT.

Validation: Linux AGG/Cairo each passed 599 tests; three AGG executable-permission
launch failures passed after restoring permissions. Three targeted heap/global
ASan+UBSan tests passed (no stack/leak coverage). All 26 accepted original SWFs
matched the pinned oracle in each profile. The Action-reference audit is 94/100.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AVM1_STORE_REGISTER_BYTES` | macro / define | [src/avm1/encoding_constants.h:135](../src/avm1/encoding_constants.h#L135) |
| `fvm_exec_global_register` | function | [include/flashvm/execution.h:607](../include/flashvm/execution.h#L607) |
<!-- source-links:end -->
