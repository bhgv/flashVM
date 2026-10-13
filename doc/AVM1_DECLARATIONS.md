# AVM1 declarations / Объявления переменных

v0.94 поддерживает DefineLocal (0x3c) и DeclareLocal (0x41) на верхнем
уровне: адресат — текущий MovieClip, если receiver является клипом, иначе
глобальный объект. Внутри функции сохраняется локальная карта активации.
Имя преобразуется в строку с учётом версии и сохраняет counted UTF-16 ключ.
DefineLocal использует существующий setter-путь. DeclareLocal не стирает
существующее локальное значение/параметр; на верхнем уровне не создаёт
свойство поверх существующего в цепочке прототипов и не вызывает getter.

DefineLocal/DeclareLocal now work without a function-local map. Top-level
clip/global bindings use the existing property bridge; initialized declarations
observe inherited setters. Uninitialized declarations preserve an existing
local/formal and top-level inherited property. Names are coerced and canonicalized
while both name and value remain operand-stack roots. Setter/coercion errors
propagate before operand consumption. Prototype walks have the existing 1024
work bound and report cycles rather than silently creating a new binding.

Regression avm1_declarations covers full NUL names, numeric names, global/clip
isolation, own undefined cells, inherited getter/setter behavior, preserved formal
parameters, coercion failure and operand underflow. Full target-path syntax,
With scope and historical case-insensitive naming remain open.
Primary cross-check: [pinned Ruffle declarations](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
