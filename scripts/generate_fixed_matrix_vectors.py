#!/usr/bin/env python3
"""Generate independent fixed-point fixtures using Python arbitrary-precision integers."""
import argparse
import random
from pathlib import Path


def signed(value):
    value %= 1 << 32
    return value if value < 1 << 31 else value - (1 << 32)


def product(a, b):
    return signed((a * b) // (1 << 16))


def row(a, b, x, y, t=0):
    return signed(product(a, x) + product(b, y) + t)


def compose(left, right):
    a, b, c, d, tx, ty = left
    e, f, g, h, ux, uy = right
    return (row(a, c, e, f), row(b, d, e, f), row(a, c, g, h),
            row(b, d, g, h), row(a, c, ux, uy, tx), row(b, d, ux, uy, ty))


def literal(value):
    return "(-2147483647L-1L)" if value == -(1 << 31) else f"{value}L"


def matrix(value):
    return "{" + ",".join(map(literal, value)) + "}"


def render():
    rng = random.Random(337)
    edges = [-(1 << 31), -(1 << 31) + 1, -65537, -65536, -65535, -1,
             0, 1, 32768, 65535, 65536, 65537, (1 << 31) - 1]
    pairs = [(a, b) for a in edges for b in edges]
    pairs += [(rng.randint(-(1 << 31), (1 << 31) - 1),
               rng.randint(-(1 << 31), (1 << 31) - 1)) for _ in range(128)]
    lines = ["/* Independent arbitrary-precision fixtures; regenerate with scripts/generate_fixed_matrix_vectors.py. */",
             "struct FixedProductCase { long a,b,result; };",
             "static const struct FixedProductCase fixed_products[] = {"]
    lines += [" {" + ",".join(map(literal, (a, b, product(a, b)))) + "}," for a, b in pairs]
    lines += ["};", "struct FixedMatrixCase { FVM_FixedMatrix left,right,result; };",
              "static const struct FixedMatrixCase fixed_matrices[] = {"]
    identity = (65536, 0, 0, 65536, 0, 0)
    matrices = [(identity, (32768, -32768, 65537, -65537, -1, 1)),
                ((32768, 32768, -32768, -32768, (1 << 31) - 1, -(1 << 31)),
                 (1, -1, 1, -1, 1, 1))]
    for _ in range(24):
        matrices.append(tuple(tuple(rng.randint(-(1 << 31), (1 << 31) - 1)
                                    for _ in range(6)) for _ in range(2)))
    lines += [" {" + ",".join(map(matrix, (a, b, compose(a, b)))) + "}," for a, b in matrices]
    lines += ["};", ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    target = Path(__file__).resolve().parents[1] / "tests/fixed_matrix_vectors.h"
    expected = render()
    if args.check:
        if target.read_text() != expected:
            raise SystemExit("fixed matrix fixtures differ; regenerate them")
    else:
        target.write_text(expected)


if __name__ == "__main__":
    main()
