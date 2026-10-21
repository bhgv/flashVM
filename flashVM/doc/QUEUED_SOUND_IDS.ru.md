# Идентичность звука в очереди

## v0.793: длительность готового голоса на выходной сетке

[`fvm_swf_sound_voice_output_frames`](../include/flashvm/swf_sound_mixer.h#L44) считает точное число оставшихся выходных
кадров для голоса, передаваемого с ТЕКУЩЕЙ позиции исходного PCM и нулевой фазой
конвертации. Учитывает оставшиеся повторы, округляет рациональную длительность
вверх и возвращает ноль на EOF. Не читает PCM, не выделяет память, не вызывает
callbacks, не обращается к VM и не меняет позицию. Выходная частота ограничена
диапазоном configured-микшера. Неверные аргументы дают INVALID_ARGUMENT без
изменения выхода; превышение SOUND_OUTPUT_MAX_CLOCK — UNSUPPORTED. Голос остаётся
у вызывающего и требует сериализации. Результат — целое double, а не секунды или
позиция DAC. Уже переданный микшеру голос имеет собственные lookahead/фазу и
не описывается этим расчётом новой выходной сетки.

`swf_sound_voice_output_frames` сравнивает каждую позицию трёх повторов при
четырёх исходных и восьми выходных частотах с реальным завершением микшера при
чтении по одному кадру, включая EOF и чтение после уничтожения VM. Это учёт
нативных отсчётов, а не полная приёмка таймлайна Flash или физических устройств.

## v0.794: необязательное фиксированное хранилище ID

[`fvm_swf_sound_output_set_identity_tracking`](../include/flashvm/swf_sound_output.h#L61) включает/выключает фиксированную
нативную таблицу OUTPUT_ID_SLOTS double-границ завершения (512 KiB на этом хосте).
Настройка разрешена без голосов, принятых или pending PCM. Включение выделяет
память транзакционно, выключение действительно освобождает таблицу. Ошибка
состояния/аргументов — INVALID_ARGUMENT, аллокации — OOM с прежней таблицей.
Backend/VM не вызываются; выход и микшер сериализуются вызывающим.

[`fvm_swf_sound_output_storage_info`](../include/flashvm/swf_sound_output.h#L74) копирует точные размеры объекта, retry PCM,
history, таблицы ID и проверенную сумму. Заимствованные микшер/устройство, VM и
накладные расходы аллокатора исключены. Неверные размер/null/reentry сохраняют
выход. [`fvm_swf_sound_output_sound_active`](../include/flashvm/swf_sound_output.h#L81) опрашивает эксклюзивную FIFO и сравнивает
границу ID с принятыми минус оставшимися в очереди кадрами. Выключенный учёт —
UNSUPPORTED, неверный ID/reentry — INVALID_ARGUMENT. Ошибки/исключения backend
сохраняют выход. Callback может блокировать по контракту backend; VM и новых
аллокаций нет. Используйте только tracked start/stop: прямое изменение микшера
не входит в контракт учёта.

`swf_sound_output_identity_storage` проверяет размеры, повторное включение,
реальное освобождение таблицы, ограничения состояния и ошибки запроса.

## v0.795: передача с SyncNoMultiple до конца очереди

[`fvm_swf_sound_output_submit_prepared_bounded`](../include/flashvm/swf_sound_output.h#L96) опрашивает FIFO, затем передаёт
голос с позиции submitted+pending. Точная оставшаяся длительность на выходной
сетке задаёт исключающую конечную границу ID. Обычные старты одного ID могут
перекрываться, сохраняется максимальный конец. NoMultiple потребляет ненужного
владельца, пока зарегистрированное время ID ещё не прошло, даже если микшер уже
освободил голос. Подавление не требует нового бюджета микшера. Пустые голоса не
продлевают границу. Принятый и pending PCM не перемешивается заново: новый голос
начинается со следующей границы композиции.

Сначала включите tracking; все start/read/stop заимствованного микшера должны
идти через выход. Бюджет микшера исключает выход/таблицу/устройство/шаблоны.
Нет аллокаций, decode и VM; callback запроса FIFO может блокировать. Владельцы
сериализованы и живы. Успех потребляет/обнуляет указатель; отказ сохраняет его
позицию, старые интервалы и голоса, но успешное наблюдение FIFO может обновить
скопированные счётчики. Переполнение часов/длительности и выключенный tracking —
UNSUPPORTED; ошибки микшера/backend передаются вызывающему.

`swf_sound_output_identity` воспроизводит случай retired-but-queued, проверяет
точную рациональную границу вместо конца целого блока, retained write failure
с точным retry, подавление без бюджета и сохранение владельца при отказе запроса
очереди после уничтожения VM.

## v0.796: владение при stop и clear

[`fvm_swf_sound_output_stop_id`](../include/flashvm/swf_sound_output.h#L104) применяет immediate/DECLICK-политику будущего
микширования и снимает регистрацию NoMultiple этого ID. Отсутствующий ID — успех,
неверный ID/null/reentry — INVALID_ARGUMENT, выключенный tracking — UNSUPPORTED.
Аллокаций, callbacks и VM нет. Принятый и pending PCM неизменяем: команда не
вынимает уже поставленные в очередь сэмплы этого ID. Новый NoMultiple разрешён
после команды, даже пока звучит прежний PCM/release-хвост. Сериализуйте выход/
микшер внешним mutex.

Общие stop и stop_release сбрасывают регистрации только ПОСЛЕ подтверждённого
backend clear. Ошибка pause/clear сохраняет ID, старые голоса и pending PCM;
успешная pause остаётся видимой. Успешный clear отменяет ID даже при последующем
отказе записи release. Такой retained release анонимен; новые голоса начинаются
после точного числа его pending кадров, без повторного микширования при retry.
`swf_sound_output_identity_stop` проверяет эти отказы, явный повторный старт,
отсутствующие/неверные ID и неизменность уже принятых PCM-байтов.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_set_identity_tracking` | function | [include/flashvm/swf_sound_output.h:61](../include/flashvm/swf_sound_output.h#L61) |
| `fvm_swf_sound_output_sound_active` | function | [include/flashvm/swf_sound_output.h:81](../include/flashvm/swf_sound_output.h#L81) |
| `fvm_swf_sound_output_stop_id` | function | [include/flashvm/swf_sound_output.h:104](../include/flashvm/swf_sound_output.h#L104) |
| `fvm_swf_sound_output_storage_info` | function | [include/flashvm/swf_sound_output.h:74](../include/flashvm/swf_sound_output.h#L74) |
| `fvm_swf_sound_output_submit_prepared_bounded` | function | [include/flashvm/swf_sound_output.h:96](../include/flashvm/swf_sound_output.h#L96) |
| `fvm_swf_sound_voice_output_frames` | function | [include/flashvm/swf_sound_mixer.h:44](../include/flashvm/swf_sound_mixer.h#L44) |
<!-- source-links:end -->
