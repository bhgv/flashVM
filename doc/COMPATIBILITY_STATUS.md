# Реестр выполнения / Compatibility status

Ориентир: [ROADMAP.md](ROADMAP.md). Базовый реестр уточняется по мере сравнения
с нормативными SWF/AS API; это ещё не полный каталог совместимости.
Статусы: отсутствует / реализовано / проверено / завершено. «Проверено» относится
к перечисленным сценариям, а не ко всем возможным входам или платформам.

| Пункт | Статус | Подтверждение и оставшаяся работа |
| --- | --- | --- |
| 1: значения String с NUL и суррогатами | Проверено для базовых сценариев v0.16 | standard_utf16.c, standard_string.c, string_wire.c; ABI 3, граф 4. См. UTF16_STRINGS.md. |
| 1: Unicode upper/lower и CASEINSENSITIVE | Проверено для выбранных сценариев v0.16 | Кириллица, расширения, финальная сигма; Unicode 3.2. Полная сверка с историческим Flash открыта. |
| 1: Object, Array и ключи | Реализовано частично | Watch/перечисление есть; generic Array, наследование индексов и registerClass проверены в сценариях v0.17 (GENERIC_ARRAYS.md). Полноценные строковые ключи, пределы длины и полная сверка открыты. |
| 2: AVM1 | Реализовано частично | Регистры Function2, основные preloads и aliasing именованных arguments проверены в v0.20 (AVM1_FUNCTION2.md). Extends/ImplementsOp/CastOp и транзитивные интерфейсы проверены в v0.22 (AVM1_INHERITANCE.md). Основные вызовы super и перенос фасадов проверены в v0.23 (AVM1_SUPER.md); числовые super-свойства, регистровые aliases и полная сверка областей/версий ещё открыты. |
| 3: MovieClip и SWF-сцена | Реализовано частично | В v0.24 проверен импорт корневых DoAction/FrameLabel из FWS/CWS по кадрам и перенос через очередь/файл (SWF_MOVIECLIP_TIMELINE.md). Полноценная сцена SWF и перечисленные в ROADMAP API открыты. |
| 4–9 | Открыты | См. ROADMAP.md и прежние контрольные точки; завершение не подтверждено. |

No roadmap stage is closed. Update this inventory with concrete tests, profiles
and limitations when implementing a remaining item. This initial inventory is
not yet a complete SWF tag/action/AS API compatibility catalogue.
