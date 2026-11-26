# phybidircreate_lanes

[Русский](phybidircreate_lanes.ru.md) · [Header](../../remote_phy/mux/bidir.h#L34) · [Implementation](../../remote_phy/mux/life.c) · [Framing](../../remote_phy/mux/pump.c)

```c
#include <mux/bidir.h>

PhyBidir *phybidircreate_lanes(RemotePhy *carrier, size_t capacity, unsigned lanes);
```

Consume the connected, reliable and ordered byte `carrier` on every
outcome. `capacity` sets the byte bound of each queue in each direction
and ranges from 1024 to 1048576. `lanes` selects two or three entries
numbered from zero. The initial pair is available immediately. The extra
lane grants receive credit only when acquired with `phybidirchannel`.
Use an application agreement before taking it. The two-lane profile
and its on-wire frame format are available through `phybidircreate`.

Return an owned mux, or NULL for invalid input, allocation or native
worker failure. The caller owns lane acquisition and calls `phybidirfree`
after dispatchers retire. The native worker moves bytes independently
of the consumers. Lane handles retain the underlying lifetime through
their own retirement. Link `Styx::bidir` in the hosted build.
