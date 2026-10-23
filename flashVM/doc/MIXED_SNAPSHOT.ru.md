# Согласованный снимок смешанного потока

[`fvm_swf_stream_output_mix_info`](../include/flashvm/swf_stream_output.h#L138) одним реальным queued-callback получает состояние
stream и ledger принятых BASE/TAIL. Раздельные запросы могут видеть разные моменты
расходования FIFO; единый результат сохраняет равенство общих и частных часов.

## Контракт

Передайте живой stream, его точный adapted mix queue и отдельный writable
[`FVM_SWFStreamMixInfo`](../include/flashvm/swf_stream_output.h#L26); достаточно задать внешний struct_size=sizeof. Размеры
вложенных структур заполняются при OK. Submitted-эпохи должны совпадать, origin
tracking должен быть включён. Конвертер, immutable snapshot, микшер, userdata
backend и retry PCM остаются живыми. Внешне сериализуйте обоих владельцев и
жизненный цикл очереди; busy guards не заменяют mutex между OS-потоками.

Source chunk, pending BASE, EOF и pause берутся у stream. Pending TAIL по-прежнему
запрашивается через mix_queue_pending и не становится stream pending PCM.
Суммы submitted/played/queued категорий равны общему clock. Seconds — общий dequeue,
делённый на output rate, включая TAIL; это не позиция источника и не DAC time.
BASE включает тишину и примешанные события, TAIL — выбранные event-only padding и
release. Отклонённый PCM не учитывается до подтверждённого принятия.

Выполняется один опрос native FIFO, обновляются ledger и cache stream, копируется
результат. Нет decode/render, аллокаций, продвижения источника, разрешения кадров,
pause/clear, VM/GC. Callback может блокироваться согласно backend-контракту.
Stream/mix busy guards отклоняют повторный вход во время callback.

Неверные указатели/внешний размер/reentry, чужой adapted backend, разные submitted
эпохи, регресс cached dequeue или backlog выше stream target дают INVALID_ARGUMENT.
Отключённые origins — UNSUPPORTED. Статусы backend передаются, исключения — ERROR;
невозможный рост очереди/регресс dequeue — ERROR. При ошибке out и stream cache
сохраняются. Валидное наблюдение mix может уже снять native origin-позиции перед
отказом из-за target: прогресс устройства не откатывается. Успешный seek/clear
обнуляет обе эпохи; failed clear сохраняет pending/epoch с уже подтверждённой pause.
Continue сохраняет общий clock и категории прежних сегментов.

## Desktop и пример

`fvm_desktop_audio_mix_info` держит mutex producer и учитывает EOF последовательности:
если впереди есть подготовленный header, sequence EOF ещё false. Mixed refill,
resume/завершение и публичная stream-диагностика используют единое наблюдение.
--stats получает категории из того же результата. Отдельный origin API сохранён.
Правила A/V-планирования этим не меняются.

C89 swf_stream_mix --origins показывает common17/0/17 до software drain и
17/17/0 после него; категории BASE9 и TAIL8. Независимый ten-mode checker сверяет
оба снимка и точный PCM. Это программный consumer, не физическое воспроизведение.

## Проверки и границы

Проверяются один queued-опрос, неизменность source/retry, disabled mode, ошибки,
исключения и reentry, чужие владельцы/эпохи, pending BASE, failed clear и reset.
Четыре независимых OS-worker проходят 256 finite-эпох с точным PCM; запрет аллокаций
охватывает новый запрос. White-box seed проверяет wrap и предел 2^53-1 без огромного
PCM и многолетнего запуска. Desktop worker использует настоящий SDL dummy callback,
Stop/Play, host pause, GC и interpreter/native JIT.

Source-sample mapping, MP3 delay, автоматические A/V drift/seek, дочерние таймлайны
и словари, реальные устройства и платформы остаются открытыми. Все девять стадий
плана открыты.

Внутренний desktop stream-запрос сохраняет legacy-путь для вручную подготовленных адаптеров без origin tracking. Проверка disabled mode не опрашивает FIFO; fallback выполняет один queued-опрос. Явный paired API по-прежнему возвращает UNSUPPORTED.

Desktop выбирает внутренний путь по immutable composition_origins, установленному при подготовке, а не по статусу callback. Поэтому backend UNSUPPORTED передаётся после одного FIFO-вызова и не вызывает повторный fallback-опрос. Ручной frontend-владелец выбирает режим после включения origins.

Единый mixed-снимок v0.1051–v0.1063 получает source/retry metadata, общий dequeue clock и категории BASE/TAIL одним FIFO-опросом. Desktop refill, resume/завершение и статистика используют paired-путь под mutex; заранее выбранный immutable-режим сохраняет legacy-адаптеры и передаёт статусы backend без повторного опроса. Итог: 215 выбранных native regression/tutorial/CLI проверок и две проверки options; 68 выбранных ASan/UBSan и четырнадцать --no-throw запусков; после финального mode/status исправления пересобраны и пройдены все 17 mixed frontend sanitizer проверок; отдельно 12 SDK-free core/tutorial проверок. Исключения и allocation failures проверяются обычной сборкой, LeakSanitizer отключён. Проверены один опрос draining FIFO, неизменность source/retry, pending BASE/TAIL, failed clear, reentry, сохранение out, seeded clock/ring границы, запрет аллокаций, 256 эпох независимых OS-workers и настоящий SDL dummy worker с Stop/Play/pause/GC/interpreter/JIT. Ten-mode C89-пример сохраняет точный PCM и проверяет paired-отчёты до/после drain. Source/DAC time, codec latency, автоматические A/V/drift/reference seek, дочерние словари/таймлайны, реальные устройства/платформы остаются открытыми. Все девять стадий открыты.


v0.1051: Добавлен единый снимок stream и BASE/TAIL без рендеринга и продвижения источника.

v0.1052: Проверены чужие владельцы, отключённый режим, ошибки и исключения callbacks, pending BASE, неудачный clear и сброс эпохи.

v0.1053: Четыре OS-потока проверяют PCM и один опрос на снимок в 256 эпохах; запрет аллокаций охватывает снимки при retry.

v0.1054: Добавлен desktop-запрос под mutex для смешанной статистики; сохранена семантика EOF последовательности.

v0.1055: Worker SDL dummy проверяет суммы clock при authored Stop, Play, pause и GC в интерпретаторе и JIT.

v0.1056: Mixed refill, resume, завершение и stream-диагностика используют единое наблюдение.

v0.1057: C89-пример выводит единые снимки до и после drain; независимый ten-mode PCM checker проверяет оба отчёта.

v0.1058: Seeded-проверки охватывают границу точности clock, wrap, регрессию, target mismatch и сброс.

v0.1059: Описаны один callback, публикация stream cache, неоткатываемый прогресс очереди и desktop-владение.


v0.1060: Внутренний stream-запрос совместим с адаптерами без origins; явный paired API сохраняет UNSUPPORTED.


v0.1061: Один PCM-эталон проверяет legacy и origin-enabled refill, точный retry, финальный drain и сохранение out при UNSUPPORTED.


v0.1062: Режим наблюдения публикуется до worker; backend UNSUPPORTED сохраняется после одного callback с проверкой out и числа опросов.


v0.1063: Зафиксированы native, sanitizer, SDK-free и C89 проверки единого снимка; source-time и A/V приёмка остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_output_mix_info` | function | [include/flashvm/swf_stream_output.h:138](../include/flashvm/swf_stream_output.h#L138) |
| `FVM_SWFStreamMixInfo` | struct | [include/flashvm/swf_stream_output.h:26](../include/flashvm/swf_stream_output.h#L26) |
<!-- source-links:end -->
