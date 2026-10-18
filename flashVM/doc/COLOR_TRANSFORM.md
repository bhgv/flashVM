# flash.geom.ColorTransform

ColorTransform хранит восемь числовых каналов в отдельном управляемом состоянии:
четыре множителя и четыре смещения RGBA. Публичные свойства преобразуют
присваиваемое значение через ToNumber. rgb задаёт три RGB-смещения и обнуляет
их множители, сохраняя alphaMultiplier/alphaOffset; чтение использует упаковку
с приведением к беззнаковому 32-битному числу, без обрезания отдельных смещений.

Конструктор без аргументов создаёт единичное преобразование. Один аргумент —
нативный ColorTransform — копирует каналы в независимое состояние. Набор из
восьми и более аргументов преобразует первые восемь через ToNumber; остальные
варианты арности дают единичное состояние. concat читает именно нативные
каналы второго объекта, поддерживает передачу самого себя и изменяет смещения
до множителей. toString читает публичные свойства. Эта модель проверена
в отдельных сценариях; полная сверка с Flash Player остаётся открытой.

Состояние является корнем из resource_result и передаётся вместе с графом
экземпляра, прототипов и классов. Десериализатор требует ровно восемь числовых
полей в приватной записи без прототипа, метатаблицы, методов и accessor-ов.
Подменённое имя поля отклоняется до восстановления. Чтение/запись каналов после
восстановления не требует установки стандартной библиотеки в получателе.

Проверки standard_color_transform.cpp: конструкторы, копия, concat и self-alias,
rgb и alpha, строковое ToNumber, toString, очередь/файл/loopback TCP, GC,
неверная приватная запись. color_transform_allocation.cpp перебирает отказы
выделения при копирующем конструкторе и десериализации; исходные каналы не
меняются. Общие проверки geometry_allocation.cpp покрывают установку класса.

Открыто: применение через Transform и MovieClip.transform, эталонные проверки
перечисления/удаления/перезаписи нативных accessor-ов, все необычные преобразования
и удалённые процессы. Номер версии и наличие методов не завершают этап 4.

## English

ColorTransform has eight native numeric channels stored in a traced private
record. Assignments use ToNumber. Packed rgb writes RGB offsets and zeroes their
multipliers while preserving alpha. Construction supports identity, independent
native copies and eight explicit numeric parameters. concat composes native
channels and supports self-aliasing; toString reads public properties.

Snapshots retain private state and class/prototype identity. Restoration validates
exactly eight numeric private fields with no executable or prototype metadata.
Tests cover copy/concat/rgb, string conversion, queue/file/loopback TCP, collection,
malformed private metadata and allocation failure during construction/restoration.
The receiving context needs no standard-library installation for native operations.

Transform/MovieClip integration, full accessor mutation/enumeration semantics,
exceptional coercion cases and remote-process coverage remain open. This is a
tested subset, not completion of roadmap stage 4 or full Flash equivalence.

## Проверки v0.35 / Validation

Linux: headless 192, AGG/Cairo по 202, portable 183. Три headless и два portable
теста не запустились из-за execute permissions; после восстановления прав
все эти тесты прошли. Strict C89/C++98 и ASan/UBSan: 189 тестов/примеров.
ASan stack и LeakSanitizer отключены из-за среды; heap/global ASan и UBSan
включены, container_exceptions проверен только UBSan. Аудит комментариев
не обнаружил пропусков для функций, членов заголовков, tutorial-строк и
проверяемых прямых GC-операций. Остальные ОС и полная совместимость не проверены.

All listed Linux cases passed, including permission-related retries. The strict
language/sanitizer run passed with the explicit limitations above; other target
platforms and complete Flash Player equivalence remain open. Logs are not committed.
