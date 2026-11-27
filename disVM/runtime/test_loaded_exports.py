# Copyright (c) 2026 bhgv
"""Test the real compiled Dis export catalogue. / Проверить каталог экспортов Dis."""

from io import BytesIO
from pathlib import Path
from selectors import DefaultSelector, EVENT_READ
import subprocess
import sys
from tempfile import TemporaryDirectory
from zlib import crc32


# EN: Read an unsigned S-record field, returning the value and its exact bytes.
# RU: Прочитать беззнаковое поле S-record, вернуть значение и исходные байты.
def unsigned_field(stream):
    data = bytearray(stream.read(1))
    if not data or data[0] & 0x40:
        raise ValueError("missing or signed profile field")
    value = data[0] & 0x3F
    while not data[-1] & 0x80:
        part = stream.read(1)
        if not part:
            raise ValueError("truncated profile field")
        data.extend(part)
        value = (value << 7) | (part[0] & 0x7F)
    return value, data


# EN: Validate complete owner records and extract their named call shapes.
# RU: Проверить полные записи владельца и извлечь имена со схемами вызовов.
def offers(profile):
    stream = BytesIO(profile)
    result = {}
    while stream.tell() < len(profile):
        checksum, _ = unsigned_field(stream)
        length, encoded_length = unsigned_field(stream)
        encoded_record = stream.read(length)
        crc_mask = (1 << min(32, 6 + 7 * (len(encoded_length) - 1))) - 1
        if len(encoded_record) != length or crc32(encoded_length + encoded_record) & crc_mask != checksum:
            raise ValueError("invalid profile record")
        record = BytesIO(encoded_record)
        tag, _ = unsigned_field(record)
        payload = record.read()
        if len(payload) < 6 or payload[0] != 4 or not any(payload[1:5]):
            raise ValueError("invalid owner offer identity")
        if tag == 71:
            name, shape = payload[5:], None
        elif tag == 88 and len(payload) >= 8:
            names, args, returns = payload[5:8]
            if len(payload) != 8 + names + args + returns:
                raise ValueError("invalid owner offer shape length")
            name = payload[8:8 + names]
            shape = payload[8 + names:8 + names + args], payload[8 + names + args:]
        else:
            raise ValueError("invalid owner offer tag")
        if name in result:
            raise ValueError("duplicate owner offer")
        result[name] = shape
    return result


# EN: Compile independent images, then replace the path while the guest holds its anchor.
# RU: Собрать образы и заменить путь, пока процесс удерживает исходный экземпляр.
def check(runtime, limbo, mode):
    here = Path(__file__).resolve().parent
    dis = here.parent
    sources = here / "tests/loaded_exports"
    with TemporaryDirectory(prefix="disvm-loaded-exports-") as temp:
        work = Path(temp)
        for source, output in (("original", "target"), ("replacement", "next"), ("overflow", "overflow"), ("client", "client")):
            subprocess.run(
                (limbo, "-D", "RG", "-I", dis / "upstream/module", "-I", dis / "module",
                 "-o", work / f"{output}.dis", sources / f"{source}.b"),
                cwd=work, check=True, timeout=15,
            )
        with subprocess.Popen((runtime, "--native-spawn", mode, "client.dis"), cwd=work,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as guest:
            try:
                with DefaultSelector() as ready:
                    ready.register(guest.stdout, EVENT_READ)
                    if not ready.select(5):
                        raise RuntimeError("guest did not retain the original module")
                marker = guest.stdout.readline()
                if marker != "READY original module\n":
                    raise RuntimeError(f"guest stopped before replacement: {marker}")
                (work / "next.dis").replace(work / "target.dis")
                (work / "replacement-ready").touch()
                output, _ = guest.communicate(timeout=7)
                if guest.returncode or "PASS exact compiled export catalogue" not in output:
                    raise RuntimeError(output)
                profiles = {key: bytes.fromhex(value) for key, _, value in
                            (line.partition(" ") for line in output.splitlines()) if key.endswith("-profile")}
                original = profiles["original-profile"]
                if original != profiles["held-profile"]:
                    raise RuntimeError("retained profile changed")
                published = offers(original)
                if set(published) != {b"add", b"current", b"label", b"ports"}:
                    raise RuntimeError(f"wrong export profile: {published}")
                order = next(line.partition(" ")[2] for line in output.splitlines()
                             if line.startswith("compiled-names "))
                if list(published) != order.encode().split(b"\t"):
                    raise RuntimeError("offers and names disagree on compiled order")
                if published[b"label"] is None or published[b"ports"] is None:
                    raise RuntimeError(f"lost managed call shapes: {published}")
                if set(offers(profiles["fresh-profile"])) != {b"other"}:
                    raise RuntimeError("fresh profile did not select replacement")
                print(output, end="")
            except (RuntimeError, subprocess.TimeoutExpired) as failure:
                if guest.poll() is None:
                    guest.kill()
                output, _ = guest.communicate()
                raise RuntimeError(f"{mode}: {failure}\n{output}") from failure


if __name__ == "__main__":
    check(*(Path(arg).resolve() for arg in sys.argv[1:3]), sys.argv[3])
