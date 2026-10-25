#!/usr/bin/env python3
"""Independent optional system mpg123 decoder shared by original fixture generation and queue oracles."""
import ctypes as C
import os
import struct


class Reference:
    """Own an independent system decoder; never load the runtime's minimp3 backend."""

    def __init__(self):
        self.lib = C.CDLL("libmpg123.so.0")
        self.lib.mpg123_init()
        self.lib.mpg123_new.argtypes = [C.c_char_p, C.POINTER(C.c_int)]
        self.lib.mpg123_new.restype = C.c_void_p
        self.lib.mpg123_param.argtypes = [C.c_void_p, C.c_int, C.c_long, C.c_double]
        self.lib.mpg123_open.argtypes = [C.c_void_p, C.c_char_p]
        self.lib.mpg123_read.argtypes = [C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
        self.lib.mpg123_getformat.argtypes = [
            C.c_void_p,
            C.POINTER(C.c_long),
            C.POINTER(C.c_int),
            C.POINTER(C.c_int),
        ]
        self.lib.mpg123_delete.argtypes = [C.c_void_p]

    def decode(self, root, fixture, latency=529):
        """Disable implicit gapless trimming so SWF latency is the sole source trim."""
        data, rate, channels, _, _, _ = fixture
        source = root / "reference.mp3"
        source.write_bytes(data)
        error = C.c_int()
        handle = self.lib.mpg123_new(None, C.byref(error))
        if not (handle):
            raise AssertionError("condition check failed: flashVM/scripts/free_mp3_reference.py:34")
        try:
            if not (self.lib.mpg123_param(handle, 13, 64, 0) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/free_mp3_reference.py:36"
                )  # REMOVE_FLAGS, GAPLESS.
            if not (self.lib.mpg123_open(handle, os.fsencode(source)) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/free_mp3_reference.py:37"
                )
            hz, count, encoding = C.c_long(), C.c_int(), C.c_int()
            if not (
                self.lib.mpg123_getformat(handle, C.byref(hz), C.byref(count), C.byref(encoding))
                == 0
            ):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/free_mp3_reference.py:39"
                )
            if not ((hz.value, count.value, encoding.value) == (rate, channels, 208)):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/free_mp3_reference.py:43"
                )
            raw = bytearray()
            buffer = (C.c_char * 65536)()
            done = C.c_size_t()
            while True:
                result = self.lib.mpg123_read(handle, buffer, len(buffer), C.byref(done))
                raw += bytes(buffer[: done.value])
                if result == -12:  # MPG123_DONE.
                    break
                if not (result in (0, -11)):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/free_mp3_reference.py:52"
                    )  # OK or NEW_FORMAT, never conceal a failed source.
            samples = list(struct.iter_unpack("h" * channels, raw))
            if not (len(samples) == 5760):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/free_mp3_reference.py:54"
                )
            return [pair if channels == 2 else pair * 2 for pair in samples[latency:]]
        finally:
            self.lib.mpg123_delete(handle)
