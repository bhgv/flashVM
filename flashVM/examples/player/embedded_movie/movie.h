/* Embedded SWF8 fixture; the same bytes as ../muted_stream/muted-stream.swf. */
#ifndef EXAMPLE_EMBEDDED_MOVIE_H /* Guard this private tutorial data. */
#define EXAMPLE_EMBEDDED_MOVIE_H /* No runtime file access is required. */
static const unsigned char embedded_movie[] = { /* Immutable binary, not a C string. */
    /* Immutable binary, not a C string. */
    0x46, 0x57, 0x53, 0x08, 0x2c, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0xc8, /* SWF bytes 0..11. */
    0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x18, 0x01, 0x00, 0x43, 0x02, 0x30, /* SWF bytes 12..23. */
    0x50, 0x70, 0x44, 0x0b, 0x0e, 0x3e, 0x03, 0x00, 0xc6, 0x04, 0x00, 0x00, /* SWF bytes 24..35. */
    0x00, 0x10, 0x00, 0xf0, 0x40, 0x00, 0x00, 0x00,                         /* SWF bytes 36..43. */
}; /* sizeof includes every SWF byte; there is no added NUL terminator. */
#endif /* End the private embedded resource. */
