/* A minimal SWF/SWD pair embeds one mapped throwing action block for the tutorial. */
#ifndef FVM_DEBUGGER_SOURCE_FIXTURE_H /* Keep this educational fixture local to one example. */
#define FVM_DEBUGGER_SOURCE_FIXTURE_H /* Prevent duplicate static array definitions. */

/* AVM1 integer 42, Throw, End; bytes are an independent educational wire-format fixture. */
static const unsigned char debugger_actions[] = /* Own immutable input for the corresponding memory API. */
{ /* Begin the counted raw-byte fixture. */
    0x96, 0x05, 0x00, 0x07, 0x2a, 0x00, 0x00, 0x00, /* Preserve encoded bytes 0 through 7 in this fixture. */
    0x2a, 0x00 /* Preserve encoded bytes 8 through 9 in this fixture. */
}; /* End the immutable fixture; sizeof supplies its exact byte count. */

/* FWS8 document with matching 16-byte DebugID and one DoAction; bytes are an independent educational wire-format fixture. */
static const unsigned char debugger_movie[] = /* Own immutable input for the corresponding memory API. */
{ /* Begin the counted raw-byte fixture. */
    0x46, 0x57, 0x53, 0x08, 0x30, 0x00, 0x00, 0x00, /* Preserve encoded bytes 0 through 7 in this fixture. */
    0x08, 0x00, 0x00, 0x0c, 0x01, 0x00, 0xd0, 0x0f, /* Preserve encoded bytes 8 through 15 in this fixture. */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, /* Preserve encoded bytes 16 through 23 in this fixture. */
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, /* Preserve encoded bytes 24 through 31 in this fixture. */
    0x0a, 0x03, 0x96, 0x05, 0x00, 0x07, 0x2a, 0x00, /* Preserve encoded bytes 32 through 39 in this fixture. */
    0x00, 0x00, 0x2a, 0x00, 0x40, 0x00, 0x00, 0x00 /* Preserve encoded bytes 40 through 47 in this fixture. */
}; /* End the immutable fixture; sizeof supplies its exact byte count. */

/* FWD8 DebugID, Main.as source text and a source-line offset record; bytes are an independent educational wire-format fixture. */
static const unsigned char debugger_symbols[] = /* Own immutable input for the corresponding memory API. */
{ /* Begin the counted raw-byte fixture. */
    0x46, 0x57, 0x44, 0x08, 0x03, 0x00, 0x00, 0x00, /* Preserve encoded bytes 0 through 7 in this fixture. */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, /* Preserve encoded bytes 8 through 15 in this fixture. */
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, /* Preserve encoded bytes 16 through 23 in this fixture. */
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, /* Preserve encoded bytes 24 through 31 in this fixture. */
    0x02, 0x00, 0x00, 0x00, 0x4d, 0x61, 0x69, 0x6e, /* Preserve encoded bytes 32 through 39 in this fixture. */
    0x2e, 0x61, 0x73, 0x00, 0x2f, 0x2f, 0x20, 0x53, /* Preserve encoded bytes 40 through 47 in this fixture. */
    0x6f, 0x75, 0x72, 0x63, 0x65, 0x0d, 0x0a, 0x74, /* Preserve encoded bytes 48 through 55 in this fixture. */
    0x68, 0x72, 0x6f, 0x77, 0x20, 0x34, 0x32, 0x3b, /* Preserve encoded bytes 56 through 63 in this fixture. */
    0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, /* Preserve encoded bytes 64 through 71 in this fixture. */
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x22, 0x00, /* Preserve encoded bytes 72 through 79 in this fixture. */
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff /* Preserve encoded bytes 80 through 85 in this fixture. */
}; /* End the immutable fixture; sizeof supplies its exact byte count. */
#endif /* End the source tutorial fixture guard. */
