# Работа загруженных String-методов — v0.382

В загруженном String.split снят отказ на 65 537-м поле. Прежние правила AS limit
и особые delimiter-правила SWF5 сохранены; незагруженный host сохраняет отдельный
предел 65 536 полей с ошибкой памяти. Вместо ошибки результат не обрезается.
Обычные ограничения памяти и представления Array остаются.

Загруженные split/indexOf/lastIndexOf входят в общий scope учёта до преобразования
receiver и аргументов: вложенные callbacks используют тот же бюджет AVM1.
Split оплачивает позицию обхода, сравнение unit разделителя и каждый выдаваемый
UTF-16 unit. При кодировании supplementary-символа оплачиваются оба unit.
Поиск оплачивает кандидатную позицию и каждый сравниваемый unit, включая длинные
неполные префиксы. Исчерпание лимита или насыщение счётчика даёт FVM_ERROR;
нулевой лимит остаётся неограниченным. Проверка остановки worker использует
прежнюю периодическую политику общего scope. Host-учёт не меняется.

[GC:] Неопубликованный Array результата удерживается корнем при кодировании и
own stores. Ошибки/OOM освобождают частичную работу и восстанавливают корни и
глубину. Нативный барьер по-прежнему блокирует сборку из callbacks преобразования;
проверка бюджета не запускает GC. Закреплённый результат переживает настоящий
GC, после ошибки возможен повтор. Политика выхода публичных callbacks не меняется.

Алгоритм сопоставления остаётся наивным; гарантии линейного поиска нет.
Декодирование входа/разделителя, allocation helpers и другие String-методы не
получают новый поэлементный учёт. Полная Flash-сверка открыта. ABI, callbacks и
wire не меняются.

Проверки: string_loaded_work — host/SWF4–8, 65 537 полей, host-отказ, limits,
длинные префиксы в обоих направлениях, nested/unlimited/saturated бюджеты, GC и
оплата двух units supplementary-символа; string_loaded_work_allocation — все
выделения split и обоих поисков, GC/retry, корни/глубина;
string_loaded_work_callbacks — receiver→limit→delimiter, общий бюджет вложенного
поиска, исключения каждой фазы и прежний нативный барьер;
jit_string_work — успех, бюджетная ошибка и повтор в интерпретаторе, явном и
автоматическом JIT для SWF4–8.

Сопоставление: [String split закреплённого Ruffle](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/string.rs).
Это подтверждает модель AS limit/delimiter, но не является новым прогоном
оригинального Flash Player. Все девять стадий остаются открытыми.

Validation v0.382: all 796 AGG / 796 Cairo / 777 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (two AGG, one minimal; Cairo passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new work/allocation/callback/native cases
plus legacy split/search/receiver and character-allocation checks. Stack ASan
and LeakSanitizer remain disabled. The revised String constants header passes
a strict C89 syntax probe. No new original external SWF oracle or foreign-OS
run is claimed. All nine roadmap stages remain open.

## v0.386 копирование диапазонов

Загруженные slice/substring/substr разделяют бюджет с преобразованиями
получателя и аргументов; каждая выходная единица UTF-16 учитывается отдельно.
Пара суррогатов стоит две единицы, одиночные сохраняются в WTF-8. Вызов без
аргументов остаётся вне бюджета. Ошибка не публикует частичную строку.
Декодирование и индексы не получили поэлементного учёта; host-политика прежняя.
