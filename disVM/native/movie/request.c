/* Copyright (c) 2026 bhgv
 * EN: Resolve actual request purpose in the granted live VFS and open exactly once.
 * RU: Разрешить назначение запроса в granted live VFS и открыть ровно один раз.
 */
#include "service_internal.h"
#include "request_payload.h"


/* EN/RU: Join a borrowed raw reference to its virtual movie base;1/0 with bounded output.
 * Absolute references remain inside the namespace; normalization happens in namespace planning.
 * Соединить raw reference с virtual movie base;1/0, normalization выполняет namespace. */
static int pathjoin(const char *reference, const char *base, char *output)
{
    const char *slash = base ? strrchr(base, '/') : NULL;
    size_t prefix = reference[0] == '/' || !slash ? 0 : (size_t)(slash-base)+1;
    size_t length = strlen(reference);
    if (!length || prefix+length >= MOVIE_PATH)
    {
        return 0;
    }
    if (prefix)
    {
        memcpy(output, base, prefix);
    }
    memcpy(output+prefix, reference, length+1);
    return 1;
}


/* EN/RU: See service_internal.h; errors become explicit terminal failure, never local fallback.
 * См. service_internal.h; ошибки дают явный terminal failure, без local fallback. */
int movie_request(MovieOpen *opened, const unsigned char *bytes, size_t size)
{
    char reference[MOVIE_PATH], base[MOVIE_PATH], path[MOVIE_PATH];
    DVM_ResourcePlan plan;
    FVM_FileInfo info;
    FVM_PortFile *file = NULL;
    FVM_PortDirectory *directory = NULL;
    FVM_Result status;
    unsigned purpose; /* Validated explicit wire intent, never inferred from filename. */
    if (!movie_request_parse(bytes, size, &purpose, reference, base))
    {
        return 0;
    }
    opened->stage = MOVIE_END;
    opened->outcome = 1;
    if (!pathjoin(reference, *base ? base : NULL, path) ||
        !dvm_resource_plan_intent(opened->group, path,
            purpose == MOVIE_CODE ? DVM_RESOURCE_SWF : DVM_RESOURCE_DATA, &plan))
    {
        return 1;
    }
    plan.requested_access = FVM_ACCESS_READ;
    status = dvm_resource_info(&plan, &info);
    if (status == FVM_OK && !info.directory)
    {
        status = plan.provider ? dvm_provider_open(plan.provider, plan.relative, "r", &opened->resource) :
            dvm_resource_open(&plan, "r", &file, &directory);
        if (status == FVM_OK && !plan.provider && !dvm_vfs_adopt(&opened->resource, file, directory))
        {
            fvm_port_file_close(file);
            fvm_port_directory_close(directory);
            status = FVM_ERROR;
        }
        if (status == FVM_OK)
        {
            strcpy(opened->origin, plan.names.path);
            opened->stage = MOVIE_BEGIN;
            opened->outcome = 0;
        }
    }
    dvm_resource_close(&plan);
    return 1;
}
