#!/usr/bin/env python3
"""Interactive native workspace build selector; delegates builds to Bash.

Uses only Python 3's standard library. External SDK paths are inherited from
Default dependencies are source-built from tools/. Explicit SDK overrides use
COCOR_ROOT/COCOR_EXECUTABLE, LIMBO_EXECUTABLE, EMU_EXECUTABLE and
LIMBO_MODULE_DIR, as documented for build-workspace.sh.
"""

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


# Stable display order also determines the component numbers in the menu.
COMPONENTS = (
    ("vm", "FlashVM: окно, графика, ввод, звук, JIT и консольный runner"),
    ("compilers", "Синтаксические фронтенды AS1, AS2, AS3, Limbo"),
    ("fir", "FIR: компилятор, проверка и оптимизатор"),
    ("linker", "SWL: линкер SWO/SWA в SWF"),
    ("symbols", "SBL: читатель отладочных символов Limbo"),
    ("ide", "IDE: библиотечные Limbo-модули и .sbl (окно: disvm-editor)"),
    ("as1", "Только синтаксический фронтенд AS1"),
    ("as2", "Только синтаксический фронтенд AS2"),
    ("as3", "Только синтаксический фронтенд AS3"),
    ("limbo", "Собственный Coco/R Limbo: синтаксис, .m и внешние заготовки"),
    ("disvm", "Dis interpreter/JIT core (готовые приложения: disvm-runtime/disvm-editor)"),
    ("cocor", "Coco/R C: локальная сборка из tools/coco-r-C"),
    ("cocor-limbo", "Coco/R Limbo: C generator и cocor.dis"),
    ("limbo-bootstrap", "Временный native bootstrap из вашего experiment"),
    ("mk", "Inferno mk: нативная сборка из выбранных исходников"),
    ("iyacc", "Inferno iyacc: генератор парсера limbo.y"),
    ("disvm-host", "Общие с FlashVM файлы, сеть, каналы и нативные потоки Dis"),
    ("disvm-runtime", "Dis: исполняемая VM и терминальный редактор IDE (Linux AMD64, эксперимент)"),
    ("disvm-editor", "IDE MovieClip: несколько окон, каретка/выделение, Limbo/Dis + FlashVM/SDL2, сессионный clipboard"),
)


NATIVE_COMPONENTS = frozenset(
    (
        "mk",
        "iyacc",
        "limbo-bootstrap",
        "cocor",
        "as1",
        "as2",
        "as3",
        "limbo",
        "compilers",
        "linker",
    )
)
NATIVE_TEST_COMPONENTS = frozenset(("mk", "iyacc", "limbo-bootstrap"))
NATIVE_BACKENDS = ("meson", "make", "mk")


def validate_backend(backend, projects, tests=False, demo=None):
    """Reject unsupported native requests before creating any build products."""
    if backend not in NATIVE_BACKENDS:
        return
    unsupported = [name for name in projects if name not in NATIVE_COMPONENTS]
    problems = []
    if unsupported:
        problems.append("не поддерживает компоненты: " + ", ".join(unsupported))
    if demo:
        problems.append("не поддерживает --demo")
    if tests and not set(projects).issubset(NATIVE_TEST_COMPONENTS):
        problems.append(
            "--tests поддерживает только mk,iyacc,limbo-bootstrap; --tests/--demo для workspace требуют CMake"
        )
    if problems:
        raise ValueError(
            backend
            + ": "
            + "; ".join(problems)
            + ". Для полного выбора используйте cmake или auto. "
            "Для нативной сборки выберите compilers,linker,mk,iyacc,limbo-bootstrap."
        )


def choose_compatible_backend(projects, backend=None):
    """Keep the full component selection unless the user explicitly replaces it."""
    while True:
        backend = backend or choose_backend()
        try:
            validate_backend(backend, projects)
            return projects, backend
        except ValueError as error:
            print(error)
        value = input("1 — изменить двигатель, 2 — выбрать компоненты заново [1]: ").strip()
        if value == "2":
            print(
                "Для {} доступны: {}".format(
                    backend, ", ".join(name for name, _ in COMPONENTS if name in NATIVE_COMPONENTS)
                )
            )
            projects = choose_components()
            if projects is None:
                return None, backend
        else:
            backend = None


