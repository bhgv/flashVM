# IDE 0.1.19 / FIR 0.5.2 checkpoint

The source-to-SWF backend passes 25 unit tests with process doubles and 16 real CLI/compiler/linker/FlashVM scenarios across four numeric source profiles. A renamed detached copy passes the same 25 unit and 16 real scenarios using only explicit executable paths. The C89/C++98 headless FlashVM runner is an external acceptance tool, not an IDE dependency.

FIR passes all eight CTests natively and all eight with AddressSanitizer/UndefinedBehaviorSanitizer; the runs include object-name inspection and real traces for seven names plus their combined SWF. SWL passes three existing CTests. LeakSanitizer is disabled; executable-name lookup warnings may limit sanitizer symbolization. The numeric AS2 example prints 42 through the real pipeline. Generated logs and test reports stay outside Git.

The twenty-commit patch is applied with git am --3way to a clean copy of the published FlashVM workspace v0.1524, with both bhgv identities and the final tree checked. FlashVM itself stays at v0.1524; FIR becomes 0.5.2 and this independent IDE backend becomes 0.1.19. Graphical editing, complete source languages, ABC, LSP/incremental projects, source maps and debugger UI remain open. Windows and physical Android were not accepted in this series.

## RU

Backend проходит 25 unit-тестов с process doubles и 16 настоящих CLI/compiler/linker/FlashVM сценариев четырёх численных языковых профилей. Переименованная отдельная копия проходит те же 25 unit и 16 real сценариев, используя только явные пути executable. Headless FlashVM runner — внешний инструмент приёмки, а не зависимость IDE.

FIR проходит восемь CTests нативно и восемь с ASan/UBSan, включая object-name inspection, настоящий trace для семи имён и общего SWF. SWL проходит три прежних CTests. LeakSanitizer отключён; предупреждения поиска имени executable могут ограничивать символизацию. Численный пример AS2 выводит 42 через настоящий pipeline. Generated logs/reports остаются вне Git.

Патч из двадцати коммитов применяется через git am --3way к чистой опубликованной базе workspace v0.1524 со сверкой обеих подписей bhgv и итогового дерева. Сам FlashVM остаётся v0.1524; FIR получает 0.5.2, независимый IDE backend — 0.1.19. Графический редактор, полные языки, ABC, LSP/incremental projects, source maps и debugger UI остаются открытыми. Windows и физический Android в этой серии не приняты.
