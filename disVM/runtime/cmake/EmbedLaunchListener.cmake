# Copyright (c) 2026 bhgv
# EN: Embed the build's native-cell launch listener as an immutable byte image.
# RU: Встроить образ приёмника запуска, собранный компилятором этой сборки.
file(READ "${INPUT}" image HEX)
string(REGEX REPLACE "(..)" "0x\\1," bytes "${image}")
file(WRITE "${OUTPUT}" "static const unsigned char launch_listener_image[] = { ${bytes} };\n")
