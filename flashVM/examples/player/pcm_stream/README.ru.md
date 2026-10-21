# Корневой PCM stream: цвет и звук

Готовый [pcm-stream.swf](pcm-stream.swf), автономный [Python3-генератор](make_movie.py), [полный контракт](../../../doc/PLAYER_PCM_STREAMS.ru.md). Двухсекундный SWF8: 320×200, 10 fps, один mono S16
LE/44100 stream, двадцать блоков по 4410 samples. Первая секунда: синий фон и 441 Гц;
вторая: оранжевый и 882 Гц. Все целочисленные вычисления генератора детерминированы;
библиотеки/компиляторы не нужны. Файл воспроизводится, но реальное устройство в этой среде
не проверено.

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
stream+event mixing, несколько заголовков и сжатые stream codecs остаются открытыми.
