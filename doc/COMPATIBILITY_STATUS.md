# Реестр выполнения / Compatibility status

Ориентир: [ROADMAP.md](ROADMAP.md). Базовый реестр уточняется по мере сравнения
с нормативными SWF/AS API; это ещё не полный каталог совместимости.
Статусы: отсутствует / реализовано / проверено / завершено. «Проверено» относится
к перечисленным сценариям, а не ко всем возможным входам или платформам.

| Пункт | Статус | Подтверждение и оставшаяся работа |
| --- | --- | --- |
| 1: значения String с NUL и суррогатами | Проверено для базовых сценариев v0.16 | standard_utf16.c, standard_string.c, string_wire.c; ABI 3, граф 4. См. UTF16_STRINGS.md. |
| 1: Unicode upper/lower и CASEINSENSITIVE | Проверено для выбранных сценариев v0.16 | Кириллица, расширения, финальная сигма; Unicode 3.2. Полная сверка с историческим Flash открыта. |
| 1: Object, Array и ключи | Реализовано частично | Watch/перечисление есть; строковые ключи, generic Array, наследование индексов, registerClass и полная сверка открыты. |
| 2: AVM1 | Реализовано частично | Function2, preloads, arguments aliasing, полный контроль/области/версии ещё открыты. |
| 3: MovieClip и SWF-сцена | Реализовано частично | Проверена модель хоста и её перенос; полноценная сцена SWF и перечисленные в ROADMAP API открыты. |
| 4–9 | Открыты | См. ROADMAP.md и прежние контрольные точки; завершение не подтверждено. |

No roadmap stage is closed. Update this inventory with concrete tests, profiles
and limitations when implementing a remaining item. This initial inventory is
not yet a complete SWF tag/action/AS API compatibility catalogue.