# Validate the complete selection before spawning a process or writing files.
def selection(value):
    """Resolve comma/space-separated numbers or names; preserve unique order."""
    names = [name for name, _ in COMPONENTS]
    tokens = value.replace(",", " ").split()
    if tokens in (["all"], ["все"], ["*"]):
        return names[:6] + ["disvm", "disvm-host", "disvm-runtime", "disvm-editor", "mk", "iyacc"]
    if not tokens:
        raise ValueError("Выберите хотя бы один компонент")
    result = []
    for token in tokens:
        if (
            token.isdigit()
            and all("0" <= ch <= "9" for ch in token)
            and 1 <= int(token) <= len(names)
        ):
            token = names[int(token) - 1]
        if token not in names:
            raise ValueError("Неизвестный компонент: " + token)
        if token not in result:
            result.append(token)
    return result


# Report local tools without installing them; source bootstrap is a distinct capability.
def backend_status():
    """Describe executable discovery and the native subset for each engine."""
    entries = []
    for name, variable, executable in (
        ("cmake", "CMAKE", "cmake"),
        ("meson", "MESON", "meson"),
        ("make", "MAKE", "make"),
    ):
        path = shutil.which(os.environ.get(variable, executable))
        detail = path if path else "не найден"
        if name == "meson":
            ninja = shutil.which(os.environ.get("NINJA", "ninja"))
            detail += "; Ninja: " + (ninja if ninja else "не найден")
        entries.append((name, detail))
    bootstrap = ROOT / "tools/mk/bootstrap.sh"
    entries.append(
        (
            "mk",
            (
                "исходники есть, будет собран локально"
                if bootstrap.is_file()
                else "исходники отсутствуют"
            ),
        )
    )
    entries.append(
        ("auto", "CMake 3.10+, затем Meson/Ninja, Make, локальный mk; с учётом выбора компонентов")
    )
    return entries


# Print discovery separately from actual version/selection validation in the launcher.
def show_backends():
    """Display what exists on this machine; no probe is build acceptance."""
    print("Сборочные двигатели:")
    for number, (name, detail) in enumerate(backend_status(), 1):
        print("  {}. {:7s} {}".format(number, name, detail))
    print("CMake: весь workspace. Meson/Make/mk: фронтенды, линкер, mk, iyacc, limbo-bootstrap.")
    print(
        "Нативные --tests: mk, iyacc, limbo-bootstrap. CMake disvm-editor собирает IDE со встроенной DisVM."
    )


# Give explicit engine choice while retaining auto as the empty-input default.
def choose_backend():
    """Accept one engine name or its displayed number, never evaluate user input."""
    show_backends()
    names = [name for name, _ in backend_status()]
    while True:
        value = input("Двигатель [auto]: ").strip().lower()
        if not value:
            return "auto"
        if value in names:
            return value
        if value in ("1", "2", "3", "4", "5"):
            return names[int(value) - 1]
        print("Выберите cmake, meson, make, mk или auto.")


# Ask until a valid selection is supplied; EOF and Ctrl-C are handled by main.
def choose_components():
    """Show the component menu and return one or several selected names."""
    print("\nСборка FlashVM workspace\n")
    for number, (name, description) in enumerate(COMPONENTS, 1):
        print("  {}. {:10s} {}".format(number, name, description))
    print("\nВведите номера/имена через пробел или запятую; all — всё, q — выход.")
    while True:
        value = input("Собирать: ").strip()
        if value.lower() in ("q", "quit", "выход"):
            return None
        try:
            return selection(value)
        except ValueError as error:
            print(error)


# Keep optional interactive choices explicit; empty input accepts the default.
def ask_yes(prompt):
    """Return a yes/no answer, rejecting accidental non-answer text."""
    while True:
        value = input(prompt + " [y/N]: ").strip().lower()
        if value in ("y", "yes", "д", "да"):
            return True
        if value in ("", "n", "no", "н", "нет"):
            return False
        print("Введите y/да или n/нет.")


