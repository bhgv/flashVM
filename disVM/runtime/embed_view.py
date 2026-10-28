"""Embed an already linked SWF; this tool neither parses AS/FIR nor emits VM instructions."""
from pathlib import Path
import sys


# Copy the complete linker product into a C resource for the native launcher.
# Explicit source/destination paths; reject a missing/oversized/wrong-container product.
def main():
    source, target = (Path(value) for value in sys.argv[1:])
    data = source.read_bytes()
    if not data.startswith(b'FWS') or len(data) > 1024 * 1024:
        raise ValueError('expected a bounded SWL FWS product')
    rows = ['    ' + ', '.join('0x%02x' % value for value in data[index:index+12]) + ','
            for index in range(0, len(data), 12)]
    target.write_text('/* Generated from the linked AS -> FIR -> SWO -> SWF product. */\n'
                      'static const unsigned char ide_view_swf[] = {\n' + '\n'.join(rows) + '\n};\n')


if __name__ == '__main__':
    main()
