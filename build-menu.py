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
    ("vm", "FlashVM: виртуальная машина и консольный SWF runner"),
    ("compilers", "Синтаксические фронтенды AS1, AS2, AS3, Limbo"),
    ("fir", "FIR: компилятор, проверка и оптимизатор"),
    ("linker", "SWL: линкер SWO/SWA в SWF"),
    ("symbols", "SBL: читатель отладочных символов Limbo"),
    ("ide", "IDE: Limbo-ядро и отладочные данные"),
    ("as1", "Только синтаксический фронтенд AS1"),
    ("as2", "Только синтаксический фронтенд AS2"),
    ("as3", "Только синтаксический фронтенд AS3"),
    ("limbo", "Собственный Coco/R Limbo: синтаксис, .m и внешние заготовки"),
    ("disvm", "Dis interpreter/JIT core; полный host runner ещё в работе"),
    ("cocor", "Coco/R C: локальная сборка из tools/coco-r-C"),
    ("cocor-limbo", "Coco/R Limbo: C generator и cocor.dis"),
    ("limbo-bootstrap", "Временный native bootstrap из вашего experiment"),
    ("mk", "Inferno mk: нативная сборка из выбранных исходников"),
    ("iyacc", "Inferno iyacc: генератор парсера limbo.y"),
)


# Validate the complete selection before spawning a process or writing files.
def selection(value):
    """Resolve comma/space-separated numbers or names; preserve unique order."""
    names = [name for name, _ in COMPONENTS]
    tokens = value.replace(",", " ").split()
    if tokens in (["all"], ["все"], ["*"]):
        return names[:6] + ["disvm", "mk", "iyacc"]
    if not tokens:
        raise ValueError("Выберите хотя бы один компонент")
    result = []
    for token in tokens:
        if token.isdigit() and all("0" <= ch <= "9" for ch in token) and 1 <= int(token) <= len(names):
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
    for name, variable, executable in (("cmake", "CMAKE", "cmake"),
                                        ("meson", "MESON", "meson"),
                                        ("make", "MAKE", "make")):
        path = shutil.which(os.environ.get(variable, executable))
        detail = path if path else "не найден"
        if name == "meson":
            ninja = shutil.which(os.environ.get("NINJA", "ninja"))
            detail += "; Ninja: " + (ninja if ninja else "не найден")
        entries.append((name, detail))
    bootstrap = ROOT / "tools/mk/bootstrap.sh"
    entries.append(("mk", "исходники есть, будет собран локально" if bootstrap.is_file() else "исходники отсутствуют"))
    entries.append(("auto", "CMake 3.10+, затем Meson/Ninja, Make, локальный mk; с учётом выбора компонентов"))
    return entries


# Print discovery separately from actual version/selection validation in the launcher.
def show_backends():
    """Display what exists on this machine; no probe is build acceptance."""
    print("Сборочные двигатели:")
    for number, (name, detail) in enumerate(backend_status(), 1):
        print("  {}. {:7s} {}".format(number, name, detail))
    print("CMake: весь workspace. Meson/Make/mk: компиляторы, линкер и инструменты; без --tests/--demo.")


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


# Build an argv list, never a shell expression; SDK preflight belongs to Bash.
def build_command(args, projects, bash):
    """Construct the shared builder command with literal paths and arguments."""
    command = [bash, str(Path(__file__).resolve().with_name("build-workspace.sh")),
               "--projects", ",".join(projects), "--jobs", str(args.jobs),
               "--config", args.config]
    if args.build_root is not None:
        command.extend(["--build-root", args.build_root])
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
    parser.add_argument("--projects", help="Names/numbers separated by commas; all for every component")
    parser.add_argument("--jobs", type=int, default=2, help="Parallel jobs (default 2)")
    parser.add_argument("--config", default="Release")
    parser.add_argument("--backend", choices=("cmake", "meson", "make", "mk", "auto"), default=None)
    parser.add_argument("--build-root", help="Output directory; default build/workspace")
    parser.add_argument("--tests", action="store_true", help="Run selected component tests")
    parser.add_argument("--demo", nargs="?", const="navigation", choices=("navigation", "history", "replaceall"),
                        help="Run a Limbo IDE example (default navigation; requires ide)")
    parser.add_argument("--list-backends", action="store_true", help="Show locally available build engines")
    parser.add_argument("--dry-run", action="store_true", help="Print command without executing")
    args = parser.parse_args(argv)
    if args.list_backends:
        show_backends()
        return 0
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if not args.config or args.build_root == "":
        parser.error("Configuration and build root must not be empty")
    if args.projects is None:
        if not sys.stdin.isatty():
            parser.error("Use --projects when stdin is not a terminal")
        projects = choose_components()
        if projects is None:
            return 0
        if args.backend is None:
            args.backend = choose_backend()
        if not args.tests:
            args.tests = ask_yes("Запустить тесты выбранных компонентов?")
        if "ide" in projects and not args.demo:
            if ask_yes("Запустить консольный пример IDE?"):
                args.demo = choose_example()
    else:
        try:
            projects = selection(args.projects)
        except ValueError as error:
            parser.error(str(error))
    if args.backend is None:
        args.backend = "auto"
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
        return subprocess.call(command)
    except OSError as error:
        print("Не удалось запустить сборку: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (KeyboardInterrupt, EOFError):
        print("\nСборка прервана пользователем.", file=sys.stderr)
        sys.exit(130)
