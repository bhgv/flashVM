# Copyright (c) 2026 bhgv
# EN: Finish hosted ARM64/RV64 generation with guarded C opcode tails.
# RU: Завершить hosted-генерацию ARM64/RV64 защищёнными хвостами C-opcode.
import argparse
from pathlib import Path

ARM = r"""
extern int dvm_runtime_guarded_copies; /* Immutable runtime selection before JIT publication. */


/* [GC:] EN: Emit a native tail to the checked opcode executor in guarded mode.
 * out/cap describe caller-owned code storage; abi/op are validated compiler data.
 * Return emitted words or zero on refusal. The tail preserves the caller LR/SP
 * and passes no unchecked fast callback. Legacy inline code stays reference-only.
 * RU: В guarded-режиме выдать native-хвост к проверенному C executor. out/cap —
 * память caller, abi/op — данные компилятора. Вернуть число слов либо ноль.
 * Хвост сохраняет LR/SP и не передаёт непроверенный fast callback. Старый inline
 * остаётся для reference-режима. */
static int
leafemit(u32int *out, int cap, LeafLayout *abi, int op)
{
    if (dvm_runtime_guarded_copies)
    {
        LeafOp operation;
        if (!leafop(&operation, op, sizeof(WORD)))
        {
            return 0;
        }
        return leafbridgeemit(out, cap, (void *)cjitexec, op, nil);
    }
    return dvm_reference_leafemit(out, cap, abi, op);
}
"""
RV = r"""
extern int dvm_runtime_guarded_copies; /* Immutable before JIT code/cache publication. */


/* [GC:] EN: Emit an RV64 native tail to checked C without an inline fast entry.
 * out/cap/abi/op stay compiler-owned; return words or the emitter's refusal.
 * a0/ra/sp preserve the original REG/caller. RV32 and reference mode retain
 * their original emitter; no ownership fields are read by this generated tail.
 * RU: Выдать RV64-хвост к проверенному C без inline fast. out/cap/abi/op —
 * данные компилятора; вернуть число слов либо отказ emitter. a0/ra/sp сохраняют
 * REG/caller. RV32 и reference используют исходный emitter. Поля владельцев
 * сгенерированный хвост сам не читает. */
static int
rvemit(u32int *out, int cap, RvLayout *abi, int op)
{
#if RVXLEN == 64
    if (dvm_runtime_guarded_copies)
    {
        return rvbridgeemit(out, cap, (uvlong)(uintptr)cjitexec, op, 0);
    }
#endif
    return dvm_reference_rvemit(out, cap, abi, op);
}
"""


# EN: Replace one exact borrowed boundary; return new text or reject source drift.
# No runtime allocation, lock or ownership is involved in source generation.
# RU: Заменить одну точную границу source; вернуть текст либо отказать при дрейфе.
# Генерация не владеет runtime-ресурсами и не захватывает их замки.
def replace(source, before, after):
    if source.count(before) != 1:
        raise ValueError("guarded native tail boundary changed: " + before)
    return source.replace(before, after, 1)


# EN: Adapt a freshly generated compiler name/source after the main preparation.
# Preserve cache sizing/emission symmetry and publish RV64 JIT tables using the
# same builder lock/acquire-release protocol as AMD64/ARM64. Return owned text;
# refuse already adapted or changed input before any output is written.
# RU: Адаптировать свежие name/source после основного prepare. Сохранить симметрию
# sizing/emission и публиковать RV64 JIT тем же lock/acquire-release, что AMD64/ARM64.
# Вернуть текст; повторную адаптацию/дрейф отвергнуть до записи результата.
def adapt(name, source):
    if name == "comp-leaf64.h":
        anchor = '#include LEAF_EMITTER\n#include "leaf-code.h"'
        return replace(
            source,
            anchor,
            "#define leafemit dvm_reference_leafemit\n#include LEAF_EMITTER\n"
            '#undef leafemit\n#include "leaf-code.h"\n' + ARM,
        )
    if name != "comp-riscv.c":
        raise ValueError("unexpected native compiler: " + name)
    source = replace(
        source,
        '#include "riscv-emit.h"',
        '#define rvemit dvm_reference_rvemit\n#include "riscv-emit.h"\n' "#undef rvemit\n" + RV,
    )
    for before, after in (
        ("\tif(m->jit != nil){\n\t\tj = m->jit;", "\tj = dvm_jit_load(m);\n\tif(j != nil){"),
        ("\tm->jit = j;", "\tdvm_jit_publish(m, j);"),
        (
            "\tok = kprotect(rvcompile, &c);",
            "\tdvm_jit_build_lock();\n\tok = kprotect(rvcompile, &c);",
        ),
        ("\treturn ok;\n}", "\tdvm_jit_build_unlock();\n\treturn ok;\n}"),
    ):
        source = replace(source, before, after)
    return '#include "jit_publication.h"\n' + source


# EN: CLI --output owns the generated view; read both inputs and validate all
# changes before writing either. This stage follows prepare.py in the build.
# RU: CLI --output задаёт каталог генерации. Проверить оба входа до любой записи.
# Эта стадия сборки выполняется после prepare.py.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    outputs = {
        name: adapt(name, (args.output / name).read_text())
        for name in ("comp-leaf64.h", "comp-riscv.c")
    }
    for name, text in outputs.items():
        (args.output / name).write_text(text)


if __name__ == "__main__":
    main()
