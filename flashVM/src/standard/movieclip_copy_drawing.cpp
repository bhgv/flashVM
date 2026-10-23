/* FlashVM: deep-copy editable native drawing state while preserving shared bitmap resources. */
#include "movieclip_construction.hpp"
#include "movieclip_storage.hpp"
#include "flashvm/serialization.h"
using namespace fvm_movieclip_storage;
/* [GC:] The caller retains source/destination and blocks collection during temporary graph setup.
 */
FVM_Result fvm_movieclip_copy_drawing(FVM_Context *c, FVM_Object *source, FVM_Object *destination)
{
    FVM_Object *bundle = 0;
    FVM_Result r = fvm_object_create(c, &bundle);
    if (r != FVM_OK)
        return r;
    FVM_Value paths, active;
    r = field(source, "paths", paths);
    if (r == FVM_OK)
        r = field(source, "active", active);
    if (r != FVM_OK)
        return r;
    /* [GC:] Preserve live bitmap edges separately; duplication clones geometry, not its texture. */
    FVM_Object *original_paths = paths.object;
    bool bitmap_paths = false;
    for (Slots::iterator i = original_paths->indexed.begin(); i != original_paths->indexed.end();
         ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        if (i->second.object->properties.find("bitmap") != i->second.object->properties.end())
            bitmap_paths = true;
    }
    if (bitmap_paths)
    {
        /* [GC:] The native barrier protects these stripped paint copies until the bundle adopts
         * them. */
        FVM_Object *table = 0;
        /* [GC:] Allocate an independent path table without serializing shared bitmap resources. */
        r = fvm_object_create(c, &table);
        if (r != FVM_OK)
            return r;
        FVM_Value mapped_active = object(0);
        for (Slots::iterator i = original_paths->indexed.begin();
             i != original_paths->indexed.end(); ++i)
        {
            /* [GC:] Copy immutable paint metadata while retaining command edges for graph cloning.
             */
            FVM_Object *part = 0;
            /* [GC:] Allocate a temporary paint record protected by the native barrier. */
            r = fvm_object_create(c, &part);
            if (r != FVM_OK)
                return r;
            /* [GC:] Commands/gradients will be deep-copied together with the active-path alias. */
            part->properties = i->second.object->properties;
            /* [GC:] Exclude the live texture descriptor from the independent geometry snapshot. */
            part->properties.erase("bitmap");
            /* [GC:] The temporary table owns this stripped paint record. */
            table->indexed[i->first] = object(part);
            if (active.type == FVM_VALUE_OBJECT && active.object == i->second.object)
                /* [GC:] Map the active path to the same stripped record held by the table. */
                mapped_active = object(part);
        }
        /* [GC:] The bundle adopts the stripped graph; original bitmap edges remain in the source.
         */
        paths = object(table);
        /* [GC:] Preserve the active alias without sharing any editable command table. */
        active = mapped_active;
    }
    /* [GC:] One snapshot preserves aliases between the path table and its active path. */
    bundle->properties["paths"] = paths;
    bundle->properties["active"] = active;
    if (source->properties.find("gradient") != source->properties.end())
    {
        /* [GC:] Clone the current ramp together with its aliases in active/completed paths. */
        bundle->properties["gradient"] = source->properties["gradient"];
    }
    if (source->properties.find("lineGradient") != source->properties.end())
    {
        /* [GC:] Clone both current ramps in the same graph to preserve cross-path aliases. */
        bundle->properties["lineGradient"] = source->properties["lineGradient"];
    }
    FVM_Value value = object(bundle), restored;
    unsigned char *bytes = 0;
    size_t size = 0;
    r = fvm_value_serialize(c, &value, &bytes, &size);
    if (r != FVM_OK)
        return r;
    r = fvm_value_deserialize(c, bytes, size, &restored);
    fvm_serialized_free(bytes);
    if (r != FVM_OK)
        return r;
    r = field(restored.object, "paths", paths);
    if (r == FVM_OK)
        r = field(restored.object, "active", active);
    if (r != FVM_OK)
        return r;
    /* [GC:] The new instance owns the cloned geometry and the matching active-path edge. */
    destination->properties["paths"] = paths;
    destination->properties["active"] = active;
    if (restored.object->properties.find("gradient") != restored.object->properties.end())
    {
        /* [GC:] Adopt the cloned current ramp rather than retaining the source's descriptor. */
        destination->properties["gradient"] = restored.object->properties["gradient"];
    }
    if (restored.object->properties.find("lineGradient") != restored.object->properties.end())
    {
        /* [GC:] Future duplicate paths adopt their own copied stroke descriptor. */
        destination->properties["lineGradient"] = restored.object->properties["lineGradient"];
    }
    if (bitmap_paths)
    {
        for (Slots::iterator i = original_paths->indexed.begin();
             i != original_paths->indexed.end(); ++i)
        {
            Properties::iterator bitmap = i->second.object->properties.find("bitmap");
            if (bitmap != i->second.object->properties.end())
            {
                Slots::iterator copied = paths.object->indexed.find(i->first);
                if (copied == paths.object->indexed.end() ||
                    copied->second.type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                /* [GC:] Both clips retain the original live bitmap through the immutable
                 * descriptor. */
                copied->second.object->properties["bitmap"] = bitmap->second;
            }
        }
    }
    Properties::iterator current_bitmap = source->properties.find("bitmap");
    if (current_bitmap != source->properties.end())
        /* [GC:] Future paths in the duplicate retain the same current texture descriptor. */
        destination->properties["bitmap"] = current_bitmap->second;
    const char *styles[] = {"cx",     "cy",    "fill",    "rgb",      "alpha",
                            "stroke", "width", "lineRgb", "lineAlpha"};
    for (size_t i = 0; i < sizeof(styles) / sizeof(styles[0]); ++i)
    {
        r = field(source, styles[i], value);
        if (r != FVM_OK)
            return r;
        destination->properties[styles[i]] = value;
    }
    return FVM_OK;
}
