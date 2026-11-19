# Copyright (c) 2026 bhgv
# EN: Validate retained decimal limb fixes and route public calls through synchronization.
# RU: Проверить исходные decimal-limbs и направить публичные вызовы через синхронизацию.
"""Validate 32-bit Gay arithmetic limbs and prepare synchronized decimal entry points."""

import argparse
from pathlib import Path


# EN: Validate pinned source and rename its three entry points for the owned
# synchronized facade. Return private source text or reject boundary drift.
# RU: Проверить source и переименовать три входа для собственного синхронного
# фасада. Вернуть личный текст либо отвергнуть расхождение границ.
def prepare(source):
    if "dtoa_requires_32_bit_unsigned_int" not in source or "unsigned  int x[1];" not in source:
        raise ValueError("upstream dtoa must provide checked 32-bit limbs")
    if "(unsigned int)(b) << 16" not in source:
        raise ValueError("upstream dtoa unsigned packing is missing")
    for before, after in (
        ('\nstrtod(const char *s00, char **se)', '\nlimbo_decimal_strtod_unlocked(const char *s00, char **se)'),
        ('\nfreedtoa(char *s)', '\nlimbo_decimal_free_unlocked(char *s)'),
        ('\ndtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve)',
         '\nlimbo_decimal_dtoa_unlocked(double d, int mode, int ndigits, int *decpt, int *sign, char **rve)'),
    ):
        if source.count(before) != 1:
            raise ValueError('decimal synchronization entry drift: ' + before)
        source = source.replace(before, after)
    return source



# EN: CLI supplies pristine source/build-owned output. Write no imported files;
# return normally or raise a preparation error before compilation.
# RU: CLI задаёт исходник и личный выход сборки. Импорт не менять; вернуть успех
# либо ошибку подготовки до компиляции.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(prepare(args.source.read_text()))


if __name__ == "__main__":
    main()
