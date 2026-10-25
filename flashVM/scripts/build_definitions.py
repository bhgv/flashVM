"""DEFS.txt reader for the Linux standalone compiler runner.

CMake has its own equivalent validator for cross-platform toolchain builds.
"""

from pathlib import Path
import re
import struct
import sys

GROUPS = (
    ("FONTS", "NO_FONTS"),
    ("IMAGE_CODECS", "NO_IMAGE_CODECS"),
    ("GRAPHICS_AGG", "GRAPHICS_CAIRO", "NO_GRAPHICS"),
    ("AUTO_PLATFORM", "WINDOWS", "LINUX", "MAC", "ANDROID"),
    ("HEADLESS", "FB", "GLES", "OGL", "DX", "SDL1", "SDL2", "SDL3"),
    ("SQLITE", "NO_SQLITE"),
    ("OS_PORTS", "NO_OS_PORTS"),
    ("RENDER_JOBS", "NO_RENDER_JOBS"),
    ("INTERPRETER", "JIT"),
    ("NO_MEDIA", "MEDIA"),
)


def read_keys(path):
    keys = []
    known = {key for group in GROUPS for key in group}
    for line in Path(path).read_text().splitlines():
        key = line.split("#", 1)[0].strip()
        if not key:
            continue
        if key not in known:
            raise ValueError("Unknown DEFS.txt key: " + key)
        if key in keys:
            raise ValueError("Duplicate DEFS.txt key: " + key)
        keys.append(key)
    for group in GROUPS:
        if sum(key in keys for key in group) != 1:
            raise ValueError("Exactly one DEFS.txt key required in: " + ", ".join(group))
    if "HEADLESS" not in keys or "MEDIA" in keys:
        raise ValueError("Selected output/media adapter is not implemented")
    if not sys.platform.startswith("linux"):
        raise ValueError("Standalone runner supports Linux; use CMake for other targets")
    if any(key in keys for key in ("WINDOWS", "MAC", "ANDROID")):
        raise ValueError("DEFS.txt platform conflicts with Linux runner")
    if "AUTO_PLATFORM" in keys:
        keys.append("LINUX")
    return keys


def generated_headers(root, output, keys):
    output.mkdir(exist_ok=True)
    (output / "flashvm_build_config.h").write_text(
        "/* Generated from DEFS.txt. */\n" + "".join("#define " + key + " 1\n" for key in keys)
    )
    if "GRAPHICS_CAIRO" not in keys and "IMAGE_CODECS" not in keys:
        return
    replacements = {
        "@FVM_VENDOR_SIZEOF_LONG@": str(struct.calcsize("l")),
        "@CMAKE_SIZEOF_VOID_P@": str(struct.calcsize("P")),
        "@FVM_VENDOR_PTHREAD@": "1",
    }
    for dependency in ("cairo", "pixman"):
        directory = output / dependency
        directory.mkdir(exist_ok=True)
        template = (root / "external" / dependency / "config.h.in").read_text()
        for before, after in replacements.items():
            template = template.replace(before, after)
        template = template.replace(
            "#cmakedefine FVM_VENDOR_BIG_ENDIAN 1",
            "#define FVM_VENDOR_BIG_ENDIAN 1" if sys.byteorder == "big" else "/* little endian */",
        )
        (directory / "config.h").write_text(template)
    (output / "cairo/cairo-features.h").write_text(
        (root / "external/cairo/cairo-features.h.in")
        .read_text()
        .replace("@FVM_CAIRO_IMAGE_CODECS@", "1" if "IMAGE_CODECS" in keys else "0")
        .replace("@FVM_CAIRO_FONTS@", "1" if "FONTS" in keys else "0")
    )
    template = (root / "external/pixman/upstream/pixman/pixman-version.h.in").read_text()
    for name, value in (("MAJOR", "0"), ("MINOR", "38"), ("MICRO", "4")):
        template = template.replace("@PIXMAN_VERSION_" + name + "@", value)
    (output / "pixman/pixman-version.h").write_text(template)


def cmake_sources(path, variable):
    """Use the pinned list in the vendor CMake file, avoiding globbed backends."""
    text = path.read_text()
    match = re.search(r"set\(" + variable + r"_SOURCES\s+(.*?)\)", text, re.S)
    return [path.parent / name for name in match.group(1).split()]
