# Отдельный патч библиотеки Clutter3D / Standalone library patch

[clutter3d-library-fixes-v0.1.patch](clutter3d-library-fixes-v0.1.patch) — **git am
патч именно для https://github.com/bhgv/Inferno-OS_Android-Clutter3d**.
Это не патч для FlashVM. Автор и коммитер: bhgv <bhgv@bhgv.bhgv>.
`v0.1` обозначает версию поставки патча, не новую версию upstream Clutter.

База: 2a616b264f0045a3ba00ce77aa1930ca905956ea. Изменяются только
sources/clutter-android/configure.ac и sources/clutter-android/clutter/clutter-actor.c.
Папка inferno-os не изменяется и не нужна для библиотечной SDK-сборки.
Её физически исключает [подготовка SDK](../../../external/clutter3d/README.md).

## Применение в репозитории библиотеки

Скачайте файл, откройте **корень Git библиотеки**, затем выполните:

```sh
git pull --ff-only https://github.com/bhgv/Inferno-OS_Android-Clutter3d.git master
git -c user.name=bhgv -c user.email=bhgv@bhgv.bhgv am --3way ~/storage/downloads/clutter3d-library-fixes-v0.1.patch
git log -1 --format='Author: %an <%ae>%nCommitter: %cn <%ce>'
git push https://github.com/bhgv/Inferno-OS_Android-Clutter3d.git master
```

Не применяйте его командой am в FlashVM. Патч FlashVM v0.602 помещает эту
копию и инструкцию в examples/frontends/clutter3d, но не публикует изменения
в Git библиотеки: для публикации выполните команды выше в её репозитории.

## Что исправлено

1. Настройка C++-тега libtool до LT_INIT на Linux и Android. Раньше Linux мог
   получить пустую команду архивации C++ и успешный выход без файла библиотеки.
2. Убран обязательный шаг генерации неиспользуемого Linux/Android Win32 config
   из отсутствующего входного шаблона. Это не реализация Windows-порта.
3. В обратной проекции actor сохраняются дробные входные координаты и размеры
   allocation. Убраны преждевременные приведения к int; публичный ABI прежний.

Это те же изменения, что использовала подготовка SDK FlashVM v0.600–0.601,
теперь в одном отдельно применяемом git am файле. Новый прямой патч проверен
применением к исходной базе с побайтовым совпадением результирующего Git-дерева.
Предыдущие Linux GLX/EGL/GLES2 и texture/input/3D проверки относятся к тем же
исправлениям; здесь не заявляется новый прогон нативной библиотеки.

## Сборка и ограничения

[Подробная подготовка и сборка SDK](../../../external/clutter3d/README.md)
содержит Linux GLX и EGL/GLES2, зависимости, отдельные source/prefix для разных
рендереров и требования Android. Фиксированный экспорт читает исходную базу и
сам применяет исправления; после am в исходный Git повторно вручную накладывать
сырой diff на уже исправленное дерево не нужно.

Linux был проверен в предыдущем блоке работ. Android требует NDK, Android ABI,
GLib-Android и реального Activity/ANativeWindow lifecycle; проверка на телефоне
остаётся открытой. Windows сейчас не подтверждён: отсутствующий Win32 config
шаблон и SDK/toolchain должны быть восстановлены и проверены отдельно.

English: this is a standalone git-format patch for the requested library Git,
not for FlashVM. It changes two library source files, never the inferno-os
client. Apply using git am in the library root; the FlashVM patch only carries
this copy and instructions. Linux/Android library build configuration and
subpixel inverse are repaired; native Android and Windows acceptance are not
claimed. The existing SDK guide documents actual Linux profiles and Android
prerequisites. No new standalone repository or GitHub publication is claimed.
