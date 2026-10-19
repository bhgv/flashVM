# Форматы SWO1 и SWA1

Все целые поля — unsigned little endian, без выравнивания структуры C.
SWO1 содержит AVM1, а не FIR и не сериализованные адреса памяти.

| Offset | SWO1 field |
| --- | --- |
| 0 | Four bytes `SWO1` |
| 4 | u32 version = 1 |
| 8 | u32 requirements: bit 0 FlashVM channels; other bits rejected |
| 12 | u32 module-name byte length, 1..255 |
| 16 | u32 AVM1 byte length, 1..2 MiB |
| 20 | u32 FNV-1a of name followed by AVM1 bytes |
| 24 | ASCII module name, without NUL, then AVM1 bytes |

Имя: `[A-Za-z_][A-Za-z_0-9.]*`. FNV-1a: начальное 2166136261,
для каждого байта `(hash XOR byte) * 16777619` по модулю 2^32; это проверка
случайного повреждения, не аутентификация. Последняя инструкция ActionEnd.
Проверяются границы записей, Push и адреса переходов; неизвестные opcodes
отвергаются. Это профиль команд текущего FIR backend, а не вся AVM1.

SWA1: четыре байта `SWA1`, u32 version=1, u32 count (1..1024), затем count
записей `[u32 размер SWO1][байты SWO1]`. Хвостовые байты запрещены.
Имена модулей заменяют таблицу символов инициализаторов первой версии;
релокации вызовов функций ещё не определены. Контейнер SWF8 содержит DoAction
на каждый объект, затем ShowFrame и End. Jump внутри объекта не меняется.