# Select a runnable Limbo client after the caller opts into a console example.
def choose_example():
    """Choose navigation, history or replaceall; empty input preserves the original default."""
    while True:
        value = input("Пример: 1 — navigation, 2 — history, 3 — replaceall [1]: ").strip().lower()
        if value in ("", "1", "navigation"):
            return "navigation"
        if value in ("2", "history"):
            return "history"
        if value in ("3", "replaceall"):
            return "replaceall"
        print("Введите 1/navigation, 2/history или 3/replaceall.")


# Select a visible player configuration; Enter builds the real AGG/JIT desktop host.
# args receives explicit profile/player settings. Returns nothing; invalid input retries.
def choose_presentation(args):
    print("\nFlashVM: что собрать?")
    print("  1. Окно, графика, мышь/клавиатура, звук + JIT (по умолчанию)")
    print("  2. Окно, графика, ввод и звук: интерпретатор")
    print("  3. Обе оконные сборки: интерпретатор и JIT")
    print("  4. Только консоль, без графического SDK")
    while True:
        value = input("Вариант FlashVM [1]: ").strip()
        if value in ("", "1", "2", "3", "4"):
            args.vm_profiles = {
                "": "player",
                "1": "player",
                "2": "agg",
                "3": "agg,player",
                "4": "minimal",
            }[value]
            args.no_player = value == "4"
            args.player = not args.no_player
            if args.player:
                print("SDL2 SDK при необходимости соберётся автоматически; примеры будут в out/.")
            return
        print("Введите 1, 2, 3 или 4.")


# Edit all remaining build settings without requiring command-line switches.
# args is the parsed settings object. Returns after the user accepts the settings.
def choose_settings(args):
    if not ask_yes("Изменить дополнительные настройки сборки?"):
        return
    while True:
        print(
            "\nНастройки: 1 — параллельные задания ({}), 2 — конфигурация ({})".format(
                args.jobs, args.config
            )
        )
        print(
            "  3 — каталог сборки ({}), 4 — каталог установки ({})".format(
                args.build_root or "build/workspace", args.out or "out"
            )
        )
        print(
            "  5 — установка результатов ({}), 6 — профили FlashVM ({})".format(
                "выключена" if args.no_install else "включена", args.vm_profiles or "авто"
            )
        )
        print(
            "  7 — только показать команду ({}), Enter — начать сборку".format(
                "да" if args.dry_run else "нет"
            )
        )
        value = input("Настройка: ").strip()
        if not value:
            return
        if value == "1":
            count = input("Число параллельных заданий [{}]: ".format(args.jobs)).strip()
            if count.isdigit() and int(count) > 0:
                args.jobs = int(count)
            elif count:
                print("Нужно положительное целое число.")
        elif value == "2":
            config = input("Release, Debug, RelWithDebInfo [{}]: ".format(args.config)).strip()
            if config in ("Release", "Debug", "RelWithDebInfo", "MinSizeRel"):
                args.config = config
            elif config:
                print("Неизвестная конфигурация.")
        elif value in ("3", "4"):
            path = input("Каталог (Enter — оставить текущий): ").strip()
            if path:
                if value == "3":
                    args.build_root = path
                else:
                    args.out = path
        elif value == "5":
            args.no_install = not ask_yes(
                "Устанавливать бинарные файлы, примеры и команды запуска?"
            )
        elif value == "6":
            profiles = input("Профили через запятую: default,minimal,agg,cairo,player: ").strip()
            if profiles and all(
                item in ("default", "minimal", "agg", "cairo", "player")
                for item in profiles.split(",")
            ):
                args.vm_profiles = profiles
            elif profiles:
                print("Неизвестный профиль.")
        elif value == "7":
            args.dry_run = ask_yes("Только показать команду, не собирать?")
        else:
            print("Введите номер настройки или Enter.")


