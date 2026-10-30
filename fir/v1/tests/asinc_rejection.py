"""Never lower recognized frame-free syntax into synchronous executable FIR."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile


# Exercise generated syntax and semantic parsers through the public CLI, with
# exact prefix spans and preserved existing output. Legacy identifier use succeeds.
def main():
    compiler = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="asinc lowering ") as directory:
        root = Path(directory)
        source, output = root / "source.as", root / "output.fir"
        for language in ("as1", "as2", "as3"):
            for text in ("var x=41; asinc { trace(x+1); }", "asinc function work(){trace(42);}", "var task=asinc {trace(42);};"):
                source.write_text(text, encoding="utf-8")
                output.write_bytes(b"old artifact")
                result = subprocess.run([compiler,language,"--input",str(source),str(output),"--diagnostics-json"],capture_output=True,text=True,timeout=10)
                if result.returncode != 1:
                    raise RuntimeError("unsupported closure accepted: "+result.stdout+result.stderr)
                diagnostic = json.loads(result.stderr)
                if diagnostic["kind"] != "semantic" or diagnostic["byte_offset"] != text.index("asinc") or diagnostic["byte_length"] != 5 or "lexical-closure/task lowering" not in diagnostic["message"]:
                    raise RuntimeError("wrong asinc diagnostic: "+str(diagnostic))
                if output.read_bytes() != b"old artifact":
                    raise RuntimeError("unsupported asinc replaced the executable artifact")
            source.write_text("var asinc=41; trace(asinc+1);",encoding="utf-8")
            result = subprocess.run([compiler,language,"--input",str(source),str(output)],capture_output=True,text=True,timeout=10)
            if result.returncode:
                raise RuntimeError("legacy identifier rejected: "+result.stderr)
    print("PASS explicit asinc lowering refusal, exact source spans, rollback and legacy identifiers")


if __name__ == "__main__":
    main()
