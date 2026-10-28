"""Relink the copied Limbo SDK guides to shipped headers or public repository sources."""
from pathlib import Path
import os
import re
import sys
from urllib.parse import quote


# root/package are explicit source/install directories. Modify copied guides only;
# shipped targets link locally, omitted source files link to GitHub. No network access.
def main():
    root, package = (Path(value).resolve() for value in sys.argv[1:])
    frontend = root / 'compiler/limbo/frontend'
    pairs = {frontend / name: package / name
             for name in ('ALIASES.md', 'ALIASES.ru.md', 'NATIVE_C.md', 'NATIVE_C.ru.md')}
    for source in frontend.glob('*.h'):
        target = package / 'include' / source.name
        if target.is_file():
            pairs[source] = target
    for source, target in pairs.items():
        if source.suffix != '.md' or not target.is_file():
            continue

        # Resolve each original Markdown destination before selecting an installed counterpart.
        # Unchanged web/anchor links and fragments retain their original meaning.
        def destination(match):
            value = match[1]
            if ':' in value or value.startswith('#'):
                return match[0]
            path, separator, fragment = value.partition('#')
            original = (source.parent / path).resolve()
            installed = pairs.get(original)
            if installed is not None and installed.is_file():
                link = Path(os.path.relpath(installed, target.parent)).as_posix()
            else:
                try:
                    relative = original.relative_to(root)
                except ValueError:
                    return match[0]
                if not original.is_file():
                    return match[0]
                link = 'https://github.com/bhgv/flashVM/blob/master/' + quote(relative.as_posix())
            return '](' + link + (separator + fragment if separator else '') + ')'

        target.write_text(re.sub(r'\]\(([^)]+)\)', destination, source.read_text()))


if __name__ == '__main__':
    main()