# Discover installed scripts without treating the generic checker as an application
# dependency. prefix is an explicit path; return stable menu IDs and copied metadata.
# The fallback launcher belongs only to the separately labelled legacy editor.
def product_entries(prefix):
    rows = (
        ("4", "console", "Программа Dis", (prefix / "disvm-runtime/run-console.sh",), False),
        (
            "5",
            "terminal",
            "Терминальный редактор Dis",
            (prefix / "disvm-runtime/run-editor.sh",),
            False,
        ),
        ("6", "services", "Сервисы IDE", (prefix / "disvm-runtime/run-ide.sh",), False),
        ("7", "modules", "Библиотеки Inferno", (prefix / "disvm-runtime/run-modules.sh",), False),
        (
            "8",
            "desktop",
            "IDE MovieClip: общий стол, Draw/SVG, Debug-панели, подсветка Limbo/AS, F7/F8",
            (prefix / "disvm-editor/run-desktop-ide.sh",),
            True,
        ),
        (
            "9",
            "editor",
            "Прежний одиночный графический редактор",
            (prefix / "disvm-editor/run-ide.sh", prefix / "disvm-editor/run-graphical-editor.sh"),
            True,
        ),
        (
            "10",
            "lifecycle",
            "Пример независимых владельцев и закрытия окон",
            (
                prefix / "disvm-runtime/run-desktop-lifecycle.sh",
                prefix / "disvm-editor/run-desktop-lifecycle.sh",
            ),
            False,
        ),
        (
            "11",
            "pointer",
            "Пример Unicode-выделения и проверки revision",
            (
                prefix / "disvm-runtime/run-desktop-pointer.sh",
                prefix / "disvm-editor/run-desktop-pointer.sh",
            ),
            False,
        ),
    )
    # The clipboard client is part of the same native packages, not a Python UI.
    rows += (("12", "clipboard", "Пример сессионного Copy/Cut/Paste между окнами",
              (prefix / "disvm-runtime/run-desktop-clipboard.sh",
               prefix / "disvm-editor/run-desktop-clipboard.sh"), False),)
    rows += (("13", "envelope", "Dis64: инспектор контейнера (демо, без исполнения)",
              (prefix / "disvm-editor/run-dis64-envelope.sh",
               prefix / "disvm-runtime/run-dis64-envelope.sh",
               prefix / "disvm-host/run-dis64-envelope.sh",
               prefix / "disvm/run-dis64-envelope.sh"), False),)
    rows += (("14", "pilot", "Dis64: проверка metadata/types/code pilot42 (без исполнения)",
              (prefix / "disvm-editor/run-dis64-pilot.sh",
               prefix / "disvm-runtime/run-dis64-pilot.sh",
               prefix / "disvm-host/run-dis64-pilot.sh",
               prefix / "disvm/run-dis64-pilot.sh"), False),)
    rows += (("15", "storage", "Dis64: initialized data40/ADDW2 (проверка без исполнения)",
              (prefix / "disvm-editor/run-dis64-storage.sh",
               prefix / "disvm-runtime/run-dis64-storage.sh",
               prefix / "disvm-host/run-dis64-storage.sh",
               prefix / "disvm/run-dis64-storage.sh"), False),)
    rows += (("16", "syntax", "Coco/R: ленивый анализ Limbo/AS (проверка сервиса, без GUI)",
              (prefix / "disvm-editor/run-syntax-check.sh",
               prefix / "disvm-runtime/run-syntax-check.sh"), False),)
    rows += (("17", "tokens", "Coco/R: контекст, поиск и лексические кандидаты (пример без GUI)",
              (prefix / "disvm-editor/run-token-query.sh",
               prefix / "disvm-runtime/run-token-query.sh"), False),)
    entries = {}
    for key, name, label, candidates, graphical in rows:
        script = next((path for path in candidates if path.is_file()), None)
        if script is not None:
            entries[key] = (name, label, script, graphical)
    return entries


# Launch one admitted installed application. entry contains immutable discovery
# metadata; document/font are literal argv values. interactive controls only the
# optional source-path question, never a font search. Return the child exit status.
def launch_product(entry, args, bash, interactive):
    name, label, script, graphical = entry
    command = [bash, str(script)]
    if graphical:
        font = os.environ.get("FVM_EDITOR_FONT")
        if font:
            command += ["--font", font]
        document = getattr(args, "document", None)
        if interactive and document is None:
            document = input("Файл UTF-8 (Enter — новый буфер без пути сохранения): ").strip()
        if document:
            command += ["--document", document]
    print("Запуск " + label + ": " + " ".join(shlex.quote(value) for value in command), flush=True)
    return subprocess.call(command)


