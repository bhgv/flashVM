"""Generate native-cell Sys declarations; unsupported host calls fail explicitly."""

import argparse
from pathlib import Path
import re
import subprocess
import sys


# Apply exactly one reviewed relative-sequence source adaptation; reject drift.
# source/before/after are owned strings, count is the required single occurrence.
# Return replaced text or raise before publishing an incomplete generated view.
def relative_replace(source, before, after, count=1):
    if count != 1 or source.count(before) != 1:
        raise ValueError("relative sequence boundary changed: " + before[:80])
    return source.replace(before, after, 1)


# Generate paired headers from the same compiler used for native-cell modules.
# CLI supplies compiler, Sys interface and owned build output. Returns zero on success.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--sys", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--idehost", type=Path)
    parser.add_argument("--native-demo", type=Path)
    parser.add_argument("--native-bindings", type=Path)
    args = parser.parse_args()
    subprocess.run(
        [sys.executable, str(Path(__file__).with_name("prepare_exception.py")),
         "--source", str(args.source.parent / "emu/port/exception.c"),
         "--output", str(args.output / "exception.c")], check=True)
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
    if args.native_demo is not None:
        if args.native_bindings is None:
            raise ValueError("native module tutorial requires our Coco/R limbo_bindings")
        subprocess.run([str(args.native_bindings), "--format", "native-c-nc64", str(args.native_demo),
                        str(args.output / "nativedemo_native.h")], check=True)
    # Generate the hosted-only table ABI from the same native-cell compiler.
    table_interface = Path(__file__).resolve().parents[1] / "module/bigtable.m"
    table = subprocess.check_output(
        [args.compiler, "-a", "-t", "Bigtable", str(table_interface)], text=True
    )
    table_at = table.index(marker)
    with (args.output / "runt.h").open("a") as declarations:
        declarations.write(table[:table_at])
    (args.output / "bigtablemod.h").write_text(table[table_at:])
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
        "Sys_stat",
        "Sys_fstat",
        "Sys_dirread",
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
        legacy_byte_guard = "a->t->size != 1 || a->t->np != 0" in code
        staged_byte_guard = (name in ("string.c", "runt.c") and "if(view.t != &Tbyte)" in code
                             and "if(size != 1 || pointers != 0)" in code
                             and "if(owner.t != &Tarray)" in code)
        if not legacy_byte_guard and not staged_byte_guard:
            raise ValueError("upstream byte descriptor validation changed; review owner preflight: " + name)
        if name == "runt.c":
            code = code.replace("\tint i;\n", "\tWORD i;\n", 1)
            code = code.replace(
                "Dis integer arguments occupy four bytes, even on LP64 hosts.",
                "NC64 arguments occupy the explicitly selected native cell width.",
            )
        path.write_text(code)
    # Relative indexes are host language extensions, kept outside the snapshot.
    for filename in ("xec.c", "string.c"):
        path = args.output / filename
        code = path.read_text()
        code = relative_replace(code, '#include "interp.h"', '#include "interp.h"\n#include <stdint.h>', 1)
        # Insert helpers after all includes/macros but before the first operation.
        point = code.index("OP(indc)" if filename == "string.c" else "OP(runt)")
        code = code[:point] + '/* RELATIVE_SEQUENCE_HELPERS */\n' + code[point:]
        helpers = Path(__file__).with_name("relative_sequence.c").read_text()
        code = relative_replace(code, "/* RELATIVE_SEQUENCE_HELPERS */", helpers)
        if filename == "xec.c":
            code = relative_replace(code, 'array == H || array == nil || index < 0 ||', 'array == H || array == nil ||', 1)
            needle = 'memmove(&length, (uchar*)array+offsetof(Array, len), sizeof(length));'
            if code.count(needle) != 1:
                raise ValueError("array indexing preflight changed")
            code = relative_replace(code, needle, needle + '\n\tindex = dvm_relative_index(length, index);', 1)
            # A relative-bound array slice copies first, then applies the original
            # validated view machinery to that independent backing owner.
            start = code.index('OP(slicea)')
            finish = code.index('OP(slicela)', start)
            part = code[start:finish]
            part = relative_replace(part, 'WORD v, n, start;', 'WORD v, n, start;\n\tint relative;\n\tArray *copy;')
            part = relative_replace(part, 'if(ds == H || ds == nil){',
                'relative = start < 0 || v < 0;\n\tif(ds == H && start == 0 && v == -1) return;\n\tif(ds == H || ds == nil){', 1)
            part = relative_replace(part, 'if(start < 0 || v < start || v > ds->len)',
                'if(relative) dvm_relative_bounds(ds->len, &start, &v);\n\tif(start < 0 || v < start || v > ds->len)', 1)
            needle = 'data = arrayspan(ds, t, start, n);'
            if part.count(needle) != 1:
                raise ValueError("array slice preflight changed")
            part = relative_replace(part, needle, needle + '\n\tif(relative){\n\t\tcopy = arraycpy(ds);\n\t\tmemmove(DVM_R.d, &copy, sizeof(copy));\n\t\tdestroy(ds);\n\t\tds = copy;\n\t\tt = ds->t;\n\t\tdata = arrayspan(ds, t, start, n);\n\t}', 1)
            code = code[:start] + part + code[finish:]
            needle = 'if(dl < 0 || dl > ds->len || ss->len < 0 || ss->len > ds->len-dl)'
            if code.count(needle) != 1:
                raise ValueError("array slice-store boundary changed")
            code = relative_replace(code, needle, 'dl = dvm_relative_index(ds->len, dl);\n\t' + needle, 1)
        else:
            needle = 'if(index < 0 || index >= length)'
            if code.count(needle) != 1:
                raise ValueError("string index boundary changed")
            code = relative_replace(code, needle, 'index = dvm_relative_index(length, index);\n\t' + needle, 1)
            needle = 'if(index < 0 || index > length)'
            if code.count(needle) != 1:
                raise ValueError("string store boundary changed")
            code = relative_replace(code, needle, 'index = dvm_relative_index(length, index);\n\t' + needle, 1)
            start = code.index('OP(slicec)')
            finish = code.index('void\ncvtup', start)
            part = code[start:finish]
            part = relative_replace(part, 'if(start < 0 || end < start || (uvlong)end > 0x7fffffff)',
                'if(start >= 0 && end >= 0 && (end < start || (uvlong)end > 0x7fffffff))', 1)
            part = relative_replace(part, 'if(end > length)', 'dvm_relative_bounds(length, &start, &end);\n\t\tif(end > length)', 1)
            part = relative_replace(part, 'result = slicer((ulong)start, (ulong)end, old);',
                'if(old == H) dvm_relative_bounds(0, &start, &end);\n\tresult = slicer((ulong)start, (ulong)end, old);', 1)
            code = code[:start] + part + code[finish:]
        path.write_text(code)
    path = args.output / "load.c"
    code = path.read_text().replace("*(WORD*)si = disw(isp);", "*(WORD*)si = (int)disw(isp);")
    path.write_text(code)
    path = args.output / "xec.c"
    code = path.read_text()
    code = relative_replace(code, '#include "interp.h"', '#include "interp.h"\n#include "table_leases.h"')
    start = code.index('OP(mcall)')
    stop = code.index('\nOP(', start + len('OP(mcall)'))
    body = code[start:stop]
    body = relative_replace(body,
        '\tmemmove((uchar*)f+offsetof(Frame, lr), &DVM_R.PC, sizeof(f->lr));',
        '\t/* Host lease gate runs before call-frame/module publication. */\n'
        '\tif(program == nil && !dvm_bigtable_call_ready(entry.u.runt, f)){\n'
        '\t\treturn;\n'
        '\t}\n'
        '\tmemmove((uchar*)f+offsetof(Frame, lr), &DVM_R.PC, sizeof(f->lr));')
    code = code[:start] + body + code[stop:]

    code = code.replace("if(!j->entry[op](&DVM_R))", "if(!dvm_runtime_leaf(j, op, &DVM_R))")
    # Complete entries must retain the same NC64 table policy and fast-only counts.
    fast_boundary = "if(fast != nil && fast(r))"
    if code.count(fast_boundary) != 1:
        raise ValueError("complete JIT executor changed; review NC64 fast policy")
    code = code.replace(fast_boundary, "if(fast != nil && dvm_runtime_fast(op, fast, r))")
    boundary = "\t\t/* A C-translated step consumes the same single scheduler instruction. */"
    if code.count(boundary) != 1:
        raise ValueError("instruction boundary changed; review runtime quota/count placement")
    # No configured quota needs no external call: keep the ordinary count inline.
    # Both arms still admit exactly one instruction before either execution path.
    dispatch = (
        "\t\tif(dvm_runtime_instruction_limit) {\n"
        "\t\t\tdvm_runtime_step();\n"
        "\t\t} else {\n"
        "\t\t\tdvm_runtime_instructions++;\n"
        "\t\t}\n"
    )
    code = code.replace(boundary, dispatch + boundary, 1)
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
