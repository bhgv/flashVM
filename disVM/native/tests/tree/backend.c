/* Copyright (c) 2026 bhgv
 * EN: Prepared live tree publication, captured handle metadata and directory cursor oracle.
 * RU: Живая tree-publication, metadata захваченных handles и проверка курсоров каталога.
 */
#include "file_provider.h"
#include "tree.h"
#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct State { int hide, added, denied, replaced, bad, opaque; } State;
typedef struct Open { State *state; int index, listed, failed; size_t position; FVM_FileInfo captured; } Open;
static const char *paths[] = {".", "visible", "secret", "nest", "nest/inner", "later"};
static State *state;
static int opened, closed, destroyed, escaped;
extern int treewire(StyxBacking *root);

/* EN/RU: Keep all native fixture checks active in release builds.
 * Сохранить все native-проверки в release. */
static void check(int ok, const char *why)
{
	if(!ok){ fprintf(stderr, "tree bridge: %s\n", why); exit(2); }
}

/* EN/RU: Explicit native controls, not remote credentials or host path operations.
 * Явное native-управление, не credentials и не операции host path. */
void treechange(int action)
{
	if(action == 1) state->hide = 1;
	if(action == 2) state->hide = 0;
	if(action == 3) state->added = 1;
	if(action == 4) state->denied = 1;
	if(action == 5) state->denied = 0;
	if(action == 6) state->replaced = 1;
}

/* EN/RU: Resolve only the native fixture's relative namespace.
 * Разрешить только относительное пространство имён теста. */
static int indexof(const char *path)
{
	int i;
	if(path[0] == '/' || strstr(path, "..")) escaped++;
	for(i = 0; i < 6; i++) if(!strcmp(paths[i], path)) return i;
	return -1;
}

/* EN/RU: Return current attributes without treating mode as an access grant.
 * Вернуть текущие атрибуты, не превращая mode в полномочия. */
static void attributes(State *value, int index, FVM_FileInfo *out)
{
	const char *name = strrchr(paths[index], '/');
	memset(out, 0, sizeof(*out));
	strcpy(out->name, name ? name+1 : paths[index]);
	out->identity.low = index == 0 && value->replaced ? 9 : index+1;
	out->device.low = 99;
	out->directory = index == 0 || index == 3;
	out->mode = out->directory ? 0555 : index == 1 && value->added ? 0400 : 0444;
	out->length.low = 8;
}

/* EN/RU: Metadata visibility of the backing namespace; the view filters it further.
 * Видимость backing namespace; view дополнительно её ограничивает. */
static FVM_Result info(void *value, const char *path, FVM_FileInfo *out)
{
	int index = indexof(path);
	State *store = value;
	if(index < 0 || (index == 5 && !store->added)) return FVM_NOT_FOUND;
	attributes(store, index, out);
	return FVM_OK;
}

/* EN/RU: Real effective-access callback, independently revocable while mode stays0555.
 * Настоящий effective-access callback с отзывом при неизменном mode0555. */
static FVM_Result accessnode(void *value, const char *path, unsigned access)
{
	State *store = value;
	int index = indexof(path);
	if(index < 0 || (access&FVM_ACCESS_WRITE) || (store->denied && (index == 0 || index == 3))) return FVM_ERROR;
	return FVM_OK;
}

/* EN/RU: Captured object identity survives a path being replaced.
 * Захваченная identity переживает подмену пути. */
static FVM_Result openinfo(void *value, FVM_FileInfo *out)
{
	Open *handle = value;
	attributes(handle->state, handle->index, out);
	out->identity = handle->captured.identity;
	out->device = handle->captured.device;
	return FVM_OK;
}

/* EN/RU: Read only from the retained native object; the view enforces current policy.
 * Читать удержанный объект; текущую политику обеспечивает view. */
static void readchunk(void *value, void *bytes, size_t size, DVM_IoResult *out)
{
	Open *handle = value;
	const char *text = handle->index == 4 ? "inside!!" : "old-data";
	size_t n = handle->position >= 8 ? 0 : 8-handle->position;
	if(n > size) n = size;
	memset(out, 0, sizeof(*out));
	if(n) memcpy(bytes, text+handle->position, n);
	handle->position += n;
	out->count = n;
	out->status = FVM_OK;
	out->flags = DVM_IO_PREFIX;
}

