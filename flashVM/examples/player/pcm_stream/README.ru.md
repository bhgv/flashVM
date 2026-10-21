# Корневой PCM stream: цвет и звук

Готовый [pcm-stream.swf](pcm-stream.swf), автономный [Python3-генератор](make_movie.py), [полный контракт](../../../doc/PLAYER_PCM_STREAMS.ru.md). Двухсекундный SWF8: 320×200, 10 fps, один mono S16
LE/44100 stream, двадцать блоков по 4410 samples. Первая секунда: синий фон и 441 Гц;
вторая: оранжевый и 882 Гц. Все целочисленные вычисления генератора детерминированы;
библиотеки/компиляторы не нужны. Файл воспроизводится, но реальное устройство в этой среде
не проверено.

Дополнительный [pcm-segments.swf](pcm-segments.swf) через секунду меняет источник:
44100 Hz mono → 22050 Hz stereo, с промежуточным пустым header в том же кадре.
После смены слева 882 Hz, справа 441 Hz. Одна SDL queue остаётся 48000 Hz stereo,
а loop заново подготавливает все headers. Генерация:
`python3 flashVM/examples/player/pcm_stream/make_movie.py --segments`.
Запускайте те же команды ниже с pcm-segments.swf вместо pcm-stream.swf. Это тот
же автономный script; без опции сохраняются прежние single-header bytes.
Каждый sample обоих фильмов проверяется независимым целочисленным oracle.
[Контракт последовательности](../../../doc/PLAYER_PCM_STREAMS.ru.md).

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
cmake --build build/player --target flashvm_player
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

Сначала соберите standalone player со статическим SDL2 SDK, FVM_BUILD_DESKTOP_FRONTEND и
FVM_ENABLE_SDL_AUDIO по [инструкции](../../../doc/PLAYER_TEST_BUILDS.ru.md); JIT требует доступного backend. --mute показывает только цвет
и не открывает audio device. Без --once плеер повторяет таймлайн до закрытия. Скорость
--fps меняет movie authorization, но не native sample rate: это не time stretch. Source
snapshot принадлежит хосту до остановки worker; root удерживается через GC, управление
остаётся в owner thread. При закрытии worker join предшествует освобождению
snapshot/device/context. Нет callbacks на стороне генератора и C API-обёрток, которые
нужно связывать отдельно. C89 IDE-хост может запускать те же байты через
fvm_desktop_run_memory; см. [встроенный фильм](../embedded_movie/README.ru.md).

Для воспроизводимой проверки очереди: SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy с теми
же аргументами. Это не слышимое устройство. FVM_AUDIO_TRACE=/absolute/path/evidence явно
включает WAV/текстовую запись принятых PCM блоков при закрытии. Не выдавайте trace за
запись динамиков. У worker есть frame gate, pause и bounded work; точный A/V sync,
stream+event mixing, сжатые stream codecs остаются открытыми.

## Потоковый ADPCM из целых пакетов (v0.764)

[adpcm-stream.swf](adpcm-stream.swf) — самостоятельный SWF8 320×200, восемь кадров
при 5 fps. В каждом кадре полный четырёхбитный моно ADPCM-пакет: 4096 сэмплов
22050 Hz, негромкая треугольная волна около 441 Hz; фон синий, затем оранжевый.
Длина таймлайна 1,6 с и 32768 исходных сэмплов намеренно различаются: автоматическая
компенсация drift и вставка тишины не заявлены. Плеер преобразует в 48000 Hz stereo.

```sh
python3 flashVM/examples/player/pcm_stream/make_adpcm.py --ffmpeg ffmpeg
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/adpcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/adpcm-stream.swf
python3 flashVM/frontend/tests/stream_adpcm.py build/player/frontend/flashvm_player --ffmpeg
```

Отдельный [генератор](make_adpcm.py) требует Python3 и FFmpeg с encoder `adpcm_swf`
и FLV muxer. Временные raw/FLV-файлы удаляются и при ошибке; указанный SWF заменяется
только после проверки полученных пакетов. Результат воспроизводится выбранной
версией encoder; между версиями FFmpeg байты могут отличаться. Прежний генератор
PCM не меняется и не требует FFmpeg. **Для воспроизведения FFmpeg не нужен**:
владелец VM заранее декодирует C++98-функцией; worker использует независимый PCM.
Освобождение снимков/SDL — после join. Короткие ADPCM-пакеты, смешивание с корневыми
StartSound и прочие кодеки пока не поддержаны: [контракт](../../../doc/STREAM_ADPCM.ru.md).

Независимая проверка декодирует все пакеты FFmpeg, вычисляет рациональную конверсию
целыми числами и сравнивает 71332 принятых стереокадра. Это dummy-очередь SDL,
не запись динамиков и не приёмка физического устройства. При сильном голодании
host конечный запуск может завершиться до производства последних PCM-данных.
Для прямого C89-встраивания передайте те же байты в `fvm_desktop_run_memory`, как
в [готовом примере](../embedded_movie/README.md). Контекст принадлежит владельцу,
не worker; время жизни снимка завершается только после остановки producer.
