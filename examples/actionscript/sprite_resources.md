# sprite_resources.as

Создаёт два независимых экземпляра экспортированного спрайта Hero и обращается к его таймлайну. Требует заранее загруженного SWF с ExportAssets и меткой second; показывает использование ресурсов документа, а не создание спрайта из пустого объекта.

## Запуск и особенности

Исходник: [sprite_resources.as](sprite_resources.as). Требуется внешний компилятор AS1/AS2 → SWF/AVM1.
FlashVM не компилирует этот текст самостоятельно. Расширения channel/thread/mutex/gc
требуют явной установки хостом; доступность операций определяется профилем.
Читайте комментарии к требованиям документа и именам ресурсов в самом исходнике.

## Демонстрируемый ActionScript API

- `attachMovie` — [SWF_SPRITE_RESOURCES.md](../../doc/SWF_SPRITE_RESOURCES.md).
- `gotoAndStop` — [SWF_SPRITE_RESOURCES.md](../../doc/SWF_SPRITE_RESOURCES.md).
- `play` — [SWF_SPRITE_RESOURCES.md](../../doc/SWF_SPRITE_RESOURCES.md).
- `duplicateMovieClip` — [SWF_SPRITE_RESOURCES.md](../../doc/SWF_SPRITE_RESOURCES.md).
- `getDepth` — [SWF_SPRITE_RESOURCES.md](../../doc/SWF_SPRITE_RESOURCES.md).
- `trace` — [SWF_SPRITE_RESOURCES.md](../../doc/SWF_SPRITE_RESOURCES.md).

[Описание в общем каталоге](../README.md#example-actionscript-sprite_resources-as).
