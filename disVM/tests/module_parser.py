#!/usr/bin/env python3
"""Exercise the new image adapter with retained production parsemod.

Reuse upstream's explicit allocator/GC/JIT test boundary, not an executable VM.
The test validates image acquisition -> real parser -> independent module data.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


# Extract one retained function without modifying or reimplementing its body.
def function(root, path, name, result):
    source = (root / path).read_text()
    start = source.index(result + "\n" + name + "(")
    brace = source.index("{", start)
    depth = 0
    for token in re.finditer(
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*|[{}]', source[brace:]
    ):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start : brace + token.end()]
    raise ValueError(name)


# Compile real parser functions inside the upstream test's documented host fixture.
def main():
    root = Path(__file__).resolve().parents[1]
    upstream = root / "upstream"
    source = (upstream / "libinterp/load.c").read_text()
    parser = source[
        source.index("/* Check the complete encoded operand") : source.index("Module*\nnewmod(")
    ]
    parts = [function(upstream, "emu/port/dis.c", "kprotect", "int")]
    parts.append(
        re.sub(
            r"^#include \"(?:lib9|isa|interp)\.h\"\n",
            "",
            (upstream / "libinterp/disc.c").read_text(),
            flags=re.MULTILINE,
        )
    )
    for path, names in [
        (
            "libinterp/heap.c",
            [("dtype", "Type*"), ("freetype", "void"), ("initmem", "void"), ("initarray", "void")],
        ),
        (
            "libinterp/link.c",
            [("newlink", "static void"), ("mlink", "void"), ("destroylinks", "void")],
        ),
        (
            "libinterp/load.c",
            [
                ("operand", "static int"),
                ("disw", "static ulong"),
                ("canontod", "double"),
                ("brpatch", "int"),
                ("freemod", "void"),
            ],
        ),
    ]:
        parts.extend(function(upstream, path, name, result) for name, result in names)
    unit = (upstream / "libinterp/tests/dis-loader.c").read_text()
    unit = unit.replace("IMPLEMENTATION", "\n".join(parts) + "\n" + parser)
    # Preserve the retained body while accepting both one-line and split C entry declarations.
    # Reject a missing/ambiguous entry before compiling the independent memory-image adapter.
    unit, renamed = re.subn(r"(?m)^int\s+main\s*\(\s*void\s*\)", "int upstream_fixture_main(void)", unit)
    if renamed != 1:
        raise RuntimeError("expected exactly one retained loader fixture entry, found " + str(renamed))
    unit += "\n" + (root / "host/module_image.c").read_text()
    unit += "\n" + re.sub(
        r"^#include \"(?:lib9|isa|interp)\.h\"\n",
        "",
        (root / "host/module_parse.c").read_text(),
        flags=re.MULTILINE,
    )
    unit += r"""#include <stdio.h>
#include <stdlib.h>

/* The production parser must own its result after the input snapshot is freed. */
int main(void)
{
    DVM_ModuleImage image = {0, 0, 0};
    DVM_Module *module = nil;
    upstream_fixture_main();
    fixture();
    if (!(dvm_image_copy("memory:ide-result", buffer, at, sizeof(buffer), &image) == DVM_IMAGE_OK))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    if (!(dvm_image_parse(&image, &module) == DVM_IMAGE_OK))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    if (!(module && strcmp(module->path, "memory:ide-result") == 0))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    dvm_image_dispose(&image);
    if (!(module->nprog == 1))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    drop(module);
    fixture();
    buffer[0] = 0;
    if (!(dvm_image_copy("embedded:invalid", buffer, at, sizeof(buffer), &image) == DVM_IMAGE_OK))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    module = nil;
    if (!(dvm_image_parse(&image, &module) == DVM_IMAGE_BAD_MODULE))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    if (!(module == nil))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    dvm_image_dispose(&image);
    if (!(live == 0 && modules == nil && worker.nerr == 0))
{
    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}
    return 0;
}
"""
    with tempfile.TemporaryDirectory(prefix="disvm-image-parser-") as directory:
        output = Path(directory)
        unit_path = output / "parser.c"
        unit_path.write_text(unit)
        command = shlex.split(os.environ.get("CC", "cc"))
        command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
        command += [
            "-I" + str(upstream / include)
            for include in ["Linux/arm/include", "emu/port", "include", "libinterp"]
        ]
        command += ["-I" + str(root / "include"), str(unit_path), "-o", str(output / "parser")]
        subprocess.run(command, check=True)
        subprocess.run([str(output / "parser")], check=True)


if __name__ == "__main__":
    main()
