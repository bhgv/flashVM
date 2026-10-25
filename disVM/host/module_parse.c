/* Bind transport-independent images to Inferno's retained production parser. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "disvm/module_image.h"


/* Synthetic sources have no filesystem Qid. Name is their logical identity;
 * namespace/cache policy belongs to the embedding host, outside parsemod. */
DVM_ImageResult dvm_image_parse(const DVM_ModuleImage *image, DVM_Module **out)
{
    Dir metadata;
    Module *module;
    if (!image || !out || !image->name || !*image->name || !image->bytes || !image->size)
    {
        return DVM_IMAGE_INVALID;
    }
    if (image->size > DVM_MODULE_IMAGE_MAX)
    {
        return DVM_IMAGE_LIMIT;
    }
    memset(&metadata, 0, sizeof(metadata));
    module = parsemod(image->name, image->bytes, (ulong)image->size, &metadata);
    if (!module)
    {
        return DVM_IMAGE_BAD_MODULE;
    }
    /* [GC:] parsemod publishes into Dis's module list only after full validation.
     * Never transfer this native handle as a FlashVM object/channel value. */
    *out = module;
    return DVM_IMAGE_OK;
}
