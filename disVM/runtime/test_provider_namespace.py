# Copyright (c) 2026 bhgv
# EN: Real namespace mount/share/copy/unmount, retained FDs and provider lifetime.
# RU: Настоящие namespace mount/share/copy/unmount, retained FD и provider lifetime.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image


# EN/RU: Own host decoys, real guest images and private lifecycle observers to exit.
# Require ordinary opens use namespace provider, then unmount exposes original host.
# Владеть decoys/images/observers до exit; проверить provider lookup и host после unmount.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="provider namespace ") as temporary:
        work = Path(temporary)
        source = (root / "fixtures/provider-fd/main.b").read_text()
        for name in ("provider-read", "provider-write", "provider-list"):
            source = source.replace('"' + name + '"', '"view/' + name + '"')
        source = source.replace('    input :=', '    check(sys->pctl(Sys->FORKNS, nil) > 0, "copy provider namespace");\n    input :=', 1)
        source = source.replace('    sys->print("PASS provider FD', r'''    retained := sys->open("view/provider-read", Sys->OREAD);
    check(retained != nil, "retained provider open");
    check(sys->chdir("view") == -1, "unsupported provider chdir");
    check(sys->bind("view/provider-read", "host-target", Sys->MREPL) == -1, "unsupported provider bind");
    check(sys->remove("view/provider-read") == -1, "unsupported provider remove");
    check(sys->create("view/provider-read", Sys->OWRITE, 8r666) == nil, "unsupported provider create");
    blocked := load ProviderFD "view/provider-read";
    check(blocked == nil, "unsupported provider load");
    check(sys->unmount("host-target", "view") == -1, "foreign unmount source accepted");
    check(sys->unmount("view", "view") == 0, "filtered unmount provider");
    bytes := array[5] of byte;
    check(sys->readn(retained, bytes, 5) == 5 && string bytes == "ABCDE", "unmount invalidated retained FD");
    native := sys->open("view/provider-read", Sys->OREAD);
    check(native != nil && sys->readn(native, bytes, 5) == 5 && string bytes == "HOST!", "unmount did not restore host");
    sys->print("PASS mounted namespace provider\n");
    sys->print("PASS provider FD''')
        image = work / "main.dis"
        fixture = work / "main.b"
        fixture.write_text(source)
        compile_image(compiler, include, root, fixture, image)
        binary = relink(runtime, work, root / "test_provider_namespace.c", 0,
                        ["dvm_namespace_boot_initialize", "dvm_close_worker_stop"])
        (work / "view").mkdir()
        (work / "host-target").write_bytes(b"HOST!")
        for name in ("provider-read", "provider-write", "provider-list"):
            (work / "view" / name).write_bytes(b"HOST!")
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run([str(binary), mode, "--stats",
                                         *(["--gc-locked-poll"] if locked else []), str(image)],
                                        cwd=work, text=True, capture_output=True, timeout=45)
                output = result.stdout + result.stderr
                if result.returncode or "PASS mounted namespace provider" not in output or \
                        "PASS mounted provider destroyed once after all opened handles" not in output or \
                        "ThreadSanitizer:" in output:
                    raise RuntimeError(output[-18000:])
                if any((work / "view" / name).read_bytes() != b"HOST!"
                       for name in ("provider-read", "provider-write", "provider-list")):
                    raise RuntimeError("mounted provider modified host decoys")
                print(f"PASS mounted provider {mode} locked={locked}")


if __name__ == "__main__":
    main()
