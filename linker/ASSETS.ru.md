# Ресурсы SWL

Ресурсы добавляются через C API или консоль. Линкер выделяет CharacterId,
пишет определения до кода и экспортирует имена через ExportAssets. Для JPEG
создаётся DefineBitsJPEG2 и отдельный DefineShape3 с bitmap fill; экспортируется
готовая к размещению фигура. Чисто ресурсный SWF допустим без `.swo`.

```sh
build/linker/swl link scene.swf \
  --svg panel linker/examples/panel.svg \
  --jpeg tile linker/examples/tile.jpg \
  --wav tone linker/examples/tone.wav \
  --place panel 20 30 1 --place tile 150 30 2 --play tone
```

| Input | C API | SWF tags |
| --- | --- | --- |
| Прямоугольник SVG | [`swl_add_svg`](SVG.ru.md), `swl_add_rect` | DefineShape3 |
| Baseline JPEG | `swl_add_jpeg` | DefineBitsJPEG2 + DefineShape3 |
| PCM WAV | `swl_add_wav` | DefineSound |
| Display placement | `swl_place(name,x,y,depth)` | PlaceObject2 |
| Sound start | `swl_play(name)` | StartSound |

В примере: зелёная панель 120×80, синяя картинка 8×8 и короткий тон 440 Hz.
Входы скопированы контекстом; неудачное добавление ресурса не меняет контекст.
Каждый callback вывода вызывается только после полной сборки и проверки.
Имена ресурсов уникальны в своём пространстве, имена модулей — в своём.
Размещение требует уже добавленного ресурса и уникальной глубины 1..65535;
x/y — неотрицательные пиксели 0..3276. Теги размещения/старта звучания идут
в порядке событий до DoAction в единственном кадре.

[Прямоугольник SVG](SVG.ru.md) можно передать обычным файлом `.svg`. Линкер
принимает одну закрашенную фигуру, заполняющую область просмотра, и выпускает
нативную фигуру SWF. [Пример](examples/panel.svg).

JPEG: baseline SOF0, 8 бит, 1/3 компоненты, SOI/SOS/EOI, размеры до 3276;
проверяются заголовки и границы сегментов, entropy-код не декодируется линкером.
Progressive JPEG, PNG/GIF и ошибки самого entropy-кода не поддержаны/не
диагностируются здесь. WAV: RIFF/WAVE PCM, mono/stereo, 8/16 бит little endian,
5512/11025/22050/44100 Hz; проверяются chunks, block alignment и число frames.
Сжатые WAV/MP3 отсутствуют. JPEG/WAV ограничены 16 MiB каждый; суммарный лимит
контекста 64 MiB, максимум 1024 ресурса и 1024 события. Библиотеки `.swa` первой
версии хранят кодовые `.swo`; ресурсы передаются отдельно.

Тест независимо читает RECT, shape edges, bitmap bytes, PCM samples, exports,
placement и StartSound; опциональный FlashVM runner загружает весь SWF. Это
проверка загрузки/структуры, а не доказательство отрисовки на реальном дисплее
или воспроизведения на аудиоустройстве.
