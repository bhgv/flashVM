# Copyright (c) 2026 bhgv
# EN: Discover and launch installed products independently of build configuration.
# RU: Обнаружение и запуск установленных продуктов независимо от выбора сборки.

import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]  # Checkout root for the default out/ prefix.


# Discover installed scripts without treating the generic checker as an application
# dependency. prefix is an explicit path; return stable menu IDs and copied metadata.
# The fallback launcher belongs only to the separately labelled legacy editor.
# RU: Обнаружить скрипты в prefix; вернуть стабильные номера и метаданные.
def product_entries(prefix):
    rows = (
        ("4", "console", "Программа Dis", (prefix / "disvm-runtime/run-console.sh", prefix / "disvm-editor/run-console.sh"), False),
        (
            "5",
            "terminal",
            "Терминальный редактор Dis",
            (prefix / "disvm-runtime/run-editor.sh", prefix / "disvm-editor/run-editor.sh"),
            False,
        ),
        ("6", "services", "Сервисы IDE", (prefix / "disvm-runtime/run-ide.sh", prefix / "disvm-editor/run-ide-console.sh"), False),
        ("7", "modules", "Библиотеки Inferno", (prefix / "disvm-runtime/run-modules.sh", prefix / "disvm-editor/run-modules.sh"), False),
        (
            "8",
            "desktop",
            "IDE MovieClip: парный запуск DisVM + FlashVM, MDI, Draw/SVG, Limbo/AS, F7/F8",
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
    rows += (("18", "flashvm-native", "FlashVM: C-модуль, GC и вызов из AS/SWF (без GUI)",
              (prefix / "disvm-editor/run-flashvm-native-module.sh",), False),)
    rows += ((
        "19", "resident-owners", "DisVM: закреплённые нативные потоки и каналы",
        (prefix / "disvm-editor/run-resident-owners.sh",
         prefix / "disvm-runtime/run-resident-owners.sh"), False,
    ),)
    entries = {}
    for key, name, label, candidates, graphical in rows:
        script = next((path for path in candidates if path.is_file()), None)
        if script is not None:
            entries[key] = (name, label, script, graphical)
    return entries


# Launch one admitted installed application. entry contains immutable discovery
# metadata; document/font are literal argv values. interactive controls only the
# optional source-path question, never a font search. Return the child exit status.
# RU: Запустить entry с args/bash; interactive разрешает вопрос о файле. Вернуть код.
def launch_product(entry, args, bash, interactive):
    name, label, script, graphical = entry
    command = [bash, str(script)]
    if graphical:
        font = os.environ.get("FVM_EDITOR_FONT")
        if font and script.name != "run-desktop-ide.sh":
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
# RU: Показать/запустить продукты args через bash; interactive управляет меню. Вернуть код.
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


