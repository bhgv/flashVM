# Copyright (c) 2026 bhgv
# EN: Interactive presentation/build options; callbacks keep command execution outside.
# RU: Интерактивные настройки представления и сборки; исполнение команд остаётся снаружи.


# Select a visible player configuration; Enter builds the real AGG/JIT desktop host.
# args receives explicit profile/player settings. Returns nothing; invalid input retries.
# RU: Изменить профиль/player в args после ввода; неверный ввод повторяется.
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
# RU: Изменить args через callback ask_yes(prompt); вернуть после подтверждения.
def choose_settings(args, ask_yes):
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


