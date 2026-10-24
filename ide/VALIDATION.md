# IDE 0.1.19 / FIR 0.5.2 checkpoint

The source-to-SWF backend passes 25 unit tests with process doubles and 16 real CLI/compiler/linker/FlashVM scenarios across four numeric source profiles. A renamed detached copy passes the same 25 unit and 16 real scenarios using only explicit executable paths. The C89/C++98 headless FlashVM runner is an external acceptance tool, not an IDE dependency.

FIR passes all eight CTests natively and all eight with AddressSanitizer/UndefinedBehaviorSanitizer; the runs include object-name inspection and real traces for seven names plus their combined SWF. SWL passes three existing CTests. LeakSanitizer is disabled; executable-name lookup warnings may limit sanitizer symbolization. The numeric AS2 example prints 42 through the real pipeline. Generated logs and test reports stay outside Git.

The twenty-commit patch is applied with git am --3way to a clean copy of the published FlashVM workspace v0.1524, with both bhgv identities and the final tree checked. FlashVM itself stays at v0.1524; FIR becomes 0.5.2 and this independent IDE backend becomes 0.1.19. Graphical editing, complete source languages, ABC, LSP/incremental projects, source maps and debugger UI remain open. Windows and physical Android were not accepted in this series.

## RU

Backend проходит 25 unit-тестов с process doubles и 16 настоящих CLI/compiler/linker/FlashVM сценариев четырёх численных языковых профилей. Переименованная отдельная копия проходит те же 25 unit и 16 real сценариев, используя только явные пути executable. Headless FlashVM runner — внешний инструмент приёмки, а не зависимость IDE.

FIR проходит восемь CTests нативно и восемь с ASan/UBSan, включая object-name inspection, настоящий trace для семи имён и общего SWF. SWL проходит три прежних CTests. LeakSanitizer отключён; предупреждения поиска имени executable могут ограничивать символизацию. Численный пример AS2 выводит 42 через настоящий pipeline. Generated logs/reports остаются вне Git.

Патч из двадцати коммитов применяется через git am --3way к чистой опубликованной базе workspace v0.1524 со сверкой обеих подписей bhgv и итогового дерева. Сам FlashVM остаётся v0.1524; FIR получает 0.5.2, независимый IDE backend — 0.1.19. Графический редактор, полные языки, ABC, LSP/incremental projects, source maps и debugger UI остаются открытыми. Windows и физический Android в этой серии не приняты.

## 0.1.21 architecture and code-readability checkpoint / Контрольная точка

The Limbo/Dis/channel-only architecture, isolated runtime directory, mandatory JIT targets and debugger contracts are requirements. Dis import, embedding, native JIT execution and debugger integration have not been implemented or accepted by this patch. Existing host Python tools/tests are reformatted and commented: all 55 function/type definitions have preceding comments, and Python ASTs (including the handwritten process-double body) match the previous implementation. All 25 unit tests and 16 real source/compiler/linker/FlashVM integration scenarios pass again. Markdown target/source-line checks and clean two-commit patch application are checked separately. No fresh device or Dis runtime test is claimed.

Архитектура Limbo/Dis с UI только через каналы, отдельный runtime-каталог, обязательные JIT targets и отладчик — требования. Импорт/встраивание Dis, native JIT execution и debugger integration этим патчем не реализованы и не приняты. Прежние Python host tools/tests отформатированы и прокомментированы: у всех 55 функций/типов есть предшествующие комментарии; Python AST, включая тело process double, совпадает с прежним. Повторно проходят 25 unit и 16 настоящих source/compiler/linker/FlashVM integration сценариев. Markdown targets/source lines и чистое применение двух коммитов проверяются отдельно. Новые device/Dis runtime tests не заявляются.
