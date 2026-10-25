"""Check direct Dis CLI/API coverage boundaries without an installed VM or bootstrap compiler."""

from pathlib import Path
import os
import subprocess
import sys
import tempfile


# Invoke one borrowed tool with copied arguments and private staging; reject unexpected statuses.
def invoke(tool, arguments, environment, success=True):
    result = subprocess.run([str(tool), *map(str, arguments)], capture_output=True,
                            text=True, env=environment, timeout=30)
    if (result.returncode == 0) != success:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# Build valid i32 Dis and exercise rejected coverage while preserving old destination bytes.
# argv supplies the independently built FIR/source CLIs; no Inferno compiler is invoked here.
def main():
    compiler, source_compiler = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        environment = dict(os.environ, TMPDIR=str(directory))
        source = directory / "source.b"
        fir = directory / "program.fir"
        binary = directory / "program.dis"
        source.write_text("implement V; V: module { value: fn(): int; }; "
                          "value(): int { x := 2147483647; x += 1; return x * -3; }")
        invoke(source_compiler, ["limbo", "--input", source, fir], environment)
        invoke(compiler, ["dis", fir, binary], environment)
        accepted = binary.read_bytes()
        # 819264 in existing signed-30-bit Dis encoding. This only checks the target marker;
        # actual execution and arithmetic results are tested independently on DisVM.
        if not accepted.startswith(bytes.fromhex("c00c8040")):
            raise RuntimeError("wrong explicit NC64 magic")
        invoke(compiler, ["dis", fir, binary], environment)
        if binary.read_bytes() != accepted:
            raise RuntimeError("direct Dis emission is not deterministic")
        for body in ("%v: f64 = const.f64 1.5\ntrace %v\nreturn",
                     "%v: bool = const.bool true\ntrace %v\nreturn",
                     "%v: string = const.string \"x\"\ntrace %v\nreturn",
                     "jump ^next\n^next:\nreturn"):
            fir.write_text("FIR 1.0\nmodule @Reject\nfunc @main() -> void {\n^entry:\n" + body + "\n}\n")
            invoke(compiler, ["check", fir], environment)
            result = invoke(compiler, ["dis", fir, binary], environment, success=False)
            if "Dis i32 backend" not in result.stderr or binary.read_bytes() != accepted:
                raise RuntimeError("unsupported backend coverage changed destination or hid its diagnostic")
        # The same verified input remains valid for other backends after Dis rejects it.
        invoke(compiler, ["print", fir, directory / "copy.fir"], environment)
        invoke(source_compiler, ["limbo", "--input", source, fir], environment)
        invoke(compiler, ["dis", fir, binary], environment)
        if binary.read_bytes() != accepted:
            raise RuntimeError("Dis failure leaked state into the next compilation")
    print("PASS direct Dis backend: deterministic NC64, explicit coverage rejection and destination recovery")


if __name__ == "__main__":
    main()
