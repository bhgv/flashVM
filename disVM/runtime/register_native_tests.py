# Copyright (c) 2026 bhgv
# EN: Export the explicit native-only acceptance list as safe CMake registrations.
# RU: Экспортировать явную native-приёмку в безопасные регистрации CMake.
import re
from native_arch_cases import WIDE_CASES


# EN: Emit controlled case names/driver paths/flags only. No arbitrary input or
# execution of target code. Reject malformed metadata before writing CMake text.
# RU: Выдать лишь контролируемые имена/пути/флаги. Нет произвольного ввода или
# исполнения целевого кода. Некорректные метаданные отклонить до выдачи CMake.
def main():
    for name, driver, flags in WIDE_CASES:
        if not all(re.fullmatch(r"[A-Za-z0-9_.-]+", item) for item in (name, driver, *flags)):
            raise RuntimeError("invalid native acceptance metadata")
        arguments = " ".join('"' + flag + '"' for flag in flags)
        test = "disvm_native_" + name.replace("-", "_")
        print(
            f'add_test(NAME {test} COMMAND "${{PYTHON_EXECUTABLE}}" '
            f'"${{CMAKE_CURRENT_SOURCE_DIR}}/{driver}" "$<TARGET_FILE:disvm>" '
            f'"$<TARGET_FILE:limbo-bootstrap>" "${{u}}/module" {arguments})'
        )
        print(f'set_tests_properties({test} PROPERTIES TIMEOUT 1200 LABELS "native")')


if __name__ == "__main__":
    main()
