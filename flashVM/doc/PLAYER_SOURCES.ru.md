# Источники SWF и встраивание

[English](PLAYER_SOURCES.md).

## Вход из памяти с v0.673

fvm_desktop_run_memory из [player.h](../frontend/common/player.h) принимает
скалярные FVM_DesktopOptions, const void указатель, точное число байтов size_t,
непустое имя источника и необязательный указатель на статическую ошибку.
Подходят char buf[] и read-only массивы в исполняемом файле. NUL — обычный байт,
не конец данных. Нет временного файла, strlen(data), изменения источника или
передачи владения. Сохраняйте байты и имя доступными и неизменными до синхронного
возврата. Импорт VM владеет своей копией. FWS и CWS используют те же проверки,
сцену, корни GC, планировщик, рендеринг и очистку, что и файловый вход.

Поля плейлиста игнорируются, скалярные параметры проверяются полностью.
Null, нулевой/чрезмерный размер, отсутствующее имя и неверные параметры дают
код 2 до создания VM/окна. Повреждённый SWF даёт код 1 до открытия окна.
Успех/закрытие возвращает 0. Error получает null либо статический текст,
освобождать его нельзя. Прочая диагностика пока использует стандартные потоки.
Лимит входа [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15) — 64 MiB; лимиты распакованного SWF сохраняются.

Вызов выполняется на основном UI/VM-потоке, без параллельного или рекурсивного
воспроизведения. Это синхронный проигрыватель, не widget API. Имя — метка и
происхождение источника, а не открываемый путь. Финальный SDL_Quit принадлежит
приложению. [Владение](PLAYER_HOST_OWNERSHIP.ru.md).

[Пример встроенного фильма](../examples/player/embedded_movie/README.ru.md)
линкует неизменный SWF в программу и не требует файла ресурса при запуске.
Регрессии C89 проигрывают stack char и CWS в interpreter/JIT, проверяют ошибки,
неизменность байтов и используют несуществующие пути источников.

```sh
cmake --build build-player --target flashvm_example_embedded_movie flashvm_frontend_memory_movie
./build-player/frontend/flashvm_example_embedded_movie
ctest --test-dir build-player -R 'player_memory|example_embedded' --output-on-failure
```

[Нативная/кросс-сборка](DESKTOP_FRONTEND.ru.md) ·
[Параметры хоста](PLAYER_CONFIGURATION.ru.md) · [Реализация](../frontend/common/player.c) ·
[Этапы](ROADMAP_STATUS.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
<!-- source-links:end -->
