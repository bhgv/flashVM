"""Unused checked arithmetic may die only after a complete literal trap proof."""

from pathlib import Path
import sys
import tempfile

from integer_identities import execute, module
from table_mutation import call


# Build a typed operation spelling without using the optimizer's implementation.
def opcode(kind, operation):
    prefix = "i32" if kind == "i32" else "big"
    suffix = ".checked" if operation in ("shl", "shr") and kind == "i32" else ""
    if operation in ("div", "mod") and kind == "i32":
        suffix = ".signed"
    return prefix + "." + operation + suffix


# Explicit tools and optional actual VM are borrowed argv. Own all IR/images in
# a private directory. Compare original/optimized results and eager error sites;
# discarded values do not authorize dropping the effects of their producers.
def main():
    compiler = Path(sys.argv[1]).resolve()
    runtime = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None
    mode = "--jit" if len(sys.argv) > 3 and sys.argv[3] == "jit" else "--no-jit"
    checks = 0
    with tempfile.TemporaryDirectory(prefix="total arithmetic DCE ") as directory:
        work = Path(directory)
        raw, optimized, image = (
            work / name for name in ("raw.fir", "opt.fir", "trap.dis")
        )
        for kind, width in (("i32", 32), ("i64", 64)):
            division = opcode(kind, "div")
            for operation in ("div", "mod", "shl", "shr"):
                op = opcode(kind, operation)
                shift = operation in ("shl", "shr")
                right_kind = "i32" if shift else kind
                safe = (1, width - 1) if shift else (3, -7, -(1 << (width - 1)))
                for count in safe:
                    declaration = f"%n: {right_kind} = const.{right_kind} {count}\n"
                    raw.write_text(
                        module(
                            kind,
                            42,
                            declaration + f"%dead: {kind} = {op} %x, %n\ntrace %x",
                        )
                    )
                    call([compiler, "opt", raw, optimized], work)
                    if " = " + op + " " in optimized.read_text():
                        raise RuntimeError("proven-total unused operation survived")
                    for selected in (raw, optimized):
                        execute(compiler, runtime, mode, work, selected, [42])
                    checks += 1
                    # A safe outer operation must not erase a failing producer.
                    raw.write_text(
                        module(
                            kind,
                            42,
                            declaration
                            + f"%zero: {kind} = const.{kind} 0\n%bad: {kind} = {division} %x, %zero\n"
                            f"%dead: {kind} = {op} %bad, %n\ntrace %x",
                        )
                    )
                    call([compiler, "opt", raw, optimized], work)
                    if optimized.read_text().count(" = " + division + " ") != 1:
                        raise RuntimeError(
                            "total-operation DCE erased or duplicated a producer trap"
                        )
                    for selected in (raw, optimized):
                        call([compiler, "dis", selected, image], work)
                        if (
                            runtime
                            and "zero divide"
                            not in call([runtime, mode, image], work, False).stderr
                        ):
                            raise RuntimeError(
                                "discarded value suppressed a producer error"
                            )
                    checks += 1
                # Unsafe literal counts/divisors retain the check and still fail.
                unsafe = (-1, width, 2147483647) if shift else (0,)
                for count in unsafe:
                    raw.write_text(
                        module(
                            kind,
                            42,
                            f"%n: {right_kind} = const.{right_kind} {count}\n"
                            f"%dead: {kind} = {op} %x, %n\ntrace %x",
                        )
                    )
                    call([compiler, "opt", raw, optimized], work)
                    if " = " + op + " " not in optimized.read_text():
                        raise RuntimeError("unused invalid arithmetic lost its check")
                    for selected in (raw, optimized):
                        call([compiler, "dis", selected, image], work)
                        message = "invalid argument" if shift else "zero divide"
                        if (
                            runtime
                            and message
                            not in call([runtime, mode, image], work, False).stderr
                        ):
                            raise RuntimeError(
                                "unused invalid arithmetic did not throw"
                            )
                    checks += 1
                # Unknown values remain conservative, even when this run is safe.
                right = "%x"
                declaration = ""
                if shift and kind == "i64":
                    declaration = "%count: i32 = value.int %x\n"
                    right = "%count"
                raw.write_text(
                    module(
                        kind,
                        2,
                        declaration + f"%dead: {kind} = {op} %x, {right}\ntrace %x",
                    )
                )
                call([compiler, "opt", raw, optimized], work)
                if " = " + op + " " not in optimized.read_text():
                    raise RuntimeError("unknown arithmetic was assumed total")
                for selected in (raw, optimized):
                    execute(compiler, runtime, mode, work, selected, [2])
                checks += 1
    print("PASS total arithmetic DCE:", checks, "typed proof/trap/unknown cases")


if __name__ == "__main__":
    main()
