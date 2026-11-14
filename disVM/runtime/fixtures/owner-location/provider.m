# Copyright (c) 2026 bhgv
# EN: Typed channel boundary to an independently loaded module under inspection.
# RU: Типизированная channel boundary к отдельно загружаемому исследуемому модулю.
LocationProvider: module
{

    # EN/RU: Block on gate, preserving actual caller/Module roots until released.
    # Ждать gate, сохраняя actual caller/Module roots до release.
    wait: fn(ready, gate: chan of int): int;
};
