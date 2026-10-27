# Собственные источники инструментов и обновление

[English](UPSTREAMS.md) · [Первые шаги](../README-FIRSTSTEPS.md)

Корневая сборка больше не требует готового стороннего Coco/R или Limbo SDK.
В `tools/` находятся минимальные выборки из репозиториев владельца. Импортированные
`upstream/` содержат точные git-blobs; адаптеры CMake, тесты и наши README лежат
отдельно. Исходные NOTICE/лицензии сохранены и не заменяются общей MIT-лицензией.

| Каталог | Источник | Назначение |
| --- | --- | --- |
| [coco-r-C](coco-r-C/README.md) | https://github.com/bhgv/coco-r-C | C-генератор и используемые C-шаблоны |
| [coco-r-limbo](coco-r-limbo/README.md) | https://github.com/bhgv/coco-r-limbo | Генерация Limbo из C, исходник Limbo-генератора и runtime fragments |
| [limbo-bootstrap](limbo-bootstrap/README.md) | https://github.com/bhgv/Inferno-OS-bhgv, experiment | Временный native bootstrap; C- и Limbo-версии для разработки |
| [../disVM](../disVM/README.md) | тот же experiment | Ядро интерпретатора, JIT и необходимые regression fixtures |

Точные commit, branch, список выбора и SHA-256 каждого файла записаны в
`UPSTREAM.json` соответствующего каталога. C++-шаблоны Coco/R, готовые программы,
полная ОС, архивы примеров и посторонние каталоги документации не переносятся.
Draw->Context сохранён как интерфейс bootstrap-входа, без реализации GUI.

Проверка не обращается в сеть и ничего не меняет:

```sh
python3 tools/vendor-sync.py tools/coco-r-C
python3 tools/vendor-sync.py tools/coco-r-limbo
python3 tools/vendor-sync.py tools/limbo-bootstrap
python3 tools/vendor-sync.py disVM
python3 tools/test_vendor_sync.py
```

Обновление активного experiment из отдельного checkout:

```sh
git -C /absolute/Inferno-OS-bhgv pull --ff-only https://github.com/bhgv/Inferno-OS-bhgv.git experiment
python3 tools/vendor-sync.py disVM --update --source /absolute/Inferno-OS-bhgv --ref experiment
python3 tools/vendor-sync.py tools/limbo-bootstrap --update --source /absolute/Inferno-OS-bhgv --ref experiment
./build-workspace.sh --projects disvm,limbo-bootstrap --tests
```

Помощник сам не fetch/checkout/commit/push. Он читает точный commit через git show,
игнорируя незакоммиченные файлы checkout. Изменённые/пропавшие импортированные
файлы, symlink в пути и столкновения с незаписанными локальными файлами блокируют
обновление. Удаляться могут только прежние manifest-owned файлы. Новые blobs
сначала полностью подготавливаются, manifest публикуется последним. Это не
атомарная транзакция каталога; прерванный импорт обнаруживается проверкой.

После обновления просмотрите git diff и пересоберите затронутые части.
Новая upstream-зависимость требует явного обновления whitelist/CMake, без
автоматического затягивания всей ОС, графики или security-подсистем.

Найденные подтверждённые ошибки исправляются в исходном репозитории на локальной
ветке. Коммит с вашей подписью экспортируется отдельным format-patch; FlashVM
обновляется из этого точного commit. Когда меняются оба репозитория, выдаются
оба патча. Push остаётся ручным. В этой серии добавлены host/build-адаптеры;
подтверждённого исправления upstream-багов нет, импортированные blobs не менялись.

Native Limbo — временный bootstrap. Исторический yacc-парсер является частью
bootstrap-исходников, а не нашим новым компилятором; установка yacc не требуется.
Наши парсеры остаются Coco/R. Приоритет — собственный Limbo: семантика, debug info,
Dis/SWF backend и внешние C/AS/другие заготовки по `.m`.
[Контракт собственного компилятора](../compiler/limbo/OWN_COMPILER.ru.md).

[Inferno mk](mk/README.ru.md) · [Inferno iyacc](iyacc/README.ru.md): selected experiment f948343b711c8300dbf76417254c18a7a8d2f5ff, 82 / 17 upstream files, explicit dependencies and bootstrap scripts.

Текущие выборки Dis и bootstrap: `45370abd78dd8b9339c962a544d2a933663139f1`; проверяйте их manifest перед обновлением.
