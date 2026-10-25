# Embedded video SWF6 / Встроенное видео SWF6

`video-only.swf` содержит настоящий поток Sorenson Spark (`flv1`), 160×120,
12 кадров/с, две секунды. `video-audio.swf` добавляет синтетический тон 440 Hz:
mono MP3, 22050 Hz. Оба файла имеют заголовок FWS, версию **6** и входят в целевой
диапазон SWF ≤ 8. Контент создан фильтрами FFmpeg; скачанных роликов и чужой музыки нет.

Это **проверочные файлы пока не реализованной возможности FlashVM**. В текущем
загрузчике нет обработки DefineVideoStream/VideoFrame. Он может принять файл и
проигнорировать неизвестные теги; успешный exit code и пустой trace не доказывают
декодирование или отображение кадров. Не выдавайте чёрное/пустое окно за видеопроигрывание.

Для будущей приёмки нужны одновременно: разбор видеотегов, сопоставление потока
с display object, декодер Sorenson Spark, временные метки/очередь кадров и передача
RGBA в выбранный вывод. Наличие отдельного FFmpeg media bridge само по себе не
подключает этот путь к встроенному видеопотоку SWF.

Проверить содержимое независимым декодером можно так:

```sh
ffprobe -v error -show_streams video-audio.swf
ffmpeg -i video-only.swf -frames:v 1 first-frame.png
```

`ffprobe` должен показать `flv1`, 160×120; для второго файла также `mp3`, 22050 Hz,
один канал. Это проверка файла/внешнего декодера, не результата FlashVM.

Повторная генерация: `bash build.sh` рядом с этим README. Нужен FFmpeg с muxer
`swf`, encoder `flv` и, для звука, `libmp3lame`. Скрипт явно перезаписывает только
эти два файла. Они уже приложены, поэтому установщик проекта не требует FFmpeg.

English: real embedded-video SWF6 compatibility fixtures, not a claim of current
FlashVM video playback. The checked-in files require no encoder during normal
build/install. FFmpeg can independently probe/decode their synthetic content;
the missing SWF-video path remains a separate runtime implementation task.
