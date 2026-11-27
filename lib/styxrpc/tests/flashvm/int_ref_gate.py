# Copyright (c) 2026 bhgv
# EN: Gate metadata and single-value transfers on the original DisVM .var fid.
# RU: Удерживать метаданные и передачу одного значения на исходном fid .var в DisVM.
import struct

from vfs_control_gate import Gate

VALUE_NAME = "value"  # Only the fixture's .var file has this name.
INT_BYTES = 4  # Ordered signed Int32 payload size.


# EN: Distinguish metadata before and after open, observe actual writes and retire exact fids.
# RU: Различать метаданные до и после открытия, учитывать записи и закрытие точных fid.
class IntRefGate(Gate):
    stages = ((124, "check"), (112, "open"), (124, "identity"), (116, "read"), (118, "write"))

    # EN: Borrow directory for the relay and track only the selected variable's fids.
    # RU: Заимствовать каталог для канала и учесть только fid выбранной переменной.
    def __init__(self, directory):
        super().__init__(directory)
        self.opened = set()
        self.retiring = set()
        self.clunks = {}
        self.writes = []
        self.recovery_write = False

    # EN: Confirm exact fid retirement only on original Rclunk.
    # RU: Подтвердить закрытие точного fid только по исходному Rclunk.
    def response(self, kind, tag, frame):
        if kind == 121 and tag in self.clunks:
            self.retiring.discard(self.clunks.pop(tag))
            self.recovered = self.recovery_write and not self.retiring
        return super().response(kind, tag, frame)

    # EN: Inspect a borrowed original request. Match its transfer phase and reject
    # premature data or any replay of the unacknowledged assignment.
    # RU: Проверить заимствованный запрос, определить этап и отклонить ранние
    # данные или повтор неподтверждённого присваивания.
    def select_request(self, kind, tag, frame):
        if kind not in (112, 116, 118, 120, 124):
            return frame
        fid, = struct.unpack_from("<I", frame, 7)
        if self.paths.get(fid) != VALUE_NAME:
            return frame
        if kind == 120:
            self.opened.discard(fid)
            if fid in self.retiring:
                self.clunks[tag] = fid
            return frame
        if kind == 124:
            name = "identity" if fid in self.opened else "check"
        elif kind == 112:
            self.opened.add(fid)
            name = "open"
        else:
            offset, count = struct.unpack_from("<QI", frame, 11)
            if fid not in self.opened or offset or count != INT_BYTES or len(self.flushed) < 3:
                raise RuntimeError("invalid Int32 request during cancellation setup")
            name = "read" if kind == 116 else "write"
            if kind == 118:
                expected = (42, 73)
                value = int.from_bytes(frame[23:], byteorder="big", signed=True)
                position = len(self.writes)
                if position >= len(expected) or value != expected[position] or len(self.flushed) != 4 + position:
                    raise RuntimeError("early or replayed Int32 assignment")
                self.writes.append(value)
                if position == 1:
                    self.recovery_write = True
                    self.recovered = not self.retiring

        step = len(self.selected)
        pair = kind, name
        if step < len(self.stages) and pair == self.stages[step]:
            self.selected.append(pair)
            self.pending[tag] = pair
            self.retiring.add(fid)
        return frame
