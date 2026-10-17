# FlashVM C89/C++98 API index

- `include/flashvm/gc.h`: collection, mode, pinning, introspection.
- `include/flashvm/value.h`: primitive and object values.
- `include/flashvm/table.h`: indexed elements and metatables.
- `include/flashvm/channel.h`: raw channel transport.
- `include/flashvm/transfer.h`: graph send/receive across queue/file/TCP channels.
- `include/flashvm/property.h`: property flags, deletion and enumerable-name snapshots.
- `include/flashvm/function.h`: first-class native/AVM1 functions, invoke/construct.
- `include/flashvm/avm1.h`: actions, rooted trace/URL sinks and borrowed active-target introspection.
- `include/flashvm/stage.h`: host RGBA background selection and query.
- `include/flashvm/movieclip.h`: host-authored clip frames and vector rendering.
- `include/flashvm/standard.h`: standard installation and primitive conversion.
- `include/flashvm/capability.h`: TCP resource authority and imports.

All public headers must remain valid C89 and usable from C++98. No STL types cross the C ABI.

Object graph coverage and limitations: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

`value.h`: ABI 3 counted UTF-8/WTF-8 and UTF-16 creation/query/copy helpers.

`fvm_movieclip_swf_document_info` — [SWF document metadata / метаданные SWF](SWF_DOCUMENT_METADATA.md), `include/flashvm/movieclip.h`.

## Runtime and context

Initialize `FVM_Config` with `fvm_config_init`, create the runtime with
`fvm_runtime_create`, then create its context with `fvm_context_create`.
Check each returned `FVM_Result` before using an output handle. Destroy the
context with `fvm_context_destroy` before `fvm_runtime_destroy`. Each context
owns its managed object graph; do not access one context concurrently from
multiple threads. Worker examples create independent contexts and exchange
values through the documented transport API.

Сначала инициализируйте конфигурацию, затем создайте runtime и context.
Проверяйте результаты до использования выходных handles; уничтожайте context
раньше runtime. Контекст владеет своим графом объектов и используется его
VM-потоком. Точные декларации: [flashvm.h](../include/flashvm/flashvm.h).

## Values and borrowed views

`FVM_Value` carries a type tag and its corresponding payload. Use the
`fvm_value_*` helpers instead of leaving cells uninitialized. A borrowed string
or object view does not transfer ownership. Keep backing bytes valid for the
operation that borrows them; use counted string APIs when embedded NUL matters.
Managed objects retained by the host across GC safe points need explicit roots;
see [GC API](GARBAGE_COLLECTION.md#api). Cross-context object transfer uses
serialization/resource interfaces, not copying `FVM_Object *` pointers.

Ячейка FVM_Value хранит тег и значение. Заимствованное представление не передаёт
владение; время жизни конкретного результата определяется вызванной функцией.
Для строк со встроенным NUL используйте API с длиной, для объектов через точки
GC — явные корни. Декларации и границы lifetime: [value.h](../include/flashvm/value.h).

## Native calls and metatables

`fvm_register_function` installs a named native callback; `fvm_call` invokes a
named function and `fvm_object_call` invokes a named method with a receiver.
Check the current declarations in [value.h](../include/flashvm/value.h), and
[AVM1 function contracts](AVM1_FUNCTIONS.md) for script invocation. Registration
userdata belongs to the host; a callback must respect the owning VM thread.

`fvm_table_set_metatable` attaches a table; `fvm_meta_register` registers a
metamethod. The explicit `fvm_meta_call` API invokes native __call with owner,
defining metatable and a fresh indexed argument table (user argument zero is at
index zero). This operation has its own callback convention; it does not imply
implicit AVM1 closure dispatch. See [meta_call.h](../include/flashvm/meta_call.h),
[metamethod.h](../include/flashvm/metamethod.h) and the
[annotated meta-call description](../examples/extending/meta_call/README.ru.md).

Именованные native-функции, методы с receiver и явный meta-call — разные
соглашения вызова. Выбирайте API по примеру и не переносите правила аргументов
одного соглашения на другое. Stack callbacks описаны отдельно в [STACK_API.md](STACK_API.md).
