/* Copyright (c) 2026 bhgv
 * EN: Owned VFS file opens, precise chunks and metadata across a scalar C boundary.
 * RU: Владеющие VFS открытия, точные чанки и metadata через скалярную C-границу.
 */
#include "file_provider.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct StyxBacking { DVM_Provider *provider; }; /* One retained prepared capability. */
struct StyxOpen { DVM_VfsHandle handle; unsigned long long position; int poisoned; unsigned access; }; /* Exclusive owner FIFO. */

/* EN/RU: Copy scalar metadata without interpreting permissions as authority.
 * Скопировать скалярные metadata, не превращая mode в полномочия. */
static void metadata(const FVM_FileInfo *info, StyxMeta *output)
{
	output->identity = ((unsigned long long)(info->identity.high&0xffffffffUL)<<32) | (info->identity.low&0xffffffffUL);
	output->length = ((unsigned long long)(info->length.high&0xffffffffUL)<<32) | (info->length.low&0xffffffffUL);
	output->mode = info->mode;
	output->directory = info->directory;
}

/* EN/RU: See file_provider.h; retain only after metadata confirms a file.
 * См. file_provider.h; удержать после проверки metadata файла. */
StyxBacking *styxbacking(DVM_Provider *provider)
{
	FVM_FileInfo info;
	StyxBacking *backing;
	if(!provider || dvm_provider_info(provider, ".", &info) != FVM_OK || info.directory)
		return NULL;
	backing = calloc(1, sizeof(*backing));
	if(!backing)
		return NULL;
	if(!dvm_provider_retain(provider)){
		free(backing);
		return NULL;
	}
	backing->provider = provider;
	return backing;
}

/* EN/RU: See file.h; no open handle borrows this wrapper.
 * См. file.h; открытые handles не заимствуют этот wrapper. */
void styxbackingfree(StyxBacking *backing)
{
	if(backing){
		dvm_provider_release(backing->provider);
		free(backing);
	}
}

/* EN/RU: See file.h; output changes on success only.
 * См. file.h; output меняется только при успехе. */
int styxbackinginfo(StyxBacking *backing, StyxMeta *output)
{
	FVM_FileInfo info;
	if(!backing || !output || dvm_provider_info(backing->provider, ".", &info) != FVM_OK)
		return 0;
	metadata(&info, output);
	return 1;
}

/* EN/RU: See file.h; successful open independently pins the prepared provider.
 * См. file.h; успешное открытие независимо удерживает prepared provider. */
StyxOpen *styxfileopen(StyxBacking *backing, int mode)
{
	static const char *modes[] = {"r", "wo", "rw"};
	StyxOpen *opened;
	FVM_FileInfo info;
	if(!backing || mode < 0 || mode > 2)
		return NULL;
	opened = calloc(1, sizeof(*opened));
	if(!opened)
		return NULL;
	if(dvm_provider_open(backing->provider, ".", modes[mode], &opened->handle) != FVM_OK){
		free(opened);
		return NULL;
	}
	if(dvm_vfs_info(&opened->handle, &info) != FVM_OK || info.directory){
		dvm_vfs_close(&opened->handle);
		free(opened);
		return NULL;
	}
	opened->access = mode == 0 ? 1U : mode == 1 ? 2U : 3U;
	return opened;
}

/* EN/RU: See file.h; clear/close owned native handle exactly once.
 * См. file.h; закрыть владеющий native handle ровно один раз. */
void styxfileclose(void *value)
{
	StyxOpen *opened = value;
	if(opened){
		dvm_vfs_close(&opened->handle);
		free(opened);
	}
}

/* EN/RU: See file.h; delegate the retained handle metadata contract.
 * См. file.h; запрос следует metadata-контракту удержанного handle. */
int styxfileinfo(StyxOpen *opened, StyxMeta *output)
{
	FVM_FileInfo info;
	if(!opened || !output || dvm_vfs_info(&opened->handle, &info) != FVM_OK)
		return 0;
	metadata(&info, output);
	return 1;
}

/* EN/RU: Position before I/O; nonseekable streams permit only sequential offsets.
 * Failure poisons further operations because generic seek effects may be unknown.
 * Позиционировать до I/O; непозиционные потоки требуют последовательный offset.
 * После отказа seek дальнейшие операции запрещены из-за неизвестных effects. */
static int position(StyxOpen *opened, unsigned long long offset)
{
	long actual;
	if(!opened || opened->poisoned || offset > LONG_MAX)
		return 0;
	if(offset == opened->position)
		return 1;
	if(!opened->handle.ops->seek)
		return 0;
	if(dvm_vfs_seek(&opened->handle, (long)offset, FVM_FILE_SEEK_START, &actual) != FVM_OK || actual != (long)offset){
		opened->poisoned = 1;
		return 0;
	}
	opened->position = offset;
	return 1;
}

/* EN/RU: Preserve acknowledgement certainty, stop cause and cursor effects.
 * Сохранить достоверность подтверждения, причину остановки и смещение. */
static void outcome(StyxOpen *opened, DVM_IoResult result, StyxTransfer *output)
{
	output->confirmed = (result.flags&DVM_IO_PREFIX) != 0;
	output->uncertain = (result.flags&DVM_IO_UNCERTAIN) != 0;
	output->count = output->confirmed ? result.count : 0;
	output->success = result.status == FVM_OK;
	output->stop = result.stop;
	if(output->uncertain || output->count > (unsigned long long)LONG_MAX-opened->position)
		opened->poisoned = 1;
	else
		opened->position += output->count;
}

/* EN/RU: See file.h; only the actual operation's result determines success.
 * См. file.h; успех определяет результат самой операции. */
void styxfileread(StyxOpen *opened, unsigned long long offset, void *bytes, size_t size, StyxTransfer *output)
{
	if(!output)
		return;
	memset(output, 0, sizeof(*output));
	output->stop = DVM_IO_INVALID;
	if(opened && !(opened->access&1U)){
		output->confirmed = 1;
		output->stop = DVM_IO_DENIED;
		return;
	}
	if(position(opened, offset))
		outcome(opened, dvm_vfs_read_chunk(&opened->handle, bytes, size), output);
}

/* EN/RU: See file.h; precise native write acknowledgements are never fabricated.
 * См. file.h; точные подтверждения native-записи не фабрикуются. */
void styxfilewrite(StyxOpen *opened, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *output)
{
	if(!output)
		return;
	memset(output, 0, sizeof(*output));
	output->stop = DVM_IO_INVALID;
	if(opened && !(opened->access&2U)){
		output->confirmed = 1;
		output->stop = DVM_IO_DENIED;
		return;
	}
	if(position(opened, offset))
		outcome(opened, dvm_vfs_write_chunk(&opened->handle, bytes, size), output);
}