/* EN/RU: Position an ordinary file; directory restart uses a new checked cursor.
 * Позиционировать файл; каталог перезапускается новым проверенным курсором. */
static FVM_Result seekfile(void *value, long offset, int origin, long *position)
{
	Open *handle = value;
	if(origin != FVM_FILE_SEEK_START || offset < 0) return FVM_ERROR;
	handle->position = offset;
	*position = offset;
	return FVM_OK;
}

/* EN/RU: One current batch; metadata can change again before adapter peek.
 * Одна текущая batch; metadata могут снова измениться до peek адаптера. */
static FVM_Result list(void *value, FVM_FileInfo **entries, size_t *count)
{
	Open *handle = value;
	int i, indices[4] = {1, 2, 3, 5};
	*entries = NULL; *count = 0;
	if(handle->failed || handle->state->denied) return FVM_ERROR;
	if(handle->listed) return FVM_OK;
	handle->listed = 1;
	*count = handle->index == 3 ? 1 : handle->state->added ? 4 : 3;
	*entries = calloc(*count, sizeof(**entries));
	if(!*entries) return FVM_OUT_OF_MEMORY;
	for(i = 0; i < (int)*count; i++) attributes(handle->state, handle->index == 3 ? 4 : indices[i], &(*entries)[i]);
	if(handle->state->bad) strcpy((*entries)[0].name, "../escape");
	return FVM_OK;
}

/* EN/RU: Native cursor failure cannot masquerade as a fresh EOF.
 * Отказ native-курсора не превращается в свежий EOF. */
static void abortlist(void *value) { ((Open*)value)->failed = 1; }
/* EN/RU: Consume one opened object and count exact retirement.
 * Поглотить одно открытие и учесть retirement. */
static void closefile(void *value) { closed++; free(value); }
static const DVM_VfsOps fileops = {openinfo, NULL, NULL, seekfile, NULL, closefile, NULL, readchunk, NULL};
static const DVM_VfsOps opaqueops = {NULL, NULL, NULL, seekfile, NULL, closefile, NULL, readchunk, NULL};
static const DVM_VfsOps dirops = {openinfo, NULL, NULL, NULL, list, closefile, abortlist};

/* EN/RU: Recheck operation access and capture the actual object identity at open.
 * Перепроверить доступ и захватить identity настоящего объекта при open. */
static FVM_Result openfile(void *value, const char *path, const char *mode, DVM_VfsHandle *out)
{
	Open *handle;
	FVM_FileInfo meta;
	State *store = value;
	if(strcmp(mode, "r") || info(store, path, &meta) != FVM_OK ||
		accessnode(store, path, FVM_ACCESS_READ|(meta.directory ? FVM_ACCESS_EXEC : 0)) != FVM_OK) return FVM_ERROR;
	handle = calloc(1, sizeof(*handle));
	if(!handle) return FVM_OUT_OF_MEMORY;
	handle->state = store; handle->index = indexof(path); handle->captured = meta;
	out->ops = meta.directory ? &dirops : store->opaque ? &opaqueops : &fileops; out->state = handle;
	opened++;
	return FVM_OK;
}
/* EN/RU: Last provider owner follows all node capabilities and opened handles.
 * Последний владелец provider следует за node-capabilities и открытыми handles. */
static void destroy(void *value) { destroyed++; free(value); state = NULL; }
static const DVM_ProviderOps providerops = {info, openfile, destroy, 0, accessnode};

/* EN/RU: This prepared view hides private nodes dynamically, including held opens.
 * Prepared view динамически скрывает private-ноды, включая удержанные открытия. */
static int policy(void *value, const char *path, unsigned access)
{
	State *store = value;
	(void)access;
	return strcmp(path, "/secret") && !(store->hide && (!strcmp(path, "/visible") || !strncmp(path, "/nest", 5)));
}
/* EN/RU: Native owner prepares a live mapping, not a serialized directory tree.
 * Native-владелец готовит живой mapping, не сериализованное дерево. */
static DVM_Provider *prepared(DVM_Provider *provider)
{
	DVM_View *view = dvm_view_create(policy, state, NULL);
	check(view && dvm_view_map(view, "/", provider, ".", FVM_ACCESS_READ|FVM_ACCESS_EXEC, DVM_VIEW_TREE), "prepare mapping");
	return dvm_view_freeze(view);
}
/* EN/RU: Admit only the native local fixture ticket, never network-supplied identity.
 * Разрешить только локальный native-ticket, не сетевую identity. */
