"""Generate native-cell Sys declarations; unsupported host calls fail explicitly."""

import argparse
from pathlib import Path
import re
import subprocess
import sys


# Generate paired headers from the same compiler used for native-cell modules.
# CLI supplies compiler, Sys interface and owned build output. Returns zero on success.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--sys", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--idehost", type=Path)
    args = parser.parse_args()
    # Always start from pinned source, making repeated preparation byte-identical.
    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).resolve().parents[1] / "host/prepare_native_registers.py"),
            "--source",
            str(args.source),
            "--output",
            str(args.output),
        ],
        check=True,
    )
    text = subprocess.check_output([args.compiler, "-a", "-t", "Sys", args.sys], text=True)
    marker = "typedef struct{char *name;"
    at = text.index(marker)
    srv = subprocess.check_output(
        [args.compiler, "-a", "-t", "Srv", str(Path(args.sys).with_name("srv.m"))], text=True
    )
    srv_at = srv.index(marker)
    loader = subprocess.check_output(
        [args.compiler, "-a", "-t", "Loader", str(Path(args.sys).with_name("loader.m"))], text=True
    )
    loader_at = loader.index(marker)
    (args.output / "runt.h").write_text(text[:at] + srv[:srv_at] + loader[:loader_at])
    (args.output / "loadermod.h").write_text(loader[loader_at:])
    if args.idehost is not None:
        host = subprocess.check_output(
            [args.compiler, "-a", "-t", "Idehost", str(args.idehost)], text=True
        )
        host_at = host.index(marker)
        with (args.output / "runt.h").open("a") as declarations:
            declarations.write(host[:host_at])
        (args.output / "idehostmod.h").write_text(host[host_at:])
    math = subprocess.check_output(
        [args.compiler, "-a", "-t", "Math", str(Path(args.sys).with_name("math.m"))], text=True
    )
    math_at = math.index(marker)
    with (args.output / "runt.h").open("a") as declarations:
        declarations.write(math[:math_at])
    (args.output / "mathmod.h").write_text(math[math_at:])
    math_code = (args.source / "math.c").read_text()
    # This hosted build deliberately uses native libm for scalar IEEE operations.
    math_code = math_code.replace('#include "lib9.h"', '#include "lib9.h"\n#include <math.h>')
    math_code = math_code.replace("__ieee754_", "")
    helper = Path(__file__).with_name("native_math_types.c").read_text()

    math_code = math_code.replace(
        "/* Check full VM lengths", helper + "\n/* Check full VM lengths", 1
    )
    math_code = math_code.replace(
        "bytes->t != &Tbyte || numbers->t != type",
        "!dvm_math_type(bytes->t, &Tbyte) || !dvm_math_type(numbers->t, type)",
    )
    math_code = math_code.replace("array->t != type", "!dvm_math_type(array->t, type)")
    (args.output / "math.c").write_text(math_code)
    (args.output / "srvmod.h").write_text(srv[srv_at:])
    srv_code = (args.source.parent / "emu/port/srv.c").read_text()
    srv_code = srv_code.replace('#include\t"ip.h"', '#include "runt.h"')
    srv_code = srv_code.replace('#include\t"srv.h"', '#include "kernel.h"')
    srv_code = srv_code.replace(
        '#include\t"srvm.h"',
        '#include "srvmod.h"\nextern void dvm_srv_lock(void);\nextern void dvm_srv_unlock(void);\nextern char *dvm_srv_string(String *);\nextern int so_gethostbyname(char *, char **, int);\nextern int so_gethostbyaddr(char *, char **, int);\nextern int so_getservbyname(char *, char *, char *);',
    )
    srv_code = srv_code.replace("static\tQLock\tdbq;", "")
    srv_code = srv_code.replace("qlock(&dbq)", "dvm_srv_lock()").replace(
        "qunlock(&dbq)", "dvm_srv_unlock()"
    )
    srv_code = srv_code.replace("string2c(f->", "dvm_srv_string(f->")
    (args.output / "srv.c").write_text(srv_code)
    (args.output / "sysmod.h").write_text(text[at:])
    functions = set(re.findall(r"void (Sys_\w+)\(void\*\);", text))
    implemented = {
        "Sys_print",
        "Sys_fprint",
        "Sys_sprint",
        "Sys_aprint",
        "Sys_tokenize",
        "Sys_utfbytes",
        "Sys_byte2char",
        "Sys_char2byte",
        "Sys_open",
        "Sys_write",
        "Sys_read",
        "Sys_readn",
        "Sys_fildes",
        "Sys_create",
        "Sys_seek",
    }
    source = '#include "lib9.h"\n#include "kernel.h"\nextern void error(char *);\n'
    for name in sorted(functions - implemented):
        source += "\n/* Reject an unavailable host service without reporting success. */\n"
        source += "void " + name + "(void *frame)\n{\n    USED(frame);\n"
        source += '    error("unsupported native host service: ' + name + '");\n}\n'
    (args.output / "unsupported.c").write_text(source)
    for name in ("string.c", "runt.c"):
        path = args.output / name
        code = path.read_text()
        if "a->t->size != 1 || a->t->np != 0" not in code:
            raise ValueError("upstream byte descriptor validation is missing: " + name)
        if name == "runt.c":
            code = code.replace("\tint i;\n", "\tWORD i;\n", 1)
            code = code.replace(
                "Dis integer arguments occupy four bytes, even on LP64 hosts.",
                "NC64 arguments occupy the explicitly selected native cell width.",
            )
        path.write_text(code)
    path = args.output / "load.c"
    code = path.read_text().replace("*(WORD*)si = disw(isp);", "*(WORD*)si = (int)disw(isp);")
    path.write_text(code)
    path = args.output / "xec.c"
    code = path.read_text()
    code = code.replace("if(!j->entry[op](&DVM_R))", "if(!dvm_runtime_leaf(j, op, &DVM_R))")
    code = code.replace(
        "\t\tdec[DVM_R.PC->add]();", "\t\tdvm_runtime_instructions++;\n\t\tdec[DVM_R.PC->add]();"
    )
    # Classic upstream rows are fixed-width; NC64 keeps its explicitly private cell layout.
    adapters = Path(__file__).with_name("native_tables.c").read_text()
    for operation in ("icase", "casel", "igoto"):
        pattern = r"OP\(" + operation + r"\)\n\{[\s\S]*?\n\}"
        if len(re.findall(pattern, code)) != 1:
            raise ValueError("numeric table boundary changed: " + operation)
        code = re.sub(
            pattern,
            lambda match: "/* NC64_TABLE_ADAPTERS */" if operation == "icase" else "",
            code,
            count=1,
        )
    code = code.replace("/* NC64_TABLE_ADAPTERS */", adapters)
    path.write_text(code)
    path = args.output / "alt.c"
    code = path.read_text()
    code = code.replace(
        "uvlong bytes;\n\n\tif(a == nil",
        "uvlong bytes;\n\tWORD sends, receives;\n\n\tif(a == nil",
        1,
    )
    code = code.replace(
        "memmove(nsend, (uchar*)a+offsetof(Alt, nsend), sizeof(*nsend));",
        "memmove(&sends, (uchar*)a+offsetof(Alt, nsend), sizeof(sends));",
    )
    code = code.replace(
        "memmove(nrecv, (uchar*)a+offsetof(Alt, nrecv), sizeof(*nrecv));",
        "memmove(&receives, (uchar*)a+offsetof(Alt, nrecv), sizeof(receives));\n"
        "\tif(sends < 0 || receives < 0 || sends > 0x7fffffff || receives > 0x7fffffff) return 0;\n"
        "\t*nsend = sends;\n\t*nrecv = receives;",
    )
    path.write_text(code)


if __name__ == "__main__":
    main()
