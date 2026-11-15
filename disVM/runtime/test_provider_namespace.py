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
# Four workers plus retained aliases fit the fixed 29 ordinary slots before retirement.
# Четыре worker и aliases помещаются в 29 slots даже до завершения retirement.
# Require ordinary opens use namespace provider, then unmount exposes original host.
# Владеть decoys/images/observers до exit; проверить provider lookup и host после unmount.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="provider namespace ") as temporary:
        work = Path(temporary)
        source = (root / "fixtures/provider-fd/main.b").read_text()
        source = source.replace("i < 8", "i < 4")
        for name in ("provider-read", "provider-write", "provider-list"):
            source = source.replace('"' + name + '"', '"view/' + name + '"')
        source = source.replace('    input :=', '    check(sys->pctl(Sys->FORKNS, nil) > 0, "copy provider namespace");\n    input :=', 1)
        source = source.replace('    sys->print("PASS provider FD', r'''    retained := sys->open("view/provider-read", Sys->OREAD);
    check(retained != nil, "retained provider open");
    check(sys->chdir("view") == -1, "unsupported provider chdir");
    check(sys->bind("view/provider-read", "host-target", Sys->MREPL) == 0, "provider file bind");
    alias := sys->open("host-target", Sys->OREAD);
    data := array[5] of byte;
    check(alias != nil && sys->readn(alias, data, 5) == 5 && string data == "ABCDE", "provider file alias");
    check(sys->bind("view", "mirror", Sys->MBEFORE) == 0, "provider directory bind");
    uniondir := sys->open("mirror", Sys->OREAD);
    check(uniondir != nil, "provider union directory open");
    (unioncount, unionentries) := sys->dirread(uniondir);
    check(unioncount == 1 && unionentries[0].name == "entry", "provider union directory batch");
    (unioncount, unionentries) = sys->dirread(uniondir);
    check(unioncount == 0 && unionentries == nil, "provider union directory EOF");
    mirrored := sys->open("mirror/provider-read", Sys->OREAD);
    check(mirrored != nil && sys->readn(mirrored, data, 5) == 5 && string data == "ABCDE", "provider directory suffix");
    check(sys->bind("view/provider-list", "nested-target", Sys->MREPL) == 0, "provider subtree bind");
    listed := sys->open("nested-target", Sys->OREAD);
    (listedcount, listedentries) := sys->dirread(listed);
    check(listed != nil && listedcount == 1 && listedentries[0].name == "entry", "provider subtree identity");
    check(sys->bind("host-target", "second-target", Sys->MREPL) == 0, "provider alias-to-alias bind");
    second := sys->open("second-target", Sys->OREAD);
    check(second != nil, "provider alias-to-alias open");
    check(sys->readn(second, data, 5) == 5, "provider alias-to-alias read count");
    check(string data == "ABCDE", "provider alias-to-alias bytes");
    check(sys->bind("view/provider-read", "mirror", Sys->MREPL) == -1, "provider kind mismatch accepted");
    check(sys->remove("view/provider-read") == -1, "unsupported provider remove");
    check(sys->create("view/provider-read", Sys->OWRITE, 8r666) == nil, "unsupported provider create");
    blocked := load ProviderFD "view/provider-read";
    check(blocked == nil, "unsupported provider load");
    check(sys->unmount("host-target", "view") == -1, "foreign unmount source accepted");
    check(sys->unmount("view", "view") == 0, "filtered unmount provider");
    survivor := sys->open("mirror/provider-read", Sys->OREAD);
    check(survivor != nil && sys->readn(survivor, data, 5) == 5 && string data == "ABCDE", "source unmount redirected alias");
    check(sys->unmount("second-target", "host-target") == 0, "filtered provider alias unmount");
    check(sys->unmount(nil, "second-target") == 0, "second alias unmount");
    check(sys->unmount(nil, "mirror") == 0, "mirror unmount");
    check(sys->unmount(nil, "nested-target") == 0, "subtree unmount");
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
        (work / "second-target").write_bytes(b"HOST!")
        (work / "mirror").mkdir()
        (work / "nested-target").mkdir()
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
                if any((work / name).read_bytes() != b"HOST!" for name in ("host-target", "second-target")):
                    raise RuntimeError("provider bind modified host targets")
                print(f"PASS mounted provider {mode} locked={locked}")


if __name__ == "__main__":
    main()