# Show actual installed products in interactive and CLI builds. args supplies
# prefix/launch policy; bash is resolved. Return a real child status, 2 for an
# unavailable explicit launch, or zero when the user finishes. Missing run-checks
# never hides installed bytecode applications or their documented entry points.
def show_products(args, bash, interactive):
    requested = getattr(args, "launch", None)
    if args.no_install:
        print("Установка отключена; команды в out/ не создавались.")
        return 2 if requested else 0
    prefix = Path(args.out).expanduser().resolve() if args.out else ROOT / "out"
    checker = prefix / "run-checks.sh"
    window = prefix / "flashvm/run-demo.sh"
    entries = product_entries(prefix)
    print("\nРезультаты сборки: " + str(prefix))
    if checker.is_file():
        print("1 — Проверки: " + str(checker))
    if window.is_file():
        print("2 — Окно, ввод и звук: " + str(window))
    for key, (_, label, script, _) in entries.items():
        print(key + " — " + label + ": " + str(script))
    for component in ("disvm-editor", "disvm-runtime"):
        guide = prefix / component / "ENTRYPOINTS.md"
        if guide.is_file():
            print("Запуск и возможности: " + str(guide))
    if "9" in entries and "8" not in entries:
        print(
            "MovieClip-стол отсутствует в этом пакете. Пересоберите и установите disvm-editor; пункт 9 запускает прежний редактор."
        )
    if requested:
        entry = next((row for row in entries.values() if row[0] == requested), None)
        if entry is None:
            print(
                "Не установлен запуск "
                + requested
                + ". Для desktop/editor выберите disvm-editor; для примеров — disvm-runtime; для envelope/pilot/storage также подходит disvm-host."
            )
            return 2
        return launch_product(entry, args, bash, False)
    if not interactive or (not entries and not checker.is_file() and not window.is_file()):
        return 0
    while True:
        print("\nЗапустить установленный продукт:")
        if checker.is_file():
            print("  1 — проверки")
        if window.is_file():
            print("  2 — оконный пример")
            if checker.is_file():
                print("  3 — проверки и окно")
        for key, (_, label, _, _) in entries.items():
            print("  " + key + " — " + label)
        value = input("Enter — закончить, либо номер запуска: ").strip()
        if not value:
            return 0
        if value in entries:
            return launch_product(entries[value], args, bash, True)
        if value in ("1", "3") and checker.is_file():
            status = subprocess.call([bash, str(checker)])
            if status or value == "1":
                return status
        elif value != "2":
            print("Выберите номер установленного продукта или Enter.")
            continue
        if value in ("2", "3") and window.is_file():
            return subprocess.call(
                [bash, str(checker), "--window"] if checker.is_file() else [bash, str(window)]
            )
        print("Оконный пакет не установлен. Выберите оконную сборку FlashVM в меню.")


# Build an argv list, never a shell expression; SDK preflight belongs to Bash.
def build_command(args, projects, bash):
    """Construct the shared builder command with literal paths and arguments."""
    command = [
        bash,
        str(Path(__file__).resolve().with_name("build-workspace.sh")),
        "--projects",
        ",".join(projects),
        "--jobs",
        str(args.jobs),
        "--config",
        args.config,
    ]
    if args.build_root is not None:
        command.extend(["--build-root", args.build_root])
    if getattr(args, "out", None):
        command.extend(["--out", args.out])
    if getattr(args, "no_install", False):
        command.append("--no-install")
    if getattr(args, "vm_profiles", None):
        command.extend(["--vm-profiles", args.vm_profiles])
    if getattr(args, "player", False):
        command.append("--player")
    if getattr(args, "no_player", False):
        command.append("--no-player")
    if args.tests:
        command.append("--tests")
    command.extend(["--backend", args.backend])
    if args.demo:
        command.extend(["--demo", args.demo])
    return command


