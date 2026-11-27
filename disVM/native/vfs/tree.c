/* Copyright (c) 2026 bhgv
 * EN: Live directory node capabilities, bounded component walks and cursor retirement.
 * RU: Живые node-capabilities каталогов, ограниченный обход и retirement курсоров.
 */
#include "file_internal.h"
#include "tree.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* EN/RU: Construct an owned identity-bound node inside an already confined provider.
 * Создать владеющий node внутри уже ограниченного provider с привязкой к identity. */
static StyxBacking *node(DVM_Provider *provider, const char *path)
{
	FVM_FileInfo info;
	StyxMeta meta;
	StyxBacking *result;
	if(dvm_provider_info(provider, path, &info) != FVM_OK || !styxmeta(&info, &meta))
		return NULL;
	result = calloc(1, sizeof(*result));
	if(!result) return NULL;
	if(!dvm_provider_retain(provider)){ free(result); return NULL; }
	result->provider = provider;
	strcpy(result->path, path);
	result->captured = meta;
	result->bound = 1;
	return result;
}

/* EN/RU: See file_provider.h; this constructor never enumerates a tree.
 * См. file_provider.h; конструктор никогда не перечисляет дерево. */
StyxBacking *styxtree(DVM_Provider *provider)
{
	StyxBacking *result = provider ? node(provider, ".") : NULL;
	if(result && !result->captured.directory){ styxbackingfree(result); return NULL; }
	return result;
}

/* EN/RU: See tree.h; reject separators before invoking any provider operation.
 * См. tree.h; отклонить разделители до вызова provider. */
StyxBacking *styxwalknode(StyxBacking *parent, const char *component)
{
	StyxMeta info;
	char path[DVM_NS_PATH_LIMIT], *slash;
	if(!parent || !parent->bound || !component || !*component || strchr(component, '/') || strlen(component) >= STYX_NAME_MAX)
		return NULL;
	if(!styxbackinginfo(parent, &info) || !info.directory ||
		dvm_provider_access(parent->provider, parent->path, FVM_ACCESS_EXEC) != FVM_OK)
		return NULL;
	strcpy(path, parent->path);
	if(!strcmp(component, "..")){
		slash = strrchr(path, '/');
		if(slash) *slash = 0;
		else strcpy(path, ".");
	}else if(strcmp(component, ".")){
		if((!strcmp(path, ".") ? 0 : strlen(path)+1)+strlen(component)+1 > sizeof(path)) return NULL;
		if(!strcmp(path, ".")) strcpy(path, component);
		else { strcat(path, "/"); strcat(path, component); }
	}
	StyxBacking *result = node(parent->provider, path);
	if(result && !styxbackinginfo(parent, &info)){
		styxbackingfree(result);
		return NULL;
	}
	if(result && (!strcmp(component, ".") || (!strcmp(parent->path, ".") && !strcmp(component, ".."))) &&
		!styxsame(&parent->captured, &result->captured)){
		styxbackingfree(result);
		return NULL;
	}
	return result;
}

/* EN/RU: See tree.h. / См. tree.h. */
int styxbackingsamenode(const StyxBacking *left, const StyxBacking *right)
{
    return left && right && left->bound && right->bound && left->provider == right->provider &&
        !strcmp(left->path, right->path) && styxsame(&left->captured, &right->captured);
}


/* EN/RU: See tree.h. / См. tree.h. */
StyxBacking *styxbackingcopyat(const StyxBacking *node, const char *path)
{
    if (!node || !node->bound || !path || strcmp(node->path, path))
    {
        return NULL;
    }
    StyxBacking *copy = malloc(sizeof(*copy));
    if (!copy || !dvm_provider_retain(node->provider))
    {
        free(copy);
        return NULL;
    }
    *copy = *node;
    return copy;
}

/* EN/RU: Refresh current directory authority and bound identity, never mode inference.
 * Перепроверить полномочия и identity каталога, не выводя доступ из mode. */
static int valid(StyxOpen *opened)
{
	FVM_FileInfo info;
	StyxMeta current;
	return opened && opened->directory &&
		dvm_provider_access(opened->provider, opened->path, FVM_ACCESS_READ|FVM_ACCESS_EXEC) == FVM_OK &&
		dvm_provider_info(opened->provider, opened->path, &info) == FVM_OK &&
		styxmeta(&info, &current) && styxsame(&opened->captured, &current);
}

/* EN: Validate the held directory and restart from its current projection when required.
 * RU: Проверить удерживаемый каталог и при необходимости начать с его текущего представления. */
static int begin_directory(StyxOpen *opened, unsigned long long offset, int live)
{
	DVM_VfsHandle fresh = {0};
	FVM_FileInfo info;
	StyxMeta current;
	if(!valid(opened)) return 0;
	if(offset){ return !opened->failed && offset == opened->position; }
	if(live || opened->started || opened->failed){
		if(dvm_provider_open(opened->provider, opened->path, "r", &fresh) != FVM_OK)
			return 0;
		if(dvm_vfs_info(&fresh, &info) != FVM_OK || !styxmeta(&info, &current) || !styxsame(&current, &opened->captured)){
			dvm_vfs_close(&fresh);
			return 0;
		}
		fvm_port_directory_entries_free(opened->batch);
		opened->batch = NULL;
		opened->count = opened->index = 0;
		dvm_vfs_close(&opened->handle);
		dvm_vfs_move(&opened->handle, &fresh);
	}
	opened->position = 0;
	opened->pending = opened->failed = 0;
	opened->started = 1;
	return 1;
}

