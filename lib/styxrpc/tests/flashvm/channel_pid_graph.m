# Copyright (c) 2026 bhgv
# EN: Compiled result schema for a reverse PID carrying a graph of live queues.
# RU: Компилированная схема результата для встречного PID с графом живых очередей.
Pidgraph: module {
    open: fn(): (chan of chan of chan of chan of int, chan of int);
};
