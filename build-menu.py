#!/usr/bin/env python3
"""Interactive native workspace build selector; delegates builds to Bash.

Uses only Python 3's standard library. External SDK paths are inherited from
Default dependencies are source-built from tools/. Explicit SDK overrides use
COCOR_ROOT/COCOR_EXECUTABLE, LIMBO_EXECUTABLE, EMU_EXECUTABLE and
LIMBO_MODULE_DIR, as documented for build-workspace.sh.
"""

import argparse
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


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
)


# Validate the complete selection before spawning a process or writing files.
def selection(value):
    """Resolve comma/space-separated numbers or names; preserve unique order."""
    names = [name for name, _ in COMPONENTS]
    tokens = value.replace(",", " ").split()
    if tokens in (["all"], ["все"], ["*"]):
        return names[:6] + ["disvm"]
    if not tokens:
        raise ValueError("Выберите хотя бы один компонент")
    result = []
    for token in tokens:
        if token.isascii() and token.isdigit() and 1 <= int(token) <= len(names):
            token = names[int(token) - 1]
        if token not in names:
            raise ValueError("Неизвестный компонент: " + token)
        if token not in result:
            result.append(token)
    return result


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
    """Choose navigation or history; empty input preserves the original default."""
    while True:
        value = input("Пример: 1 — navigation, 2 — history [1]: ").strip().lower()
        if value in ("", "1", "navigation"):
            return "navigation"
        if value in ("2", "history"):
            return "history"
        print("Введите 1/navigation или 2/history.")


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
    parser.add_argument("--build-root", help="Output directory; default build/workspace")
    parser.add_argument("--tests", action="store_true", help="Run selected component tests")
    parser.add_argument("--demo", nargs="?", const="navigation", choices=("navigation", "history"),
                        help="Run a Limbo IDE example (default navigation; requires ide)")
    parser.add_argument("--dry-run", action="store_true", help="Print command without executing")
    args = parser.parse_args(argv)
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
