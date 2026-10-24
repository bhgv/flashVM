# IDE на Limbo внутри FlashVM

Статус: принятое направление продукта; интеграция ещё не реализована. [English](ARCHITECTURE.md).

## Граница продукта

Приложение IDE, модель редактора/документов/проектов и логика сложных контролов пишутся на Limbo. Оно работает внутри приложения FlashVM через встроенную Dis. FlashVM предоставляет всю графику, MovieClip, текст, ввод и доступные платформенные возможности. Отдельного desktop Inferno нет; его графическое ядро Draw, Tk, crypto/Keyring и Prefab UI stack не используются.

Цель — надмножество SWF <= 8. Недостающие native-возможности можно добавлять во встроенные классы FlashVM с явными канальными операциями. Они объявляются расширениями; наличие расширения не означает полной совместимости с прежним SWF.

Существующий [Python-драйвер](build.py) остаётся host-инструментом разработки/проверок. Он не является целевой реализацией IDE и не должен становиться обязательной зависимостью развёрнутого приложения. Нынешние native C99 API компилятора/оптимизатора/линкера впоследствии можно предоставить как build services через каналы. Нового compiler service и приложения Limbo этот документ не реализует.

## Выбор выполнения

Начальный путь для полноценного языка — настоящий компилятор Limbo и байткод Dis; он не зависит от трансляции произвольного Limbo в AVM1. Нынешний [FIR source profile](../fir/v1/doc/SOURCE_SYNTAX.ru.md) — численный поднабор value(), которого недостаточно для процессов IDE, типизированных каналов и общего использования модулей/библиотек.

Dis встраивается как execution engine в host FlashVM, а не запускается отдельным emu-процессом. Первая приёмка использует интерпретатор; target-specific Dis JIT остаётся отдельной задачей от генерации AVM1 JIT. Нынешние MIPS, SPARC и PowerPC JIT в эту интеграцию не входят. Будущие x86-64, AArch64/arm64 и RISC-V64 порты должны сохранять семантику значений/layout Dis независимо от ширины native pointers. Сам порт FlashVM не доказывает готовность Dis; небольшой оставшийся runtime adapter тоже проверяется.

## База выделения Dis

