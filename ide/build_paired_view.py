# Copyright (c) 2026 bhgv
"""Link the paired desktop presentation. / Собрать представление парного desktop."""

import argparse
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

SOURCES = Path(__file__).resolve().with_name("as")  # EN/RU: Application sources / исходники приложения.


# EN: Lower each AS module independently, link and replace output only after success.
# RU: Собрать каждый модуль AS, связать и заменить выход только после успешной сборки.
def build_movie(output, as2, fir, swl, observers=()):
    output = output.resolve()
    parts = ("word", "layers", "placement", "group", "pick-layer", "pick", "resize", "pointer", "pointer-queue",
             "poll", "text", "source", "mirror-source", "mirror-card", "mirror-pixels", "mirror", "atlas",
             "compose", "cull", "item", "vector", "vector-fragment", "vector-decode", "vector-path",
             "vector-node", "vector-cubic", "vector-finish", "vector-tools", "vector-button", "vector-input",
             "vector-pointer", "vector-origin", "vector-unproject", "ink-style", "ink-input",
             "ink-start", "ink-paint", "ink-dot", "ink-sample", "ink-cancel", "ink-present", "ink-release", "ink-send",
             "ink-next", "begin", "end", "terminal", "view")
    sources = tuple(SOURCES / f"desktop-paired-{part}.as" for part in parts) + tuple(observers)
    with TemporaryDirectory(prefix="desktop-view-", dir=output.parent) as temporary:
        work = Path(temporary)
        objects = []
        for index, source in enumerate(sources):
            ir = work / f"{index}.fir"
            optimized = work / f"{index}.opt.fir"
            obj = work / f"{index}.swo"
            for command in ((as2, source, ir), (fir / "fir1", "check", ir),
                            (fir / "fir1_opt", ir, optimized), (fir / "fir1", "object", optimized, obj)):
                subprocess.run(command, check=True, timeout=20)
            objects.append(obj)
        movie = work / "desktop-view.swf"
        subprocess.run((swl, "link", movie, *objects, "--size", "960", "640", "--fps", "60"),
                       check=True, timeout=20)
        movie.replace(output)
    return output


# EN: Parse paths to explicit project tools. / RU: Принять пути к инструментам проекта.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    for name in ("as2", "fir", "swl"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    args = parser.parse_args()
    build_movie(args.output, args.as2, args.fir, args.swl)


if __name__ == "__main__":
    main()
