/* Copyright (c) 2026 bhgv
 * EN: Encode paired module identity without native pointers or mutable path lookups.
 * RU: Кодирование парной идентичности модуля без указателей и поиска нового пути.
 */
#include "styxrpc/module_ref.h"
#include "styxrpc/serve.h"
#include <string.h>

#define REF_VERSION 1U /* This tagged record format. */
#define REF_FLAGS 3U /* Byte for the two defined flag bits. */
#define REF_WRITE 1U /* Writable opening from the issuer. */
#define REF_REVERSE 2U /* Origin is the reverse player. */
#define REF_QID 4U /* First of eight network-order identity bytes. */
#define REF_SCOPE 12U /* First byte of the pair nonce. */


/* EN: Check one counted module or variable component.
 * RU: Проверить один компонент имени модуля либо переменной. */
static int component(const char *name, size_t length)
{
    return length && length <= STYXRPC_TREE_NAME &&
        !(length == 1 && name[0] == '.') &&
        !(length == 2 && name[0] == '.' && name[1] == '.');
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
int styxrpc_module_ref_path(const char *path)
{
    static const char root[] = "mounted/"; /* Exported module image root. */
    static const char var[] = "/.var/"; /* Scope of native Int32 exports. */
    if (!path || strncmp(path, root, sizeof(root) - 1))
    {
        return 0;
    }
    const char *image = path + sizeof(root) - 1;
    const char *separator = strchr(image, '/');
    if (!separator || !component(image, (size_t)(separator - image)) ||
        strncmp(separator, var, sizeof(var) - 1))
    {
        return 0;
    }
    const char *member = separator + sizeof(var) - 1;
    return !strchr(member, '/') && component(member, strlen(member));
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
size_t styxrpc_module_ref_encode(const StyxRpcModuleRef *ref, const void *scope, void *bytes, size_t capacity)
{
    if (!ref || !scope || !bytes || !ref->node || ref->owner > STYXRPC_MODULE_REF_REVERSE ||
        (ref->writable != 0 && ref->writable != 1) ||
        !memchr(ref->path, 0, sizeof(ref->path)) || !styxrpc_module_ref_path(ref->path))
    {
        return 0;
    }
    size_t length = strlen(ref->path);
    if (capacity < STYXRPC_MODULE_REF_HEADER + length)
    {
        return 0;
    }
    unsigned char *frame = bytes;
    frame[0] = 'M';
    frame[1] = 'V';
    frame[2] = REF_VERSION;
    frame[REF_FLAGS] = (ref->writable ? REF_WRITE : 0) |
        (ref->owner == STYXRPC_MODULE_REF_REVERSE ? REF_REVERSE : 0);
    uint64_t node = ref->node;
    for (size_t i = sizeof(node); i; --i)
    {
        frame[REF_QID + i - 1] = (unsigned char)node;
        node >>= 8;
    }
    memcpy(frame + REF_SCOPE, scope, STYXRPC_MODULE_REF_SCOPE_SIZE);
    memcpy(frame + STYXRPC_MODULE_REF_HEADER, ref->path, length);
    return STYXRPC_MODULE_REF_HEADER + length;
}


/* EN/RU: See module_ref.h. / См. module_ref.h. */
int styxrpc_module_ref_decode(const void *bytes, size_t size, const void *scope, StyxRpcModuleRef *out)
{
    const unsigned char *frame = bytes;
    if (!frame || !scope || !out || size <= STYXRPC_MODULE_REF_HEADER || size > STYXRPC_MODULE_REF_WIRE_MAX ||
        frame[0] != 'M' || frame[1] != 'V' || frame[2] != REF_VERSION ||
        (frame[REF_FLAGS] & ~(REF_REVERSE | REF_WRITE)) ||
        memcmp(frame + REF_SCOPE, scope, STYXRPC_MODULE_REF_SCOPE_SIZE))
    {
        return 0;
    }
    StyxRpcModuleRef ref = {0};
    size_t length = size - STYXRPC_MODULE_REF_HEADER;
    memcpy(ref.path, frame + STYXRPC_MODULE_REF_HEADER, length);
    if (memchr(ref.path, 0, length) || !styxrpc_module_ref_path(ref.path))
    {
        return 0;
    }
    for (size_t i = 0; i < sizeof(ref.node); ++i)
    {
        ref.node = (ref.node << 8) | frame[REF_QID + i];
    }
    if (!ref.node)
    {
        return 0;
    }
    ref.owner = frame[REF_FLAGS] & REF_REVERSE ? STYXRPC_MODULE_REF_REVERSE : STYXRPC_MODULE_REF_FORWARD;
    ref.writable = (frame[REF_FLAGS] & REF_WRITE) != 0;
    *out = ref;
    return 1;
}
