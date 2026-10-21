# Короткое затухание при остановке плеера — v0.587

[English](PLAYER_AUDIO_RELEASE.md).
В WAV телефона v0.586 остановка по ресурсу даёт скачок примерно с 6026 в ноль.
Обычные stereo/ADPCM хвосты заканчиваются около 10 и 3. Поэтому плеер явно включает
короткое затухание остановки, не фильтруя весь PCM. Это ограниченная политика host,
не обещание sample-exact Flash stop или подтверждённого отсутствия щелчков на
динамике. Исходные немедленные API сохраняются.

## Голоса stop-by-resource

[`FVM_SWF_SOUND_MIXER_DECLICK`](../include/flashvm/swf_sound_mixer.h#L23) — значение 2 (0x2) в `FVM_SWFSoundMixerConfig.flags`, совместимый
с [`FVM_SWF_SOUND_MIXER_CONVERT`](../include/flashvm/swf_sound_mixer.h#L21). Ноль/только CONVERT сохраняют старую остановку.
Неизвестные биты дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) при create; размеры публичных структур
config/geometry не изменились.

С DECLICK успешный stop-by-ID отмечает совпавшие голоса на floor(output_rate/200)
выходных frames — около пяти миллисекунд, минимум два. Источник продолжает играть
с линейным усилением от единицы до точного нуля, затем слот освобождается. Конверсия
и прямое чтение используют один output-clock envelope, в том числе при разделении
чтения на буферы. Повторный stop не запускает затухание заново. Естественные концы,
loops, исходные snapshots и остальные голоса не меняются. Нет выделений памяти,
callback, managed references и устройства. Исчерпавшийся источник может завершиться
раньше: искусственное продолжение не обещается.

Затухающие голоса входят в active_voices и capacity. Они не подавляют следующий
SyncNoMultiple start, но заполненная таблица всё равно даёт [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44)
без вытеснения голоса. [`fvm_swf_sound_mixer_stop_all`](../include/flashvm/swf_sound_mixer.h#L131) остаётся немедленным даже с
DECLICK. Без CONVERT прежняя проверка strict-геометрии/частоты сохраняется. Нужна
сериализация host; submit выполняется в потоке владельца source context. Плеер
выбирает оба флага; приложения включают новую политику явно.

## fvm_swf_sound_output_set_release

`fvm_swf_sound_output_set_release(output, frames)` включает необязательную native
историю принятых PCM до первой отправки текущей эпохи output. frames=0 отключает
её и освобождает кольцо; ненулевое значение должно быть 2..pump block. Null/busy
output, эпоха с отправленными frames или неверный размер дают
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Backend callbacks не вызываются, микшер/устройство не
меняются. Caller сохраняет исключительное владение producer.

Успех [`FVM_OK`](../include/flashvm/flashvm.h#L42) копирует размер и создаёт кольцо (target+block)*channels short.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) сохраняет прежнюю настройку/историю, неожиданные исключения
превращаются в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Замена транзакционная; память ограничена существующими
лимитами block/queue/channels. В историю попадают только успешные записи: retry
не дублирует samples. При frames=0 прежние затраты памяти/копирования сохраняются.
Плеер задаёт 240 frames при 48000 Гц до старта worker.

## fvm_swf_sound_output_stop_release

`fvm_swf_sound_output_stop_release(output)` требует успешной ненулевой настройки
release. Null/busy/неподготовленный output дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Все backend
callbacks защищены обычным reentry guard. Нет выделений памяти и VM-указателей;
вызовы сериализуются с операциями микшера/output. Source VM уже может быть уничтожена:
native snapshots/история независимы. Нельзя закрывать микшер/устройство параллельно.

Функция наблюдает глубину программной очереди и выбирает последний принятый sample
перед её началом. Затем очищает backend, останавливает все software voices и
сбрасывает эпоху программных часов. Если output играет и известна ненулевая граница,
подаётся линейный хвост от этого уровня до точного нуля. Backend не pause/resume:
не вставляется отдельный paused-блок тишины. При paused состояние сохраняется и хвост
не синтезируется. Если потреблённого sample ещё нет или граница нулевая, хвост тоже
не нужен. Успех — [`FVM_OK`](../include/flashvm/flashvm.h#L42). Исходные голоса исчезают сразу; остаться может лишь
ограниченный PCM release.

Ошибка observe/clear сохраняет pending PCM и исходные голоса, возвращает статус
callback; исключения становятся [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44)/[`FVM_ERROR`](../include/flashvm/flashvm.h#L43). После успешного
clear старый звук уже удалён, даже если последующая запись хвоста не удалась.
Точный хвост сохраняется pending для pump retry, счётчики продвигаются только
после принятия. Некоторые backend, в том числе SDL с фиксацией ошибки записи,
требуют исходного hard stop/reset для восстановления; хвост тогда удаляется.
[`fvm_swf_sound_output_stop`](../include/flashvm/swf_sound_output.h#L105) по-прежнему немедленно pause/clear/reset без синтеза
хвоста и остаётся операцией восстановления/финальной очистки.

## Ограничения границы и аппаратуры

Граница берётся из software dequeue, а не позиции ЦАП. Уже переданные устройству
буферы переписать нельзя. Consumer может продвинуться между observe, clear и write:
в общем backend ABI нет атомарной замены очереди. Аппаратные буферы/конверсия и
гонки ограничивают физическую непрерывность; нужно новое прослушивание телефона.
Короткий release добавляет не более заданной PCM-длительности после уже переданных
буферов. Это явная presentation policy, не новая гарантия AVM1/audio scheduling.

Диагностический WAV объединяет принятые блоки, включая позднее удалённые. Поэтому
release после clear может выглядеть скачком относительно предыдущего WAV-блока,
который на самом деле удалён из воспроизведения. Нужны clear/write markers и
выбранная входная граница: это не аппаратная запись. Формат capture, запас очереди
и исходные тестовые тоны не менялись.

## Проверки и учебники

Дополненный [C89-пример очереди](../examples/multimedia/swf_audio_queue/README.md)
показывает настройку, release stop и прежний hard stop на явной модели тишины.
Это пример управления/владения, не слышимого fade. Настоящий
[плеер](../examples/player/animation_audio/README.md) применяет политику пяти
миллисекунд к прежним SWF в обоих режимах. Регрессии проверяют точные endpoints
прямого/converted чтения, разбиение на буферы, no-multiple, прежнюю мгновенную
остановку, wrap кольца, ошибки clear/write, reentry, OOM и отсутствие выделений
при release. AudioTrack и ключ подписи v0.585 сохранены. Приёмка новых окончаний
на телефоне ещё ожидается.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:21](../include/flashvm/swf_sound_mixer.h#L21) |
| `FVM_SWF_SOUND_MIXER_DECLICK` | macro / define | [include/flashvm/swf_sound_mixer.h:23](../include/flashvm/swf_sound_mixer.h#L23) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:131](../include/flashvm/swf_sound_mixer.h#L131) |
| `fvm_swf_sound_output_set_release` | function | [include/flashvm/swf_sound_output.h:107](../include/flashvm/swf_sound_output.h#L107) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:105](../include/flashvm/swf_sound_output.h#L105) |
| `fvm_swf_sound_output_stop_release` | function | [include/flashvm/swf_sound_output.h:111](../include/flashvm/swf_sound_output.h#L111) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:28](../include/flashvm/swf_sound_mixer.h#L28) |
<!-- source-links:end -->
