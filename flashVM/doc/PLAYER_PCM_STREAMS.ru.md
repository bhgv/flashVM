# Один корневой PCM-поток в SDL-плеере

С v0.758 реальный desktop player принимает один проверенный сегмент SoundStreamHead/Head2 формата PCM_NATIVE или PCM_LE без корневых StartSound. Источник преобразуется в 48000 Гц stereo S16 и передаётся настоящей SDL-очереди. Другие кодеки, несколько заголовков и смесь event/stream явно отклоняются при preflight до окна/устройства. --mute по-прежнему проверяет и пропускает root stream без аудиоустройства. Строгий импорт core не изменён: плеер отдельно готовит snapshot того же источника и явно отключает stream-контролы в модели сцены.

## Сборка и запуск

Нужна самостоятельная сборка FlashVM со статическим SDL2 SDK и графическим профилем из [руководства сборки](PLAYER_TEST_BUILDS.ru.md), с FVM_ENABLE_SDL_AUDIO и FVM_BUILD_DESKTOP_FRONTEND. Core без плеера по-прежнему не требует SDL. После сборки flashvm_player:

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

JIT используйте только при наличии backend. [Воспроизводимый пример](../examples/player/pcm_stream/README.ru.md) меняет цвет и частоту тона через секунду. Это исполняемый SWF с автономным Python-генератором, не компилятор. Структурированные fvm_desktop_run и fvm_desktop_run_memory используют тот же путь; CLI-only состояния и новых ABI-полей настроек нет. Source base и политика загрузки прежние. Replace/unload используют существующую строгую политику; child/sprite streams не поддержаны.

## Потоки и владение

Владелец импортирует/укореняет root и читает [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L83) в точках обслуживания. Выбранный, но ещё не выполненный кадр не разрешает звук. Только скаляры entered frame и play передаются под audio mutex; worker не читает context, clip и managed values. Он владеет независимым snapshot, converter cursor и retry buffer и делает не больше FVM_DESKTOP_AUDIO_PUMPS вызовов [fvm_swf_stream_output_pump_until](../include/flashvm/swf_stream_output.h#L64) за итерацию. Backend SDL копирует принятый PCM и не входит в VM/GC. Reserve учитывает реальный quantum драйвера. Короткий разрешённый блок/EOF запускает непустую очередь даже ниже обычного порога, чтобы ожидание reserve не блокировало старт.

Пауза хоста/сессии/background и stop клипа останавливают очередь, сохраняя данные; play возобновляет разрешённые данные. StopSounds очищает очередь и отменяет stream до обнаруженной навигации. Release ramp для stream пока нет. Pending/backward/skipped переход вызывает [fvm_swf_stream_output_seek_frame](../include/flashvm/swf_stream_output.h#L83) под тем же mutex: старые queued/pending bytes удаляются, phase=0 и выбирается первый блок не раньше выбранного кадра. Пропуск кадра удерживает более поздний блок, тишина не синтезируется. EOF источника отличается от опустевшей очереди.

Наблюдение не является полным журналом: same-frame goto, выбранный и выполненный целиком между двумя наблюдениями, в v0.758 может остаться невидимым. Drift/drop/stretch, несколько заголовков, stream+event mixing, MP3/ADPCM stream decoder, синхронизация видео и эталонная Flash-семантика seek остаются открытыми. Root unload отсоединяет доставку, дожидается worker и освобождает snapshot/queue/device до уничтожения context. GC выполняется только владельцем в штатных safe points.

## Внутренний C API хоста

[Объявления audio.h](../frontend/common/audio.h), [реализация audio.c](../frontend/common/audio.c). Это приватные frontend helpers, не новый стабильный ABI плагинов; сигнатуры публичных C-функций запуска плеера сохранены.

| Операция | Контракт |
| --- | --- |
| fvm_desktop_audio_validate | Owner-thread проверка количества, формата и event-политики. OK без stream, UNSUPPORTED для неподдержанных сочетаний, ошибки структуры распространяются. Нет native device или полученного владения. |
| fvm_desktop_audio_open_paused | Получает snapshot/converter/FIFO/device/mutex/worker и stop binding при наличии звука; pause только 0/1. После любой частичной ошибки вызывайте close. Snapshot независим от VM, но stop binding отсоединяется до context destroy. |
| fvm_desktop_audio_timeline | Владелец читает удерживаемый клип своего контекста и публикует скаляры под mutex. Ошибки query/seek/backend возвращаются; pause/clear имеют staged effects без rollback. Worker в VM не входит. |
| fvm_desktop_audio_stream_info | Копирует stream-only [FVM_SWFStreamOutputInfo](../include/flashvm/swf_stream_output.h#L11) под mutex. Ошибки сохраняют дескриптор; возможны ожидание lock/backend. Это FIFO dequeue position, не DAC/movie sync. |
| fvm_desktop_audio_pause / fvm_desktop_audio_tick_paused | Независимая пауза хоста; нет очистки данных. Tick проверяет worker error latch. Хост вызывает последовательно; worker сериализован внутри. |
| fvm_desktop_audio_clear / fvm_desktop_audio_close | Владелец отсоединяет binding/callback, join worker, закрывает snapshot и device, уничтожает mutex. Clear сохраняет счётчики. Context/userdata должны жить до detachment/quiescence. |

## Проверки и ограничения

stream_worker.cpp использует настоящий SDL dummy queue и укоренённый MovieClip: initial hold, gaps, медленный владелец, stop/play, GC, EOF, StopSounds, backward seek, host pause и joined cleanup, с выбором interpreter/JIT. stream_audio.py независимо строит FWS/CWS с AVM1 действиями и сравнивает каждый принятый PCM sample реального CLI trace с 9600 точными stereo frames. stream_preflight.py проверяет поддержанные/неподдержанные/повреждённые/muted сочетания с отсутствующими драйверами для отклонённых путей. Dummy не доказывает слышимый звук, другую ОС или CPU. FVM_AUDIO_TRACE явно включает bounded RAM capture и запись файлов при close: это принятые блоки, в том числе позже удалённые clear, не speaker playback. Все стадии roadmap остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:83](../include/flashvm/movieclip.h#L83) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:64](../include/flashvm/swf_stream_output.h#L64) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:83](../include/flashvm/swf_stream_output.h#L83) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:11](../include/flashvm/swf_stream_output.h#L11) |
<!-- source-links:end -->
