/* Counted ADPCM tutorial input: three root blocks, five/zero/three exact source frames. */
#ifndef EXAMPLE_COUNTED_ADPCM_MOVIE_H /* Begin the standalone example fixture guard. */
#define EXAMPLE_COUNTED_ADPCM_MOVIE_H /* Keep fixture symbols local to one tutorial. */
#define EXAMPLE_ADPCM_BUDGET 65536UL  /* Cap peak native snapshot storage at 64 KiB. */
#define EXAMPLE_ADPCM_CHUNK 4U        /* Copy at most four mono source frames per read. */
static const unsigned char movie[] = { /* Own bytes throughout synchronous preparation. */
    /* Own bytes throughout synchronous preparation. */
    70, 87, 83, 8, 75, 0, 0, 0, 8, 0, 0, 12,      /* Continue immutable FWS8 wire bytes. */
    3, 0, 191, 4, 4, 0, 0, 0, 14, 30, 1, 0,       /* Continue immutable FWS8 wire bytes. */
    255, 4, 5, 0, 0, 0, 128, 25, 0, 127, 59, 127, /* Continue immutable FWS8 wire bytes. */
    0, 0, 0, 0, 0, 255, 4, 0, 0, 0, 0, 127,       /* Continue immutable FWS8 wire bytes. */
    0, 0, 0, 0, 0, 255, 4, 4, 0, 0, 0, 127,       /* Continue immutable FWS8 wire bytes. */
    231, 12, 124, 127, 0, 0, 0, 0, 0, 63, 0, 0,   /* Continue immutable FWS8 wire bytes. */
    0, 0, 0                                       /* Continue immutable FWS8 wire bytes. */
}; /* Close the real root SWF8 fixture. */
static const size_t block_frames[] = {5, 0, 3}; /* Authoritative fixture counts, not header advice. */
#endif                                          /* End the example-only constants and byte fixture. */
