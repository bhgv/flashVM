#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Build-engine/component selection; installed launchers and options are separate modules.
# RU: Выбор сборщика и компонентов; установленные команды и настройки вынесены отдельно.
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

ROOT = Path(__file__).resolve().parent  # Workspace root for literal build paths.
sys.path.insert(0, str(ROOT / "tools"))
from menu.products import product_entries, launch_product, show_products
from menu.options import choose_presentation, choose_settings as _choose_settings


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


# RU: Проверить backend/projects/tests/demo до сборки; при несовместимости ValueError.
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


# RU: Для projects выбрать совместимый backend; вернуть имя после ввода.
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
# RU: Разобрать value в имена компонентов; неизвестное значение даёт ValueError.
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
# RU: Без аргументов определить доступные сборщики; вернуть описания без сборки.
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
# RU: Без аргументов вывести результаты обнаружения сборщиков; продукты не создаёт.
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
# RU: Без аргументов запросить сборщик, повторять неверный ввод; вернуть имя.
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
# RU: Без аргументов запросить компоненты; вернуть проверенный список.
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
# RU: Прочитать ответ на prompt; вернуть bool, EOF передаётся вызывающему.
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
# RU: Без аргументов выбрать пример IDE; вернуть его имя после проверки ввода.
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


# EN: Edit args through the shared options UI; injected ask_yes stays testable.
# RU: Изменить args через общие настройки; ask_yes передаётся для проверки.
def choose_settings(args):
    return _choose_settings(args, ask_yes)


# Build an argv list, never a shell expression; SDK preflight belongs to Bash.
# RU: Из args/projects/bash создать список argv; команды не выполняются.
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
# RU: Разобрать argv или открыть меню; выполнить сборку/запуск, вернуть код процесса.
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
    parser.add_argument(
        "--launch",
        choices=("console", "terminal", "services", "modules", "desktop", "editor",
                 "lifecycle", "pointer", "clipboard", "envelope", "pilot", "storage",
                 "syntax", "tokens", "flashvm-native", "resident-owners"),
        help="After a successful build launch an installed application; desktop is the MovieClip IDE",
    )
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
