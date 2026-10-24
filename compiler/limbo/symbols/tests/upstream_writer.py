#!/usr/bin/env python3
"""Optional interoperability check against the unchanged upstream sblsrcconv body."""

import argparse
from pathlib import Path
import random
import subprocess
import tempfile


# Compile an explicit upstream producer body with narrow host-only adapters.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", required=True, type=Path)
    parser.add_argument("--tool", required=True, type=Path)
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()
    source = (args.upstream / "limbo/sbl.c").read_text()
    start = source.index("static char*\nsblsrcconv(")
    stop = source.index("\n#define isnilsrc", start)
    body = source[start:stop]
    harness = r"""
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
typedef struct File { int sbl; } File;
typedef struct Fline { File *file; int line; } Fline;
typedef struct Pos { int line, pos; } Pos;
typedef struct Src { Pos start, stop; } Src;
static File files[]={{0},{1},{2}};
static int lastf,lastline;
/* Recover a source file/line from the fixture's bounded virtual line number. */
static Fline fline(int encoded) { Fline v;v.file=files+encoded/100000;v.line=encoded%100000;return v; }
/* Match seprint's returned end pointer without using the Inferno host runtime. */
static char *seprint(char *s,char *end,const char *fmt,...) {
    va_list ap;int n;size_t left=(size_t)(end-s);va_start(ap,fmt);n=vsnprintf(s,left,fmt,ap);va_end(ap);
    if(n<0)return s;return (size_t)n<left?s+n:end-1;
}
"""
    harness += body
    harness += r"""
/* Feed actual upstream compression with independently generated source records. */
int main(void) {
    int af,al,ac,bf,bl,bc,statement;Src s;char text[256];
    while(scanf("%d %d %d %d %d %d %d",&af,&al,&ac,&bf,&bl,&bc,&statement)==7){
        s.start.line=af*100000+al;s.start.pos=ac;s.stop.line=bf*100000+bl;s.stop.pos=bc;
        printf("%s%d\n",sblsrcconv(text,text+sizeof(text),&s),statement);
    }
    return ferror(stdout)?1:0;
}
"""
    rng = random.Random(20261024)
    expected = [
        tuple(rng.randrange(limit) for limit in [3, 30000, 1024, 3, 30000, 1024, 100])
        for _ in range(4096)
    ]
    with tempfile.TemporaryDirectory(prefix="sbl-upstream-") as directory:
        root = Path(directory)
        c = root / "producer.c"
        exe = root / "producer"
        c.write_text(harness)
        subprocess.run([args.cc, "-std=c99", "-O2", str(c), "-o", str(exe)], check=True)
        encoded = subprocess.run(
            [str(exe)],
            input="".join(" ".join(map(str, v)) + "\n" for v in expected).encode(),
            capture_output=True,
            check=True,
        ).stdout
        prefix = b"limbo .sbl 2.1\nUpstream\n3\na.b\nb.b\nc.b\n4096\n" + encoded
        path = root / "actual.sbl"
        path.write_bytes(prefix + b"0\n0\n0\n")
        output = subprocess.run(
            [str(args.tool.resolve()), str(path)], capture_output=True, check=True
        ).stdout.decode()
        rows = [
            tuple(map(int, line.split("\t")[1:]))
            for line in output.splitlines()
            if line.startswith("entry\t")
        ]
        assert rows == [(i, *value) for i, value in enumerate(expected)]
        assert "prefix\t" + str(len(prefix)) + "\n" in output
    print("PASS 4096 source spans emitted by the actual upstream sblsrcconv body")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