/* EN/RU: See tree.h. / См. tree.h. */
int styxdirbegin(StyxOpen *opened, unsigned long long offset)
{
    return begin_directory(opened, offset, 0);
}


/* EN/RU: See tree.h. / См. tree.h. */
int styxdirbegin_live(StyxOpen *opened, unsigned long long offset)
{
    return begin_directory(opened, offset, 1);
}


/* EN/RU: Invalidate a failed cursor instead of silently skipping results or returning EOF.
 * Пометить отказавший курсор вместо молчаливого пропуска результатов/EOF. */
static int fail(StyxOpen *opened)
{
	if(opened){
		opened->failed = 1;
		opened->pending = 0;
		dvm_vfs_abort_list(&opened->handle);
	}
	return -1;
}


/* EN/RU: See tree.h. / См. tree.h. */
StyxOpen *styxcreatechild(StyxBacking *parent, const char *name, int directory, StyxBacking **out)
{
    StyxMeta info;
    if (!parent || !name || !*name || strchr(name, '/') || !strcmp(name, ".") ||
        !strcmp(name, "..") || strlen(name) >= STYX_NAME_MAX || !out ||
        !styxbackinginfo(parent, &info) || !info.directory)
    {
        return NULL;
    }
    size_t prefix = !strcmp(parent->path, ".") ? 0 : strlen(parent->path) + 1;
    if (prefix + strlen(name) >= DVM_NS_PATH_LIMIT)
    {
        return NULL;
    }
    StyxBacking *child = calloc(1, sizeof(*child));
    StyxOpen *file = calloc(1, sizeof(*file));
    if (!child || !file || !dvm_provider_retain(parent->provider))
    {
        free(child);
        free(file);
        return NULL;
    }
    child->provider = parent->provider;
    if (prefix)
    {
        memcpy(child->path, parent->path, prefix - 1);
        child->path[prefix - 1] = '/';
    }
    strcpy(child->path + prefix, name);
    if (dvm_provider_open(child->provider, child->path, directory ? "mkdir" : "wx", &file->handle) != FVM_OK ||
        !styxfileinfo(file, &child->captured) || child->captured.directory != directory ||
        (directory && !dvm_provider_retain(child->provider)))
    {
        styxfileclose(file);
        styxbackingfree(child);
        return NULL;
    }
    child->bound = 1;
    file->access = directory ? FVM_ACCESS_READ : FVM_ACCESS_WRITE;
    if (directory)
    {
        file->provider = child->provider;
        file->directory = 1;
        file->captured = child->captured;
        strcpy(file->path, child->path);
    }
    *out = child;
    return file;
}


/* EN/RU: See tree.h. / См. tree.h. */
int styxremovenode(const StyxBacking *node, StyxRemoveNode remove, void *state)
{
    return node && node->bound && strcmp(node->path, ".") && remove(state, node->path, &node->captured);
}


/* EN/RU: See tree.h; retain at most the current provider batch, refresh each pending child.
 * См. tree.h; удержать только текущую batch provider и обновлять ожидающего ребёнка. */
int styxdirpeek(StyxOpen *opened, StyxMeta *output)
{
	FVM_FileInfo current;
	FVM_Result status;
	char path[DVM_NS_PATH_LIMIT];
	const char *name;
	if(!output || !valid(opened) || !opened->started || opened->failed)
		return fail(opened);
	for(;;){
		if(opened->index == opened->count){
			fvm_port_directory_entries_free(opened->batch);
			opened->batch = NULL;
			opened->count = opened->index = 0;
			status = dvm_vfs_list(&opened->handle, &opened->batch, &opened->count);
			if(status != FVM_OK || (opened->count && !opened->batch)) return fail(opened);
			if(!opened->count){ opened->pending = 0; return 0; }
		}
		name = opened->batch[opened->index].name;
		if(!memchr(name, 0, sizeof(opened->batch[opened->index].name)) || !*name ||
			strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, "..")) return fail(opened);
		if(!memchr(name, 0, FVM_FS_NAME_MAX) || !*name || strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, ".."))
			return fail(opened);
		if((!strcmp(opened->path, ".") ? 0 : strlen(opened->path)+1)+strlen(name)+1 > sizeof(path)) return fail(opened);
		strcpy(path, !strcmp(opened->path, ".") ? "" : opened->path);
		if(*path) strcat(path, "/");
		strcat(path, name);
		status = dvm_provider_info(opened->provider, path, &current);
		if(status == FVM_NOT_FOUND){ opened->index++; opened->pending = 0; continue; }
		if(status != FVM_OK || !styxmeta(&current, output)) return fail(opened);
		/* The enumerated virtual basename is authoritative, not a backend display path. */
		strcpy(output->name, name);
		opened->pending = 1;
		return 1;
	}
}

/* EN/RU: See tree.h; never advance until the complete record is published.
 * См. tree.h; не продвигать курсор до публикации полной записи. */
int styxdiradvance(StyxOpen *opened, size_t bytes)
{
	if(!opened || !opened->directory || !opened->pending || opened->failed || !bytes ||
		bytes > (unsigned long long)LONG_MAX-opened->position)
		return 0;
	opened->position += bytes;
	opened->index++;
	opened->pending = 0;
	return 1;
}
