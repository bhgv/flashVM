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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_set_identity_tracking` | function | [include/flashvm/swf_sound_output.h:61](../include/flashvm/swf_sound_output.h#L61) |
| `fvm_swf_sound_output_sound_active` | function | [include/flashvm/swf_sound_output.h:81](../include/flashvm/swf_sound_output.h#L81) |
| `fvm_swf_sound_output_storage_info` | function | [include/flashvm/swf_sound_output.h:74](../include/flashvm/swf_sound_output.h#L74) |
| `fvm_swf_sound_voice_output_frames` | function | [include/flashvm/swf_sound_mixer.h:44](../include/flashvm/swf_sound_mixer.h#L44) |
<!-- source-links:end -->
