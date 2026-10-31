#!/bin/sh
# Execute the installed, linked AS/SWF example against a separately compiled C module.
# Paths derive only from this package; propagate the real program status and literal argv.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$here/disvm_flashvm_native_module_example" "$here/examples/flashvm-native-module/native-module.swf" "$@"
