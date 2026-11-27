# Copyright (c) 2026 bhgv
# EN: Withhold real replies from the paired DisVM's shared-cell service.
# RU: Удерживать реальные ответы службы общих ячеек парной DisVM.
import struct

from vfs_control_gate import Gate

OP_ATTACH = 5  # Bind a fid to an existing token.
OP_GET = 6  # Read the current cell value.
OP_SET = 7  # Replace the current cell value.
HEADER = 5  # Request opcode and length.
REPLY = 4  # Reply length precedes the body.


# EN: Observe exactly one reply per phase. Verify discarded operations retire their fids.
# RU: Удержать ровно один ответ на фазу и проверить закрытие fid отменённых операций.
class CellSetupGate(Gate):
    stages = ((118, "attach-token"), (116, "read-length"), (116, "read-value"),
              (118, "set-header"), (118, "set-value"))

    # EN: Borrow the fixture directory. Store command and retirement records per fid.
    # RU: Заимствовать каталог теста. Хранить команды и закрытия по каждому fid.
    def __init__(self, directory):
        super().__init__(directory)
        self.commands = {}
        self.retire = set()
        self.clunks = {}
        self.cancelled_header = None
        self.recovered_read = False

    # EN: Forward one reply through the shared gate and confirm exact retired fids.
    # RU: Передать ответ через общий барьер и учесть закрытие точных fid.
    def response(self, kind, tag, frame):
        if kind == 121 and tag in self.clunks:
            self.retire.discard(self.clunks.pop(tag))
            self.recovered = self.recovered_read and not self.retire
        return super().response(kind, tag, frame)

    # EN: Track original .channels requests, checking that a cancelled header
    # never leads to a write of the assignment body.
    # RU: Учесть запросы .channels и проверить, что при отмене
    # заголовка клиент не отправляет тело присваивания.
    def select_request(self, kind, tag, frame):
        if kind not in (116, 118, 120):
            return frame
        fid, = struct.unpack_from("<I", frame, 7)
        if kind == 120:
            if fid in self.retire:
                self.clunks[tag] = fid
            self.commands.pop(fid, None)
            return frame
        if self.paths.get(fid) != ".channels":
            return frame
        offset, count = struct.unpack_from("<QI", frame, 11)
        name = None
        if kind == 118 and not offset and count == HEADER:
            command = frame[23]
            self.commands[fid] = command
            if command == OP_SET:
                name = "set-header"
        elif kind == 118 and offset == HEADER:
            if fid == self.cancelled_header:
                raise RuntimeError("cell assignment continued after cancelling its header")
            command = self.commands.get(fid)
            if command == OP_ATTACH:
                name = "attach-token"
            if command == OP_SET:
                name = "set-value"
        elif kind == 116 and self.commands.get(fid) == OP_GET:
            if not offset:
                name = "read-length"
            elif offset == REPLY:
                name = "read-value"
                if len(self.flushed) == len(self.stages):
                    self.recovered_read = True
                    self.recovered = not self.retire

        step = len(self.selected)
        pair = kind, name
        if step < len(self.stages) and self.stages[step] == pair:
            self.pending[tag] = pair
            self.selected.append(pair)
            self.retire.add(fid)
            if name == "set-header":
                self.cancelled_header = fid
        return frame
