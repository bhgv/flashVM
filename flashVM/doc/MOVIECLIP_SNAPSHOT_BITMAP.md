# MovieClip.snapshotBitmap

[Русский](MOVIECLIP_SNAPSHOT_BITMAP.ru.md) · [Paired desktop](../../ide/DESKTOP-PAIRED.md)

`clip.snapshotBitmap(width, height, scaleX, scaleY, translateX, translateY)`
returns an independent BitmapData raster of the receiver in clip-local
coordinates. The caller can attach it at a normal display depth or keep it
as a pixel snapshot. The paired IDE uses it to render document thumbnails
at shelf and map sizes.

Width and height are whole pixels from 1 to 2880. The scales are nonzero
finite numbers and the translations are finite destination pixels.
A local point `(x, y)` maps to
`(x * scaleX + translateX, y * scaleY + translateY)`.
The output starts transparent and clips to the requested raster.
The root is rendered in its own coordinates, even if hidden on stage.
Descendants keep their local transforms, visibility and paint. The operation
uses the same owner-thread renderer as BitmapData drawing.

Pass exactly six arguments. Invalid receivers, sizes and transforms produce
an argument error. Allocation and raster errors propagate. A player compiled
without AGG or Cairo returns undefined after validating the arguments.
The returned resource owns its pixels. Later edits to the source clip take
effect in the next snapshot. Drawing and allocation happen synchronously
on the FlashVM owner and can run ordinary drawing getters.

The miniature renderer fits the width and height in ActionScript and uses
the returned bitmap with `attachBitmap`. Render a new copy after a committed
scene before replacing the displayed miniature.

[Native implementation](../src/standard/movieclip_snapshot.inc) ·
[IDE use](../../ide/as/desktop-paired-mirror-pixels.as)