Read-only изучена ветка Inferno experiment на [218b4adf](https://github.com/bhgv/Inferno-OS-bhgv/tree/218b4adfef1e238e705dd13ef0f9636e79abad7e). [Список сборки libinterp](https://github.com/bhgv/Inferno-OS-bhgv/blob/218b4adfef1e238e705dd13ef0f9636e79abad7e/libinterp/mkfile) включает draw/tk/crypt/keyring и генерацию связанных модулей; это ещё не готовая минимальная встраиваемая библиотека. [Типы интерпретатора](https://github.com/bhgv/Inferno-OS-bhgv/blob/218b4adfef1e238e705dd13ef0f9636e79abad7e/include/interp.h) описывают scheduler, каналы, heap и модули. Текущие upstream fixes и их покрытие нужно проверять для каждой выбранной ревизии импорта.

Выделяются loader, interpreter, scheduler, typed channels/alt, heap/GC, строки, exceptions и необходимые core modules за узким host adapter. Нужно выявить и убрать зависимости регистрации/линковки исключённых модулей и graphics-specific finalization hooks; успешные пустые заглушки графики/крипто не заменяют этого. Исходники переиспользуются с сохранением лицензий/notices. Импорт исходников, исполняемое подмножество Dis и boot acceptance здесь не заявляются.

## UI только через каналы

Все инициируемые IDE операции управления контролами проходят через типизированные request/reply channels, включая локальные вызовы. У Limbo нет MovieClip pointers и прямого UI mutator API. Native-обработчики сообщений могут вызывать C API FlashVM на владельце выполнения VM: это реализация канальной границы, а не альтернативный путь доступа приложения.

По возможности используются [resource dispatch](../flashVM/doc/RESOURCE_DISPATCH.ru.md), [namespace design](../flashVM/doc/RESOURCE_NAMESPACE.ru.md) и [event resources](../flashVM/doc/RESOURCE_EVENTS.ru.md). Нынешняя реализация явно не предоставляет общих GUI methods и полного транспорта Limbo. Недостающие каналы добавляются вместе с необходимыми возможностями контролов.

Предлагаемые, ещё не реализованные сервисы:

| Сервис | Ответственность |
| --- | --- |
| Scene/widget commands | Создание/удаление контролов и изменение поддержанных MovieClip/TextField properties |
| Replies и input events | Коррелированные completion/errors; focus, keyboard, pointer и editing events |
| Документы/проекты | Версионированные text edits, file/resource operations и project state |
| Build/debug | Запросы compile/verify/optimize/link/run, диагностика и наблюдение выполнения |

Сложный контрол собирается из примитивов FlashVM с Limbo-state и канальным обменом. Недостающий примитив расширяет соответствующий встроенный класс и его канальную схему, а не вводит ещё один графический toolkit. Первый полезный editor slice требует настоящего редактирования текста, selection/focus, scrolling и keyboard events; одной нарисованной текстовой панели недостаточно.

## Распределённость и владение

Сначала используется локальный транспорт, но сообщения не кодируют native struct layouts, pointers и heap references. Dis rendezvous и remote serialized transport не становятся взаимозаменяемыми автоматически: adapter определяет копируемые payloads, request IDs, typed schemas, replies, event ordering, поколения устаревших handles и bounded flow control. Локальный и будущие IPC/network транспорты используют одинаковую семантику операций; in-process UI shortcut не вводится.

Владелец FlashVM выполняет UI operations в безопасных точках. Заблокированный Limbo send/receive не блокирует владельца rendering/events. Dis scheduling и FlashVM pump получают явные бюджеты, cancellation и порядок shutdown. Accepted/queued отличается от executed; replies подтверждают завершение по контракту операции. Одни локальные каналы не обеспечивают network discovery, remote mount и exactly-once effects.

Dis и FlashVM сохраняют отдельные collectors/heaps. Между runtime используются owner-managed handles и копируемые сообщения; native pointers не сериализуются. Каждая сторона удерживает/освобождает собственные roots, subscriptions и pending messages до completion/cancellation. Автоматическая сборка циклов через обе кучи не предполагается.

## Отдельный runtime-каталог, JIT и отладчик

Планируемый корень импорта — `flashVM/external/dis/`: исходники Dis, необходимые runtime-зависимости, лицензии, описание сборки, JIT-backends и runtime-тесты изолируются там. На границе FlashVM остаётся узкая host/channel integration. Реализации heap, scheduler и инструкций Dis не смешиваются с ядром AVM1. Это планируемое размещение; данный патч ещё не импортирует runtime.

JIT — обязательный результат продукта, а не необязательная замена interpreter-only приёмке. Запись x66 пока трактуется как x86; ранее запрошенный x86-64 сохраняется явно. Требуемые targets: x86/i386, x86-64, AArch64/arm64 и RISC-V64. Подходящие backends experiment нужно изучить/переиспользовать, отдельно учитывая операции, interpreter fallback, instruction-cache synchronization, calling conventions, executable memory и настоящее target execution. Успешный fallback не доказывает выполнение native JIT code. Исключённые MIPS/SPARC/PowerPC backends не изменяются.

Отладчик обязателен и для интерпретатора, и для JIT. Канальные сервисы поддерживают attach/detach, stop/resume, breakpoints, step into/over/out, stack frames, locals/arguments, module/source positions, exceptions и выбранное состояние каналов/процессов. Нужно определить, какие процессы Limbo останавливаются, без заморозки display/input pump FlashVM. Debug information компилятора и соответствие source → logical Dis PC → native code сохраняются при JIT transitions, fallback и module reload. Native addresses не являются публичными transport handles. Breakpoints и pending debug requests требуют поколений модулей и явного retirement при reload.

Для каждого target приёмка доказывает настоящее выполнение generated code и эквивалентность интерпретатору, а также stops/steps, просмотр стека, exceptions, GC-safe points и shutdown с pending debug work. Это будущие требования приёмки, а не пройденные тесты данного документационного патча.

## Поставка

IDE Limbo и необходимые модули Dis поставляются с host/resources FlashVM как одно приложение. Поведение загрузки SWF <= 8 сохраняется. Загрузка Dis-модулей — явная опциональная runtime capability; байткод не должен маскироваться под AVM1 и не требует нового SWF tag до проектирования формата. Сборки без этой возможности честно возвращают её отсутствие.

[Шаги приёмки](DIS_ACCEPTANCE.ru.md) задают порядок реализации. Платформенные graphics/input ports переиспользуются из FlashVM; адаптеры Dis, упаковка и реальные платформы всё равно проверяются.
