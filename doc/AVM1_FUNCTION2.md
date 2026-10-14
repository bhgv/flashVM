# AVM1 Function2 checkpoint — v0.20

## English

The interpreter now decodes `ActionDefineFunction2` records, including bodies
outside the action header, and retains per-parameter register assignments and
preload/suppression flags. Each invocation has private registers; nested try
blocks share the current invocation's registers. Parameters are copied first;
selected preloads overwrite their assigned register positions afterwards.
`this`, `arguments`, `_root`, `_parent` and `_global` preloads are supported.
Reserved flag bits, insufficient register capacity and out-of-range parameter
registers are rejected. v0.214 accepts simultaneous preload/suppress flags; see
[paired flags](AVM1_FUNCTION2_FLAGS.md). Super dispatch/preloads were added in
v0.23; see [super checkpoint](AVM1_SUPER.md).

Named formal parameters and their corresponding `arguments` indices share an
activation binding through native accessors. Duplicate names alias only the last
named occurrence. Unpassed arguments do not acquire aliases. Register-assigned
parameters have no named activation binding and are currently not aliased through
`arguments`. Deleting an indexed accessor breaks its binding through the existing
property deletion API.

[GC:] Call construction roots the activation, arguments, receiver, callee and
register storage. An accessor descriptor retains its activation through a traced
native capture. Escaped arguments remain usable after the call returns. The
activation/arguments/descriptor cycle becomes collectible when external roots are
removed. Private metadata arrays have no user-modifiable Array prototype chain.

Graph wire version 5 carries Function2 metadata and the stable mapped-argument
callback identifiers. Queue transfer between independent contexts, including a
receiver without the standard library installed, preserves those bindings. The
reader continues accepting wire versions 1–4; older FlashVM readers reject version
5 rather than silently interpreting unsupported callback metadata. Public ABI 3
is unchanged. Function2-specific file/TCP transfer between independent processes
is not yet an acceptance test in this checkpoint.

`tests/avm1_function2.cpp` covers bytecode framing, invalid flag combinations,
parameter/preload register collision, register 255, receiver/root/parent/global
preloads, suppression, parameter-to-arguments and arguments-to-parameter writes,
duplicate names, escaped activation roots, graph transfer and cycle reclamation.
The existing suites cover legacy function calls and graph/resource compatibility.

Linux verification: headless 168, AGG 178, Cairo 178, minimal configuration 159,
strict C89/C++98 with heap/global ASan and UBSan 165 checks. The strengthened cycle
case was rebuilt and rerun in every profile after its last test-only change.
Some managed-workspace binaries lost execute permissions; affected launches passed
when repeated after restoring permissions. Sanitizer constraints remain: no stack
ASan or LeakSanitizer, container allocation-failure tests use UBSan only, and SQLite
is excluded from this sanitizer profile.

Remaining work: full native superclass payloads, register-parameter alias policy,
exact inherited suppression/scope behavior across SWF versions, exhaustive
malformed-input and independent-process transfer coverage. Super preloads and
receiver/owner dispatch now follow the v0.23 checkpoint. This checkpoint does not complete Function2, AVM1 or roadmap stage 2.
The flag and preload layout follows Adobe's SWF file-format specification
([archived Adobe PDF](https://www.flashrealtime.com/content/dam/Adobe/en/devnet/swf/pdf/swf_file_format_spec_v10.pdf)).

## Русский

Добавлены разбор `ActionDefineFunction2`, приватные регистры каждого вызова,
регистровые параметры и preloads `this`, `arguments`, `_root`, `_parent`, `_global`.
Вложенные блоки try используют регистры текущего вызова. Параметры записываются
раньше preloads; пересечение регистров приводит к перезаписи параметра preload.
Проверяются зарезервированные биты, недостаточное число регистров и неверные
номера регистров параметров. С v0.214 совместные preload/suppress допустимы:
[правила парных флагов](AVM1_FUNCTION2_FLAGS.md). Super/preloads добавлены в
v0.23: [контрольная точка super](AVM1_SUPER.md).

Именованные параметры связаны с индексами `arguments` через аксессоры и общий
объект активации. Для повторяющихся имён связан только последний именованный
параметр. Отсутствующие аргументы не получают связей. Регистровые параметры пока
не связаны с индексами `arguments`. Удаление индексного аксессора разрывает связь.

[GC:] При подготовке вызова удерживаются активация, arguments, получатель,
вызываемая функция и регистры. Дескриптор аксессора удерживает активацию через
трассируемый захват. Возвращённый объект arguments остаётся рабочим после выхода
из функции и сборки мусора. После снятия последнего внешнего корня цикл
arguments/дескриптор/активация собирается. Массивы служебных метаданных отключены
от изменяемых пользовательских прототипов Array.

Формат графов повышен до версии 5: сохраняются метаданные Function2 и идентификаторы
аксессоров связанных аргументов. Проверена передача через очередь в другой контекст,
в том числе без установленной стандартной библиотеки. Чтение версий 1–4 сохранено;
старые версии FlashVM отвергают формат 5. Публичный ABI остаётся 3.
Специальные межпроцессные проверки Function2 через файл/TCP ещё предстоят.

Тесты и результаты перечислены выше. Дополненный тест сборки циклических arguments
повторно собран и выполнен во всех профилях; запуск бинарников с утраченными правами
доступа повторён после восстановления прав. Ограничения санитайзеров указаны явно.

Открыты нативные суперклассы, связанность регистровых параметров, точные правила
унаследованных suppress/областей видимости для версий SWF и исчерпывающие проверки
повреждённых данных и межпроцессной передачи. Super/preloads уже реализованы в
объёме v0.23. Этап 2 плана и Function2 целиком не закрыты.
