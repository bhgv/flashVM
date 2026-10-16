# Function class calls and construction / Вызовы и создание Function

For loaded SWF 4–8, `Function()` returns a fresh bare object without a prototype.
With arguments it returns the first value unchanged, including undefined, null,
booleans, numbers, counted strings and objects. It does not compile source text.
`new Function()` allocates an ordinary receiver using the class's own stored
prototype and hidden constructor identities; with arguments, the first value
replaces that receiver. Argument projection does not call coercion hooks.

AVM1 NewObject and NewMethod use an internal value-returning construction
boundary. Loaded native constructors may therefore return primitives. Script
constructors retain their existing rule of ignoring returned values. The public
`fvm_function_construct` and named C construction APIs still require an object
owned by the current context and reject primitive results. Unloaded host behavior
is preserved. Existing serialized callback identities are unchanged; ID 809
identifies the distinct Function call callback.

The internal boundary retains the function and a copied argument vector, blocks
GC during native execution, validates ownership and primitive metadata, copies
foreign counted string bytes before releasing the barrier, propagates exception
payloads, and publishes output only on success. Allocation failure leaves an
undefined output and restores root frames and native depth.

В SWF 4–8 `Function()` без аргументов создаёт обычный объект без прототипа,
а с аргументами возвращает первый без преобразования. Исходный текст не
компилируется. `new Function()` выделяет обычный объект с сохранённым собственным
прототипом класса и скрытыми идентификаторами конструктора; первый аргумент,
если он передан, заменяет результат.

NewObject/NewMethod сохраняют примитивные результаты нативных конструкторов.
Возвраты скриптовых конструкторов по-прежнему игнорируются. Публичные функции
создания C API требуют объект своего контекста. Поведение host без загруженного
SWF сохранено. ID 809 добавлен для обычного вызова Function; прежние ID не менялись.
Внутренний путь защищает конструктор и копию аргументов, проверяет результаты,
копирует строки до снятия GC-барьера и восстанавливает состояние при отказах.

Validation: `function_class_calls`, `native_constructor_values`,
`construction_value_allocation`; see [release evidence](RELEASE_ACCEPTANCE.md).
This change does not certify complete Flash compatibility or close a roadmap stage.
