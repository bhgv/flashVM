# Copyright (c) 2026 bhgv
# EN: Withhold acknowledgements for named enrollment and typed operation admission.
# RU: Удерживать подтверждения входа в именованный канал и подготовки типизированного обмена.
import struct

from vfs_control_gate import Gate

TOKEN_SIZE = 16  # Wire size of the module lease token, before the first guard.
NAMED_HEADER = 5  # Opcode and big-endian body length precede the enrollment path.


# EN: Require exact flushes, check the submission boundary, and witness recovery.
# RU: Проверять точные flush, границу отправки запроса и успешное продолжение обмена.
class ChannelSetupGate(Gate):
    stages = ((112, ".channels"), (118, "/cancel-enroll"), (110, ".unibus"),
              (112, ".unibus"), (118, ".unibus"))
    # Ropen, enrollment Rwrite, Rwalk, bus Ropen, and authentication Rwrite.

    # EN: Borrow the fixture directory and track the cancelled enrollment's exact clunk.
    # RU: Заимствовать каталог теста и учесть точный clunk отменённого участия.
    def __init__(self, directory):
        super().__init__(directory)
        self.enrollment_fid = None
        self.enrollment_clunk = None
        self.enrollment_retired = False

    # EN: Observe retirement before passing the reply or applying the shared gate.
    # RU: Учесть снятие участия перед передачей ответа или общей проверкой.
    def response(self, kind, tag, frame):
        if kind == 121 and tag == self.enrollment_clunk:
            self.enrollment_retired = True
        return super().response(kind, tag, frame)

    # EN: Inspect one original request and retain the tag when it matches the next stage.
    # Check that only the recovery worker can write a guard.
    # RU: Проверить исходный запрос и сохранить тег следующего этапа.
    # Запись ветви разрешена только задаче восстановления.
    def select_request(self, kind, tag, frame):
        if kind not in (110, 112, 118, 120):
            return frame
        fid, = struct.unpack_from("<I", frame, 11 if kind == 110 else 7)
        if kind == 120:
            if fid == self.enrollment_fid:
                self.enrollment_clunk = tag
            return frame
        name = self.paths.get(fid)
        if kind == 118 and name in (".channels", ".unibus"):
            offset, count = struct.unpack_from("<QI", frame, 11)
            if name == ".channels":
                if not self.flushed:
                    raise RuntimeError("enrollment write before the service opened")
                if offset == NAMED_HEADER:
                    name = frame[23:].decode("utf-8")
            elif offset or count != TOKEN_SIZE:
                if len(self.flushed) != len(self.stages) or offset != TOKEN_SIZE:
                    raise RuntimeError("guard submitted before channel admission finished")
                self.recovered = True
                return frame
        pair = kind, name
        if len(self.selected) < len(self.stages) and pair == self.stages[len(self.selected)]:
            self.selected.append(pair)
            self.pending[tag] = pair
            if name == "/cancel-enroll":
                self.enrollment_fid = fid
        elif kind == 118 and name == "/cancel-enroll":
            if len(self.flushed) != len(self.stages) or not self.enrollment_retired:
                raise RuntimeError("enrollment repeated before the cancelled fid was retired")
        return frame
