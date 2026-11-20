/* Copyright (c) 2026 bhgv
 * EN: Real VFS publication/grant fixture; native provider effects and lifetime counters.
 * RU: Настоящие publication/grant VFS; effects провайдера и счётчики lifetime.
 */
#include "file_provider.h"
#include "export.h"
#include "held_export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Store { char bytes[8]; int readonly; int uncertain; } Store; /* Provider-owned live object. */
typedef struct Open { Store *store; long position; } Open; /* Independent cursor per actual open. */
static int opened, closed, destroyed, writes;
extern int wirefixture(StyxBacking *backing); /* Inferno-only translation unit. */

/* EN/RU: Explicit fixture invariant; no release checks disappear.
 * Явная проверка теста, сохраняемая в release. */
static void check(int ok, const char *why)
{
	if(!ok){ fprintf(stderr, "VFS wire: %s\n", why); exit(2); }
}

/* EN/RU: Live metadata never authorizes an operation by itself.
 * Живые metadata сами не разрешают операцию. */
static FVM_Result info(void *state, FVM_FileInfo *out)
{
	Store *store = state;
	memset(out, 0, sizeof(*out));
	strcpy(out->name, "file");
	out->mode = store->readonly ? 0444 : 0666;
	out->identity.high = 0xfedcba98UL;
	out->identity.low = 0x76543210UL;
	out->length.low = 8;
	return FVM_OK;
}

/* EN/RU: Query the retained opened object rather than a path.
 * Запросить удерживаемый открытый объект, а не путь. */
static FVM_Result openinfo(void *value, FVM_FileInfo *out)
{
	return info(((Open*)value)->store, out);
}

/* EN/RU: A successful chunk must not need a metadata probe.
 * Успешный chunk не требует metadata probe. */
static void readchunk(void *value, void *bytes, size_t size, DVM_IoResult *out)
{
	Open *handle = value;
	size_t count = handle->position >= 8 ? 0 : 8-handle->position;
	if(count > size) count = size;
	memset(out, 0, sizeof(*out));
	if(count) memcpy(bytes, handle->store->bytes+handle->position, count);
	handle->position += count;
	out->count = count;
	out->status = FVM_OK;
	out->flags = DVM_IO_PREFIX;
}

/* EN/RU: Acknowledge exactly three bytes before revocation, then refuse later writes.
 * Подтвердить ровно три байта перед отзывом записи; последующие записи отклонить. */
static void writechunk(void *value, const void *bytes, size_t size, DVM_IoResult *out)
{
	Open *handle = value;
	size_t count = size > 3 ? 3 : size;
	writes++;
	memset(out, 0, sizeof(*out));
	out->flags = DVM_IO_PREFIX | (handle->store->uncertain ? DVM_IO_UNCERTAIN : 0);
	out->status = FVM_ERROR;
	out->stop = DVM_IO_DENIED;
	if(handle->store->readonly) return;
	if(handle->position < 0 || handle->position > 8 || count > 8-handle->position) return;
	memcpy(handle->store->bytes+handle->position, bytes, count);
	handle->position += count;
	out->count = count;
	handle->store->readonly = 1;
}

/* EN/RU: Full native-long positioning, independent for each open.
 * Полное native-long позиционирование отдельно для каждого открытия. */
static FVM_Result seekfile(void *value, long offset, int origin, long *position)
{
	Open *handle = value;
	if(origin != FVM_FILE_SEEK_START || offset < 0) return FVM_INVALID_ARGUMENT;
	handle->position = offset;
	*position = offset;
	return FVM_OK;
}

/* EN/RU: Consume one inner open before the factory can be destroyed.
 * Поглотить inner open до уничтожения фабрики. */
static void closefile(void *value)
{
	closed++;
	free(value);
}
static const DVM_VfsOps operations = {openinfo, NULL, NULL, seekfile, NULL, closefile, NULL, readchunk, writechunk};

/* EN/RU: The factory opens separate native cursor objects, never reuses a handle.
 * Фабрика создаёт отдельные native-курсоры, не использует handle повторно. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
	Open *value;
	if(strcmp(path, ".") || (strcmp(mode, "r") && strcmp(mode, "rw"))) return FVM_ERROR;
	value = calloc(1, sizeof(*value));
	if(!value) return FVM_OUT_OF_MEMORY;
	value->store = state;
	out->state = value;
	out->ops = &operations;
	opened++;
	return FVM_OK;
}

/* EN/RU: Copied metadata lookup is confined to the granted file root.
 * Поиск metadata ограничен корнем выданного файла. */
static FVM_Result providerinfo(void *state, const char *path, FVM_FileInfo *out)
{
	return strcmp(path, ".") ? FVM_ERROR : info(state, out);
}

