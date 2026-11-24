# Собранный терминал DisVM и FlashVM

[English](README.md)

Два файла байткода проверяют одно обновление интерфейса через транспорт IDE.
Программа на Limbo передаёт собранный фильм в `.upload`, подключает два обычных
канала `chan of array of byte` через `$Unibus.attach` и отправляет кадр fvd1.
Запись `start` в `.state` запускает фильм. ActionScript принимает кадр через
`styxTerminal`, сдвигает панель на 64 пикселя вправо и возвращает те же
данные Unicode с суффиксом ` AS2`. Limbo сверяет ответ целиком и отключает
каналы. Обмен в обоих направлениях идёт по одной сессии libstyxrpc.

## Запуск

Запустите входящие в комплект `terminal.dis` и `terminal.swf` на штатном
`disvm` с поддержкой встречного монтирования и
`flashvm_styx_player` с SDK. В POSIX-среде скрипт на Python 3 создаёт пару
сокетов, передаёт дескрипторы двум VM и проверяет завершение. Прикладная
логика целиком содержится в `terminal.dis` и `terminal.swf`. Используется
штатный планировщик контроллера DisVM. Он передаёт данные плееру после
небуферизованной отправки, прежде чем продолжить программу и
запустить единственный кадр SWF.

```sh
python3 launch.py /path/to/disvm /path/to/flashvm_styx_player
python3 launch.py /path/to/disvm /path/to/flashvm_styx_player --jit --capture frame.ppm
```

По умолчанию обе VM работают в режиме интерпретатора. `--jit` включает JIT.
`--directory DIR` задаёт каталог с двумя собранными файлами. `--window`
открывает окно SDL2 на время обмена. `--capture` также открывает окно и
сохраняет проверенное изображение 320×240 по заданному пути PPM. Для обоих
режимов нужен плеер с графикой и SDL2. С `SDL_VIDEODRIVER=dummy` проверка
пикселей работает без физического дисплея. Успешный запуск печатает
`PASS compiled AS2 + Limbo` и полученный кадр.

## Сборка

Понадобятся [компилятор AS2](https://github.com/bhgv/flashVM/blob/master/compiler/as2/view/README.ru.md),
[инструменты FIR](https://github.com/bhgv/flashVM/blob/master/fir/v1/doc/PIPELINE.md),
[SWL](https://github.com/bhgv/flashVM/blob/master/linker/README.md),
[анализатор Limbo на Coco/R](https://github.com/bhgv/flashVM/blob/master/compiler/limbo/frontend/README.ru.md)
и `limbo-bootstrap` из сборки DisVM. Укажите пути к инструментам, каталог
с `fir1` и `fir1_opt`, каталог исходников DisVM с интерфейсами `Sys` и
`Unibus`, а также каталог готовых файлов:

```sh
sh build.sh /build/as2/view/as2_viewc /build/fir /build/linker/swl \
  /build/limbo/frontend/limbo_parse /build/disvm/bootstrap/limbo-bootstrap \
  /source/flashVM/disVM /output/terminal
python3 launch.py /path/to/disvm /path/to/flashvm_styx_player --directory /output/terminal
```

`terminal.as` проходит цепочку AS2 → верифицированный и оптимизированный FIR →
SWO → SWL → SWF. Линкер добавляет прямоугольник из `panel.svf`. Анализатор
Limbo проверяет `terminal.b`, а bootstrap-компилятор проекта проверяет типы и
выпускает `terminal.dis`. Для работы гостевым приложениям нужны два файла
байткода.
