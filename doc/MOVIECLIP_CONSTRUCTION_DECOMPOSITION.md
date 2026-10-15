# Native construction decomposition / Декомпозиция создания клипов — v0.325

The editable drawing-copy and registered sprite-class selection functions now
live in separate implementation files, one function per file:

| File | Responsibility |
| --- | --- |
| src/standard/movieclip_copy_drawing.cpp | Clone editable path/active-path/gradient state together while retaining shared live bitmap descriptors |
| src/standard/movieclip_sprite_class.cpp | Choose the prototype/constructor from the current or transported class registry |
| src/standard/movieclip_construction.hpp | Declare the private shared construction contracts and caller root/barrier responsibilities |

Native attachment and duplication call the same class selector. Duplication
calls the same drawing copier. No adapter, copy of an algorithm or new public
ABI is added: functions are moved, renamed and declared in the private header.
A lexical token comparison confirms both bodies are unchanged apart from the
function names. Comments and formatting were preserved/refined for the new files.

The public C89 duplicate services remain the entry points for C callers and
future generated bodies. Construction helpers are internal C++98 implementation
details. Caller-owned roots and existing native collector barriers remain in
force, including exception/allocation paths. Full versioned MovieClip semantics,
constructor ordering, depth ranges, remaining construction decomposition and
machine-code JIT generation stay open.

Existing direct C/script duplication, transport, CloneSprite, collecting/throwing
coercion, gradient/bitmap and allocation checks validate the move. No new test
mirrors the implementation or changes an original oracle fixture.

## Русский

Копирование рисунка и выбор зарегистрированного класса вынесены из movieclip.cpp
в отдельные файлы, по одной функции. Общий приватный заголовок описывает
контракт и обязанности вызывающего кода по корням GC и нативному барьеру.
Attach и duplicate используют прежний общий выбор класса, duplicate — прежнее
копирование геометрии. Тела функций после переименования совпадают по токенам;
алгоритмы и публичный ABI не изменены.

Прямые C-вызовы и будущий JIT продолжают пользоваться C89-сервисами дублирования.
Внутренние помощники остаются деталями C++98. Проверяются прежние сценарии
графики, градиентов, bitmap, транспорта, исключений и отказов памяти. Полная
декомпозиция, совместимость MovieClip и все стадии роудмапа остаются открытыми.