# Return the builder's exit status so scripts can detect configuration/test errors.
def main(argv=None):
    """Parse CLI options or open a menu, then execute the common Bash builder."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--projects", help="Names/numbers separated by commas; all for every component"
    )
    parser.add_argument("--jobs", type=int, default=2, help="Parallel jobs (default 2)")
    parser.add_argument("--config", default="Release")
    parser.add_argument("--backend", choices=("cmake", "meson", "make", "mk", "auto"), default=None)
    parser.add_argument("--build-root", help="Output directory; default build/workspace")
    parser.add_argument("--out", help="Local installation directory; default out/")
    parser.add_argument(
        "--no-install", action="store_true", help="Explicitly skip local product installation"
    )
    parser.add_argument(
        "--vm-profiles", help="Comma-separated VM profiles: default,minimal,agg,cairo,player"
    )
    presentation = parser.add_mutually_exclusive_group()
    presentation.add_argument(
        "--no-player", action="store_true", help="Explicit console-only/offline VM build"
    )
    presentation.add_argument(
        "--player",
        action="store_true",
        help="Require the native window/audio/input player; SDK is bootstrapped automatically",
    )
    parser.add_argument("--tests", action="store_true", help="Run selected component tests")
    parser.add_argument(
        "--demo",
        nargs="?",
        const="navigation",
        choices=("navigation", "history", "replaceall"),
        help="Run a Limbo IDE example (default navigation; requires ide)",
    )
    parser.add_argument(
        "--list-backends", action="store_true", help="Show locally available build engines"
    )
    parser.add_argument("--launch", choices=("desktop", "editor", "lifecycle", "pointer", "clipboard", "envelope", "pilot", "storage", "syntax", "tokens"),
                        help="After a successful build launch an installed application; desktop is the MovieClip IDE")
    parser.add_argument("--document", help="Literal UTF-8 source path for --launch desktop/editor")
    parser.add_argument("--dry-run", action="store_true", help="Print command without executing")
    args = parser.parse_args(argv)
    if args.launch and args.no_install:
        parser.error("--launch requires installation; remove --no-install")
    if args.document is not None and args.launch not in ("desktop", "editor"):
        parser.error("--document requires --launch desktop or editor")
    if args.list_backends:
        show_backends()
        return 0
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if not args.config or args.build_root == "":
        parser.error("Configuration and build root must not be empty")
    interactive = args.projects is None
    if interactive:
        if not sys.stdin.isatty():
            parser.error("Use --projects when stdin is not a terminal")
        projects = choose_components()
        if projects is None:
            return 0
        projects, args.backend = choose_compatible_backend(projects, args.backend)
        if projects is None:
            return 0
        if "vm" in projects and not args.vm_profiles and not args.no_player:
            choose_presentation(args)
        choose_settings(args)
        if (
            args.backend not in NATIVE_BACKENDS or set(projects).issubset(NATIVE_TEST_COMPONENTS)
        ) and not args.tests:
            args.tests = ask_yes("Запустить тесты выбранных компонентов?")
        if args.backend not in NATIVE_BACKENDS and "ide" in projects and not args.demo:
            if shutil.which(os.environ.get("EMU_EXECUTABLE", "")):
                if ask_yes("Запустить консольный пример IDE?"):
                    args.demo = choose_example()
            else:
                print(
                    "Компонент ide создаёт библиотечные Dis/.sbl. Для приложений выберите disvm-runtime или disvm-editor; внешний emu им не нужен."
                )
    else:
        try:
            projects = selection(args.projects)
        except ValueError as error:
            parser.error(str(error))
    if args.backend is None:
        args.backend = "auto"
    try:
        validate_backend(args.backend, projects, args.tests, args.demo)
    except ValueError as error:
        parser.error(str(error))
    if args.demo and "ide" not in projects:
        parser.error("--demo requires ide in the selection")
    bash = shutil.which("bash")
    if bash is None:
        parser.error("Bash not found; install bash first")
    command = build_command(args, projects, bash)
    print("\nКоманда: " + " ".join(shlex.quote(item) for item in command), flush=True)
    if args.dry_run:
        return 0
    # Inherit terminal streams and environment, including explicit native SDK paths.
    try:
        status = subprocess.call(command)
        if status:
            return status
        return show_products(args, bash, interactive)
    except OSError as error:
        print("Не удалось запустить сборку: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (KeyboardInterrupt, EOFError):
        print("\nСборка прервана пользователем.", file=sys.stderr)
        sys.exit(130)