/* EN/RU: Final factory release follows both publication and open-handle retirement.
 * Финальное освобождение фабрики следует за publication и открытыми handles. */
static void destroy(void *state)
{
	destroyed++;
	free(state);
}
static const DVM_ProviderOps factory = {providerinfo, openfile, destroy, 0, NULL};

/* EN/RU: Native owner explicitly admits this local fixture grant, not packet credentials.
 * Native-владелец явно разрешает локальный grant теста, а не credentials пакета. */
static int authorize(void *state, const DVM_ExportRequest *request)
{
	(void)state;
	return request->flags == 0 && !strcmp(request->name, "fixture-file");
}


/* EN/RU: Move one pre-positioned open into the TCP fixture and check exact retirement.
 * Передать TCP-тесту открытый файл с установленной позицией и проверить закрытие. */
static int held_fixture(DVM_Provider *provider)
{
    DVM_VfsHandle source = {0};
    char prefix[3];
    check(dvm_provider_open(provider, ".", "rw", &source) == FVM_OK, "source open");
    DVM_IoResult read = dvm_vfs_read_chunk(&source, prefix, sizeof(prefix));
    check(read.status == FVM_OK && read.count == sizeof(prefix) &&
          !memcmp(prefix, "abc", sizeof(prefix)), "source cursor");
    check(styxheldexport(&source, FVM_ACCESS_EXEC) == NULL && source.ops, "rejected grant consumed source");
    DVM_Provider *grant = styxheldexport(&source, FVM_ACCESS_READ);
    check(grant && !source.ops && !source.state, "move held file");
    StyxBacking *backing = styxbacking(grant);
    check(backing != NULL, "held file backing");
    dvm_provider_release(grant);
    dvm_provider_release(provider);
    int status = wirefixture(backing);
    check(status == 0 && opened == 1 && closed == 1 && destroyed == 1 && writes == 0, "held ownership/effects");
    puts("held open closed once");
    return 0;
}

/* EN/RU: Consume a real publication ticket and expose only its bounded file grant.
 * Получить настоящий ticket publication и выдать только ограниченный file grant. */
int main(int argc, char **argv)
{
	Store *store = calloc(1, sizeof(*store));
	DVM_Provider *provider, *grant = NULL;
	DVM_Publication *publication;
	DVM_ExportTicket *ticket;
	DVM_ExportOptions options;
	DVM_ExportRequest request;
	StyxBacking *backing;
	StyxOpen *read_only;
	StyxTransfer refused;
	int status;
	int uncertain = argc > 1 && !strcmp(argv[1], "uncertain");
	int readonly = argc > 1 && !strcmp(argv[1], "readonly");
	check(store != NULL, "store");
	memcpy(store->bytes, "abcdefgh", 8);
	store->uncertain = uncertain;
	provider = dvm_provider_create(&factory, store);
	check(provider != NULL, "provider");
    if (argc > 1 && (!strcmp(argv[1], "held") || !strcmp(argv[1], "held-retained")))
    {
        return held_fixture(provider);
    }
	memset(&options, 0, sizeof(options));
	options.limit = 4;
	options.direct = provider;
	options.authorize = authorize;
	publication = dvm_publication_create(&options);
	check(publication != NULL, "publication");
	memset(&request, 0, sizeof(request));
	strcpy(request.name, "fixture-file");
	strcpy(request.peer, "native-fixture-owner");
	request.kind = 1;
	request.access = readonly ? FVM_ACCESS_READ : FVM_ACCESS_READ|FVM_ACCESS_WRITE;
	ticket = dvm_publication_request(publication, &request);
	check(ticket != NULL && dvm_export_take(ticket, &grant) == 1, "real grant");
	backing = styxbacking(grant);
	check(backing != NULL, "file backing");
	dvm_export_release(ticket);
	dvm_provider_release(grant);
	dvm_publication_close(publication);
	dvm_provider_release(provider);
	read_only = styxfileopen(backing, 0);
	check(read_only != NULL, "bridge readonly open");
	styxfilewrite(read_only, 0, "bad", 3, &refused);
	check(!refused.success && refused.count == 0 && writes == 0, "bridge widened readonly open");
	styxfileclose(read_only);
	check(opened == 1 && closed == 1 && destroyed == 0, "bridge readonly lifetime");
	opened = closed = 0;
	status = wirefixture(backing);
	check(status == 0 && opened == (readonly ? 1 : 2) && closed == opened && destroyed == 1 && writes == (readonly ? 0 : uncertain ? 1 : 2), "native ownership/effects");
	printf("VFS provider wire: %d opens closed, one factory destroyed, %d writes attempted\n", closed, writes);
	return 0;
}
