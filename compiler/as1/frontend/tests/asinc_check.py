"""Check contextual asinc syntax; this oracle never claims runtime scheduling."""
from pathlib import Path
import subprocess
import sys
import tempfile


# Parse fresh bounded files. Ordinary legacy identifiers named asinc remain legal;
# captures and block expressions are syntax here, not an executable-FIR oracle.
def main():
    language, executable = sys.argv[1:3]
    cases = [
        ("declaration", "asinc function update(x){ return x+1; }", True),
        ("block", "var x=1; asinc { x=x+1; }", True),
        ("expression", "var task=asinc { trace(x); };", True),
        ("function-expression", "var work=asinc function(x){ return x; };", True),
        ("comments", "asinc /* capture */ function f(){ asinc /* nested */ { trace(1); } }", True),
        ("legacy-name", "var asinc=1; asinc++; o.asinc(); function asinc(x){return x;}", True),
        ("legacy-call", "asinc(3);", True),
        ("bad-declaration", "asinc function () {}", False),
        ("bad-block", "asinc { var x=; }", False),
        ("unclosed", "asinc { trace(1);", False),
        ("namespace-or-bad-prefix", "asinc var x=1;", language == "as3"),
    ]
    if language != "as1":
        cases.append(("method", "class X { public asinc function f(){return 1;} }", True))
    with tempfile.TemporaryDirectory(prefix="asinc syntax ") as directory:
        for name, source, valid in cases:
            path = Path(directory) / (name+".as")
            path.write_text(source, encoding="utf-8")
            result = subprocess.run([executable,str(path)],capture_output=True,text=True,timeout=5)
            if result.returncode != (0 if valid else 1):
                raise RuntimeError(name+": "+result.stdout+result.stderr)
    print("PASS "+language+" contextual asinc syntax, legacy identifiers and refusal paths")


if __name__ == "__main__":
    main()
