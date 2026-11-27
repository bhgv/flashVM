# Copyright (c) 2026 bhgv
# EN: Embed the build's native-cell launch listener as an immutable byte image.
# RU: Встроить образ приёмника запуска, собранный компилятором этой сборки.
file(READ "${INPUT}" image HEX)
string(REGEX REPLACE "(..)" "0x\\1," bytes "${image}")
if(NOT DEFINED SYMBOL)
    set(SYMBOL launch_listener_image)
endif()
file(WRITE "${OUTPUT}" "static const unsigned char ${SYMBOL}[] = { ${bytes} };\n")
