# Copyright (c) 2026 bhgv
# EN: Select original control replies from DisVM's live module and check for a method write.
# RU: Выбрать исходные ответы живого модуля DisVM и проверить отправку вызова.
import struct

from vfs_control_gate import Gate


# EN: Observe the real paired session and allow method writes only in recovery.
# RU: Наблюдать штатную пару и разрешать записи вызовов только при восстановлении.
class ModuleSetupGate(Gate):
    # Ordered Topen/Tstat/Tread replies on the load service and the callable module.
    stages = ((112, "open"), (124, "generation"), (116, ".interface"),
              (112, "increment"), (112, "label"))

    # EN: Inspect one borrowed 9P frame. Hold its reply when its tag matches
    # the next stage; abort on any early method write or replay of a held open.
    # RU: Проверить заимствованный кадр 9P. Удержать ответ следующего этапа;
    # завершить тест при ранней записи вызова или повторе ожидающего открытия.
    def select_request(self, kind, tag, frame):
        if kind not in (112, 116, 118, 124):
            return frame
        fid, = struct.unpack_from("<I", frame, 7)
        name = self.paths.get(fid)
        if kind == 124 and name and len(name) == 8 and all(c in "0123456789abcdef" for c in name):
            name = "generation"
        pair = kind, name
        stage = len(self.selected)
        if stage < len(self.stages) and pair == self.stages[stage]:
            self.selected.append(pair)
            self.pending[tag] = pair
        elif (kind == 112 and name in ("increment", "label") and stage < len(self.stages)):
            raise RuntimeError(f"method open before its selected stage: {name}")
        elif kind == 118 and name in ("increment", "current", "label"):
            if len(self.flushed) != len(self.stages) or name == "increment":
                raise RuntimeError(f"unexpected method write: {name}")
            if name == "label":
                self.recovered = True
        elif kind == 118 and name == "open" and not self.flushed:
            raise RuntimeError("load submitted before the control file opened")
        return frame
