/* Private bounded linker storage. Context destruction owns every accepted copy. */
#ifndef SWL_INTERNAL_H
#define SWL_INTERNAL_H
#include "swl.h"

#define SWL_LIMIT (64u * 1024u * 1024u)
#define SWL_ITEMS 1024


/* Canonical owned object: name plus validated AVM1 ActionEnd-terminated bytes. */
typedef struct
{
    char name[256];
    unsigned char *data;
    size_t size;
    unsigned flags;
} Item;


/* Resource payload plus SWF character ID and whether PlaceObject is legal. */
typedef struct
{
    Item item;
    unsigned id;
    int displayable;
} Asset;


/* Ordered placement/sound command; coordinates are caller-supplied pixels. */
typedef struct
{
    unsigned tag, id, x, y, depth;
} Event;


struct SwlContext
{
    SwlOptions options;
    Item objects[SWL_ITEMS];
    Asset assets[SWL_ITEMS];
    Event events[SWL_ITEMS];
    size_t count, asset_count, event_count, bytes;
};


/* Growable emission buffer; failed is sticky and forbids partial publication. */
typedef struct
{
    unsigned char *data;
    size_t size, capacity;
    int failed;
} Buffer;


int swl_fail(char *, size_t, const char *);


uint32_t swl_u32(const unsigned char *);


void swl_bytes(Buffer *, const void *, size_t);


void swl_byte(Buffer *, unsigned);


void swl_word(Buffer *, unsigned);


void swl_dword(Buffer *, uint32_t);


void swl_tag(Buffer *, unsigned, const void *, size_t);


void swl_rect(Buffer *, unsigned, unsigned);


void swl_resources(SwlContext *, Buffer *);
#endif
