# Copyright (c) 2026 bhgv
# EN: One shared non-graphical wire-codec source list for standalone, VM and oracle targets.
# RU: Общий список неграфических wire-кодеков для standalone, VM и oracle.
set(FVM_PROTOCOL_SOURCES
    src/protocol/srecord.c
    src/protocol/order/prefix.c
    src/protocol/order/packet.c
    src/protocol/order/stream.c
    src/protocol/order/control.c
    src/protocol/sbindings.c
    src/protocol/sstream.c
    src/protocol/sslot.c
    src/protocol/scontrol.c
    src/protocol/smedia.c
    src/protocol/sadapt.c
    src/protocol/smeasure.c
    src/protocol/rdispatch.c
    src/protocol/rwire.c
    src/protocol/rheartbeat.c
    src/protocol/rfs.c
    src/protocol/r9p.c
)
