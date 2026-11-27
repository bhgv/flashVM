# Copyright (c) 2026 bhgv
# EN: Test atomic route import on a real DisVM module and the hosted C SDK.
# RU: Проверить атомарный импорт маршрутов на реальном модуле DisVM и SDK C.
import argparse
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

from run import compile_image
from vfs_control_gate import Gate, gated_exchange

FORWARD = 0  # C calls the DisVM export on the forward carrier lane.
TWRITE, TCLUNK, RCLUNK = 118, 120, 121  # Standard 9P request and reply types.
TOKEN_SIZE = 16  # Whole module-channel token at offset zero.


# EN: Hold only the second successful authentication in the armed import batch.
# RU: Удержать только второе успешное подтверждение в отмеченном пакете импорта.
class ModuleImportGate(Gate):
    stages = ((TWRITE, "second-token"),)

    # EN: Borrow the fixture directory. Track the exact temporary fid and clunk tag.
    # RU: Заимствовать каталог теста. Учитывать точные временный fid и тег clunk.
    def __init__(self, directory):
        super().__init__(directory)
        self.count = 0
        self.bus_fid = None
        self.close_tag = None

    # EN: Let all frames pass except the chosen original reply handled by Gate.
    # RU: Пропустить все кадры, кроме выбранного исходного ответа, который удержит Gate.
    def select_request(self, kind, tag, frame):
        if kind == TWRITE:
            fid, offset, count = struct.unpack_from("<IQI", frame, 7)
            if fid == self.bus_fid:
                raise RuntimeError("cancelled authentication sent another write on the same fid")
            if (self.bus_fid is None and self.paths.get(fid) == ".unibus" and offset == 0
                    and count == TOKEN_SIZE and (self.directory / "arm-import").is_file()):
                self.count += 1
                if self.count == 2:
                    self.bus_fid = fid
                    self.selected.append(self.stages[0])
                    self.pending[tag] = self.stages[0]
        if kind == TCLUNK and self.bus_fid is not None and not self.recovered:
            fid, = struct.unpack_from("<I", frame, 7)
            if fid == self.bus_fid:
                if tuple(self.flushed) != self.stages:
                    raise RuntimeError("temporary bus closed before its original tag was flushed")
                self.close_tag = tag
        return frame

    # EN: Require acknowledgement of the exact temporary bus clunk.
    # RU: Требовать подтверждение clunk точного временного fid шины.
    def response(self, kind, tag, frame):
        if kind == RCLUNK and self.close_tag == tag:
            self.recovered = True
        return super().response(kind, tag, frame)


# EN: Compile the existing recursive example and run against the selected transport and VM mode.
# RU: Собрать рекурсивный пример и запустить на выбранном транспорте и в режиме VM.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "runtime", "limbo", "driver"):
        parser.add_argument("--" + name, type=lambda item: Path(item).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    args = parser.parse_args()
    mode = "--jit" if args.jit else "--no-jit"
    with TemporaryDirectory(prefix="styxrpc-channel-admission-") as scratch:
        directory = Path(scratch)
        image = "channel-lifetime.dis"
        source = args.source / "examples/remote-scalar/styx-channel-recursive-owner.b"
        compile_image(args, directory, source, image)
        dispatcher = compile_image(args, directory, args.source / "limbo/scalarowner.b", "dispatcher.dis")
        (directory / "control-gate").write_bytes(b"0")
        gated_exchange(
            directory,
            lambda connection: [args.driver, connection[-1], "control-gate"],
            lambda connection: [args.runtime, "--styx-library-serve-fd", connection[-1],
                                "--export", image, image, "open", "--", "--native-spawn", mode, dispatcher.name],
            "", gate_type=ModuleImportGate, lane=FORWARD, tcp=args.tcp,
            initiator_trace=("PASS delegated PID and module channels between C and DisVM",),
        )
        print("PASS atomic route import against native DisVM")


if __name__ == "__main__":
    main()
