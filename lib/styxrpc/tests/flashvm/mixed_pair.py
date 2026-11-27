# Copyright (c) 2026 bhgv
# EN: Compile both applications and run the ordinary paired executables over Styx.
# RU: Собрать оба приложения и запустить штатную пару через Styx.
import argparse
from functools import partial
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

from run import compile_image, paired_exchange
from vfs_control_gate import gated_exchange
from module_setup_gate import ModuleSetupGate


# EN: Run the project frontend, FIR optimizer/object emitter and SWF linker.
# RU: Запустить фронтенд проекта, оптимизатор и эмиттер FIR и линкер SWF.
def compile_movie(args, directory, source, name, companions=()):
    movie = directory / (name + ".swf")
    objects = []
    for unit in (source, *companions):
        fir = directory / (unit.stem + ".fir")
        optimized = directory / (unit.stem + ".opt.fir")
        obj = directory / (unit.stem + ".swo")
        for command in ([args.as2, unit, fir], [args.fir, "check", fir],
                        [args.fir_opt, fir, optimized], [args.fir, "object", optimized, obj]):
            subprocess.run(command, check=True, timeout=15)
        objects.append(obj)
    subprocess.run([args.swl, "link", movie, *objects], check=True, timeout=15)


# EN: Test the stock mounted exchange using only the compiled Limbo and AS applications.
# RU: Проверить штатный смонтированный обмен только с собранными приложениями Limbo и AS.
def main():
    parser = argparse.ArgumentParser()
    for field in ("source", "runtime", "player", "limbo", "as2", "fir", "fir-opt", "swl"):
        parser.add_argument("--" + field, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    profiles = parser.add_mutually_exclusive_group()
    for profile in ("named", "typed", "modref", "modref-reverse", "execute", "publish", "nodes",
                    "positioned", "withdraw", "dirlive", "rebind", "module-channel", "asinc-vfs", "asinc-chan",
                    "live-dis", "cancel-dis-call", "cancel-dis-channel", "cancel-vfs-read", "cancel-vfs-write",
                    "cancel-vfs-control", "cancel-dis-setup"):
        profiles.add_argument("--" + profile, dest="profile", action="store_const", const=profile)
    parser.set_defaults(profile="mixed")
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    name = args.profile
    mode = "--jit" if args.jit else "--no-jit"
    with TemporaryDirectory(prefix="styxrpc-mixed-pair-") as scratch:
        directory = Path(scratch)
        fixture_bytes = b"A\x00RPC"
        if name == "mixed":
            (directory / "rpc-source").write_bytes(fixture_bytes)
            (directory / "created").mkdir()
        if name in ("publish", "nodes", "positioned", "withdraw", "dirlive"):
            (directory / "tools").mkdir()
        if name == "asinc-vfs":
            (directory / "tools/.prog").mkdir(parents=True)
            (directory / "tools/result").write_bytes(b"0")
            (directory / "front").mkdir()
            (directory / "front/result").write_bytes(b"1")
        if name == "live-dis":
            for folder, source in (
                ("library", tests / "live-dis_library.b"),
                ("replacement", args.source / "runtime/tests/loaded_exports/replacement.b"),
            ):
                (directory / folder).mkdir()
                compile_image(args, directory, source, folder + "/target.dis")
            (directory / "scalar.dis").write_bytes((directory / "replacement/target.dis").read_bytes())
            (directory / "gate").write_bytes(b"0")
            (directory / "cancel-gate").write_bytes(b"0")
            (directory / "malformed.dis").write_bytes(b"not a Dis image\n")
        if name == "cancel-dis-call":
            compile_image(args, directory, tests / "cancel-dis-call_library.b", "cancel-target.dis")
            (directory / "call-gate").write_bytes(b"0")
        if name == "cancel-dis-channel":
            compile_image(args, directory, tests / "cancel-dis-channel_library.b", "queues.dis")
            (directory / "channel-gate").write_bytes(b"0")
        if name in ("cancel-vfs-read", "cancel-vfs-write"):
            gate = "read-gate" if name == "cancel-vfs-read" else "write-gate"
            (directory / gate).write_bytes(b"0")
            (directory / "neighbour").write_bytes(b"Z")
        if name == "cancel-dis-setup":
            compile_image(args, directory, tests / "live-dis_library.b", "setup.dis")
        if name in ("cancel-vfs-control", "cancel-dis-setup"):
            (directory / "control-gate").write_bytes(b"0")
            (directory / "neighbour").write_bytes(b"Z")
        if name == "cancel-vfs-control":
            (directory / "hold-dir").mkdir()
            (directory / "hold-dir/child").touch()
            for path in ("remove-by-cancel", "attr-cancel", "open-cancel"):
                (directory / path).touch()
        if name == "dirlive":
            (directory / "tools/before").touch()
        if name == "modref-reverse":
            (directory / "mounted/counter.dis/.var").mkdir(parents=True)
        if name == "rebind":
            for path, byte, unique in (("live", b"A", "base-only"), ("front", b"CCC", "front-only"),
                                       ("tail", b"BB", "tail-only")):
                branch = directory / path
                branch.mkdir()
                (branch / "value").write_bytes(byte)
                (branch / unique).write_bytes(byte[:1])
            (directory / "front/.prog").mkdir()
        modules = {
            "positioned": ("styxnodes", "styxexec"),
            "withdraw": ("styxexec",),
            "dirlive": ("styxexec", "styxnodes"),
            "rebind": ("styxexec",),
            "nodes": ("styxnodes",),
            "publish": ("styxexec",),
            "execute": ("styxexec",),
            "typed": ("styxvalue", "styxnamed"),
            "modref": ("styxnamed", "styxmodref"),
            "modref-reverse": ("styxnamed", "styxmodref", "styxnodes"),
            "module-channel": (),
            "live-dis": ("styxnodes",),
            "cancel-dis-call": (),
            "cancel-dis-channel": (),
            "cancel-vfs-read": ("styxnodes",),
            "cancel-vfs-write": ("styxnodes",),
            "cancel-vfs-control": (),
            "cancel-dis-setup": (),
            "asinc-vfs": ("styxnodes",),
            "asinc-chan": ("styxvalue", "styxnamed"),
            "named": ("styxvalue", "styxnamed"),
            "mixed": ("styxintref", "styxchannel"),
        }[name]
        for module in modules:
            compile_image(args, directory, args.source / "limbo" / (module + ".b"), module + ".dis")
        if name in ("publish", "nodes", "positioned"):
            compile_image(args, directory, tests / (name + "_handler.b"), name + "-handler.dis")
        if name == "named":
            (directory / "named-area").mkdir()
        client = compile_image(args, directory, tests / (name + "_pair.b"), name + ".dis")
        companions = {
            "execute": ("execute_apply.as", "execute_refuse.as"),
            "positioned": ("positioned_transfer.as",),
            "withdraw": ("withdraw_original.as", "withdraw_replacement.as"),
            "dirlive": ("dirlive_signal.as",),
            "rebind": ("rebind_signal.as",),
            "live-dis": ("live-dis_worker.as", "live-dis_refusal.as", "live-dis_direct.as",
                         "live-dis_reuse.as", "live-dis_closed.as", "live-dis_cancel.as"),
            "cancel-dis-call": ("cancel-dis-call_worker.as",),
            "cancel-dis-channel": ("cancel-dis-channel_worker.as",),
            "cancel-vfs-read": ("cancel-vfs-read_worker.as",),
            "cancel-vfs-write": ("cancel-vfs-write_worker.as", "cancel-vfs-read_worker.as"),
            "cancel-vfs-control": ("cancel-vfs-control_worker.as",),
            "cancel-dis-setup": ("cancel-dis-setup_worker.as",),
            "asinc-vfs": ("asinc-vfs_worker.as",),
            "asinc-chan": ("asinc-chan_worker.as",),
        }.get(name, ())
        compile_movie(args, directory, tests / (name + "_pair.as"), name,
                      tuple(tests / unit for unit in companions))
        exchange = {
            "cancel-vfs-control": gated_exchange,
            "cancel-dis-setup": partial(gated_exchange, gate_type=ModuleSetupGate),
        }.get(name, paired_exchange)
        exchange(
            directory,
            lambda connection: [args.player, *connection, mode],
            lambda connection: [args.runtime, *connection, "--at", "/peer",
                                *(("--live-dis-load",) if name in ("live-dis", "cancel-dis-call",
                                                                  "cancel-dis-channel", "cancel-dis-setup") else ()), "--",
                                "--native-spawn", mode, client.name],
            f"PASS native {name} RPC paired VMs",
            tcp=args.tcp, timeout=30 if name == "live-dis" else 15,
        )
        if name == "asinc-vfs" and ((directory / "tools/result").read_bytes() != b"0" or
                                    (directory / "task-temp").exists()):
            raise RuntimeError("worker write used the old branch or left a directory behind")
        if name == "mixed" and (directory / "created/nested").exists():
            raise RuntimeError("the movie did not remove its temporary directory")
        for created in (("created/rpc-file", "rpc-root-file") if name == "mixed" else ()):
            if (directory / created).read_bytes() != fixture_bytes:
                raise RuntimeError("the movie did not create the expected VFS file")


if __name__ == "__main__":
    main()