static int authorize(void *value, const DVM_ExportRequest *request) { (void)value; return request->flags == 0; }

/* EN/RU: Validate raw cursor failure, confined walks and captured view metadata before TCP.
 * Проверить отказы курсора, границы обхода и захваченные metadata до TCP. */
static void cases(DVM_Provider *base)
{
	StyxBacking *root = styxtree(base), *parent, *leaf;
	StyxOpen *directory, *file;
	StyxTransfer result;
	char bytes[8];
	StyxMeta meta;
	DVM_Provider *view;
	check(root != NULL, "raw root");
	check(!styxwalknode(root, "../../outside") && !styxwalknode(root, "/outside"), "unsafe component accepted");
	parent = styxwalknode(root, "..");
	check(parent && styxbackinginfo(parent, &meta) && meta.identity == 1, "root dotdot escaped");
	styxbackingfree(parent);
	directory = styxfileopen(root, 0);
	check(directory && styxdirbegin(directory, 0), "raw dir open");
	state->bad = 1;
	check(styxdirpeek(directory, &meta) < 0 && !styxdiradvance(directory, 1), "unsafe entry accepted");
	state->bad = 0;
	check(styxdirbegin(directory, 0) && styxdirpeek(directory, &meta) == 1 && !strcmp(meta.name, "visible"), "cursor recovery");
	styxfileclose(directory); styxbackingfree(root);
	view = prepared(base); check(view != NULL, "prepared view");
	root = styxtree(view); directory = styxfileopen(root, 0);
	check(root && directory, "prepared directory");
	state->replaced = 1;
	check(!styxbackinginfo(root, &meta), "node rebound silently");
	check(styxfileinfo(directory, &meta) && meta.identity == 1, "view info reopened replaced path");
	check(!styxdirbegin(directory, 0), "rewind rebound opened directory");
	state->replaced = 0;
	check(styxdirbegin(directory, 0), "identity recovery");
	styxfileclose(directory);
	leaf = styxwalknode(root, "visible");
	check(leaf != NULL, "opaque leaf");
	state->opaque = 1;
	file = styxfileopen(leaf, 0);
	check(file && !styxfileinfo(file, &meta), "opaque endpoint fabricated handle stat");
	styxfileread(file, 0, bytes, sizeof(bytes), &result);
	check(result.success && result.count == 8 && !memcmp(bytes, "old-data", 8), "opaque endpoint read");
	styxfileclose(file); styxbackingfree(leaf);
	state->opaque = 0;
	styxbackingfree(root); dvm_provider_release(view);
	check(opened == closed && destroyed == 0 && escaped == 0, "case ownership");
	opened = closed = 0;
}

/* EN/RU: Hand a real local directory publication grant to the actual TCP fixture.
 * Передать настоящий локальный directory grant тесту через настоящий TCP. */
int main(void)
{
	DVM_Provider *base, *view, *grant = NULL;
	DVM_ExportOptions options = {0}; DVM_ExportRequest request = {0};
	DVM_Publication *publication; DVM_ExportTicket *ticket;
	StyxBacking *root; int status;
	state = calloc(1, sizeof(*state)); check(state != NULL, "state");
	base = dvm_provider_create(&providerops, state); check(base != NULL, "base");
	cases(base);
	view = prepared(base); check(view != NULL, "view");
	options.limit = 4; options.direct = view; options.authorize = authorize;
	publication = dvm_publication_create(&options); check(publication != NULL, "publication");
	strcpy(request.peer, "native-owner"); strcpy(request.name, "tree-fixture");
	request.kind = 2; request.access = FVM_ACCESS_READ|FVM_ACCESS_EXEC;
	ticket = dvm_publication_request(publication, &request);
	check(ticket && dvm_export_take(ticket, &grant) == 1, "directory grant");
	root = styxtree(grant); check(root != NULL, "tree capability");
	dvm_export_release(ticket); dvm_publication_close(publication);
	dvm_provider_release(grant); dvm_provider_release(view); dvm_provider_release(base);
	status = treewire(root);
	check(status == 0 && opened == closed && destroyed == 1 && escaped == 0, "wire ownership/confinement");
	puts("VFS tree TCP: all opens closed, one factory destroyed, no escaped paths");
	return 0;
}
