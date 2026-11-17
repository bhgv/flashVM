/* Copyright (c) 2026 bhgv
 * EN: Copied launch source identity, descriptive only and never an authority token.
 * RU: Копируемая identity источника запуска, только описание, не authority token.
 */
#ifndef DVM_LAUNCH_ORIGIN_H
#define DVM_LAUNCH_ORIGIN_H
#define DVM_ORIGIN_ID_BYTES 44U /* Canonical unpadded base64url of32 bytes plus NUL. */
#define DVM_ORIGIN_NICK_BYTES 128U /* Bounded display nickname including NUL. */
#define DVM_ORIGIN_TERMINAL 1U /* Resource supplied by terminal in this launch. */
#define DVM_ORIGIN_PROVIDER 2U /* Resource supplied by execution provider in this launch. */


/* EN/RU: Runtime/transport copies authenticated source association; strings alone confer no rights.
 * Runtime/transport копирует связь source; сами строки не предоставляют прав. */
typedef struct DVM_Origin
{
    unsigned role; /* Launch-relative terminal/provider role, zero for unannotated routes. */
    char system[DVM_ORIGIN_ID_BYTES]; /* Persistent installation identity. */
    char export_id[DVM_ORIGIN_ID_BYTES]; /* Concrete granted VFS root identity. */
    char session[DVM_ORIGIN_ID_BYTES]; /* Common launch-session identity, distinct from provider journal epoch. */
    char nickname[DVM_ORIGIN_NICK_BYTES]; /* Display text only, UTF-8 supplied by trusted adapter. */
} DVM_Origin;


/* EN/RU: Check borrowed ID's exact canonical shape without reading beyond capacity.
 * Caller owns44 readable bytes; return1/0. Проверить canonical ID в44 bytes, вернуть1/0. */
static inline int dvm_origin_id_valid(const char *id)
{
    unsigned i;
    if (!id)
    {
        return 0;
    }
    for (i = 0; i < 43; i++)
    {
        unsigned char c = (unsigned char)id[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_'))
        {
            return 0;
        }
    }
    if (id[43])
    {
        return 0;
    }
    const char *allowed = "AEIMQUYcgkosw048";
    for (i = 0; allowed[i]; i++)
    {
        if (id[42] == allowed[i])
        {
            return 1;
        }
    }
    return 0;
}
#endif
