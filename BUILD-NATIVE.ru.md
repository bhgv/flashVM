# Сборка через CMake 3.10, Meson, Make и Inferno mk

[English](BUILD-NATIVE.md) · [Первые шаги](README-FIRSTSTEPS.md) ·
[mk](tools/mk/README.ru.md) · [iyacc](tools/iyacc/README.ru.md)

Полная сборка workspace работает с **CMake/CTest 3.10.2**: проверено настоящими
официальными Linux AMD64 бинарниками. Обновление CMake не требуется. Конфигурация
запускается из каталога сборки с позиционным путём исходников, сборка — через
-- -jN, CTest — из рабочего каталога с проверкой -N на пустой набор тестов.
Минимумы собственных компонентов снижены до 3.10; FindPythonInterp используется
только на версиях без FindPython3. Объектные файлы FIR/SBL подключаются явно;
допуск продуктов IDE использует ограниченное чтение вместо file(SIZE).
Современный CMake также поддерживается.

```sh
./build-workspace.sh
python3 build-menu.py
```

По умолчанию собираются mk/iyacc из выбранных исходников владельца и восемь продуктов
workspace. Можно выбрать отдельные компоненты. Если нужен Limbo-bootstrap,
инструменты mk/iyacc собираются автоматически, парсер генерируется заново.
Сборки не скачивают исходники, бинарники или SDK. Python 3 нужен тестам/меню,
но не исполняемой IDE. Для тестового запуска IDE всё ещё нужна явно выбранная
совместимая VM: архив ядра DisVM не является полноценным runner.

## Прямая нативная сборка

```sh
./build-workspace.sh --backend mk --projects compilers,linker,iyacc
./build-workspace.sh --backend make --projects compilers,linker
make compilers linker
# Либо сначала собрать только mk и запустить настоящий mkfile:
sh tools/mk/bootstrap.sh build/mk
build/mk/mk -f mkfile compilers linker
```

Рецепты компилируют напрямую через CC/ar без делегирования CMake. Поддерживаются
mk, iyacc, limbo-bootstrap, `cocor`, as1/as2/as3, limbo (parse/tree/interface/bindings), compilers и
линкер SWO/SWA/SWF. Собственные парсеры генерирует локальный Coco/R, временный
Limbo bootstrap использует локальный iyacc. Нативные --tests доступны, если вся
выборка состоит из mk, iyacc и limbo-bootstrap. VM/FIR/SBL/IDE/DisVM и --demo
пока не поддержаны нативными движками; для этих сборок используйте CMake 3.10.
Meson 0.54+ и Ninja поддерживают ту же нативную выборку через настоящие run targets без вызова CMake. Нативные рецепты всегда пересобирают продукты;
инкрементальная сборка остаётся в CMake. По умолчанию CC=cc, AR=ar,
BUILD_ROOT=build/native; корневой скрипт использует build/workspace.

## Проверки и ограничения

Настоящие CMake/CTest 3.10.2: компиляторы 40/40, FIR 14/14, линкер 3/3, символы 3/3,
DisVM 15/15, проверки Coco/R и продуктов Limbo-bootstrap. Собраны библиотека FlashVM
и эталонный SWF executable. Производственная IDE собрана заново сгенерированным
bootstrap; Limbo-тесты с явным эталонным SDK прошли 25/25. Настоящий mk собрал четыре
фронтенда и линкер в путях с пробелами; прошли C/Python заготовки, разбор AS-адаптеров
тремя парсерами и 199 проверок линкера. GNU make собрал нативный линкер. Smoke-тест mk
проверяет шаблонные правила, timestamp и отказ дочернего процесса. iyacc сгенерировал
настоящий limbo.y с идентичным заголовком токенов.

Неизменённый хостовый Python-набор IDE сообщил две ошибки очистки при timeout
(подслучаи link/run), соответствующие ранее записанному ограничению этого workspace.
Они не превращаются в успех и не приписываются CMake; --tests возвращает ошибку.
Полная регрессия FlashVM и физические Android/ARM/RISC-V заново не проверялись.
Предупреждения и прежняя работа временных файлов mk/iyacc сохраняются. Исправления
подтверждённых ошибок Dis/JIT эта сборочная серия не содержит. Импортированы только
выбранные инструменты и необходимые зависимости.

Серия заменяет предложенный патч 0.1530 с запретом старого CMake. База —
опубликованный 0.1529. Применяйте новый патч на эту базу; старый 0.1530
не требуется как предварительный шаг.

## Engine selection / Выбор движка

```sh
python3 build-menu.py --list-backends
python3 build-menu.py --projects compilers,linker --backend meson
python3 build-menu.py --projects all --backend cmake
python3 build-menu.py --projects compilers,linker --backend make
python3 build-menu.py --projects compilers,linker --backend mk
python3 build-menu.py --projects all --backend auto
```

Пути движков задаются CMAKE/MESON/MAKE, Ninja — NINJA. Интерактивное меню
показывает наличие инструментов и предлагает cmake, meson, make, mk либо auto.
mk может быть собран из локальных исходников. Auto сначала выбирает CMake 3.10+;
для поддерживаемой нативной выборки затем проверяет Meson/Ninja, Make и локальный
mk. Неподдерживаемый полный набор отвергается без пропуска компонентов.
Явный выбор никогда не заменяется другим движком. --dry-run показывает команду
без требования установить выбранный движок. Meson 1.5.2/Ninja 1.11.1 в этой серии
действительно собрали четыре фронтенда и линкер. Auto проверен с настоящим CMake
3.10.2 для тестов линкера и с Make при отсутствующих CMake/Meson.

[Сборка mk/iyacc/Limbo из исходников и компиляция IDE без emu](README-FIRSTSTEPS.md)
