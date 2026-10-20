# Пример встраивания в IDE

[main.c](main.c) связывает компиляторы, оптимайзер и линкер в одном C-процессе.
Компилирует AS2 и Limbo с разными именами модулей; получает снимки FIR до/после
оптимизации; создаёт `.swo`, упаковывает их в `.swa`, добавляет фигуру и пишет SWF.
При заданном FRONTENDS_ROOT в тот же процесс включены четыре широких синтаксических
фронтенда. Дочерние процессы и команды оболочки в коде примера не используются.

```sh
cmake -S linker/examples/ide -B build/ide \
  -DFIR_ROOT="$PWD/fir/v1" -DSWL_ROOT="$PWD/linker" \
  -DFRONTENDS_ROOT="$PWD/compiler" \
  -DCOCOR_ROOT="$COCO_DIR" -DCOCOR_EXECUTABLE="$COCO_DIR/sources/cocor"
cmake --build build/ide --target ide_pipeline
build/ide/ide_pipeline build/ide/movie.swf
```

SWF выводит 42, затем 36; зелёная фигура размещается в (20,30), depth=1.
Это пример управления, не готовая IDE. Возвращаемый указатель на модуль snapshot
заимствован на время callback. Подкомпоненты задаются явными путями и могут
быть отделены/переименованы; нужен внешний Coco/R и корневая MIT LICENSE.
