/* Copyright (c) 2026 bhgv
 * EN: Actual libstyx fixture callbacks using only the scalar VFS file boundary.
 * RU: Настоящие callbacks libstyx через скалярную границу VFS-файла.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "file.h"
static Styxserver server; /* Isolated test owner; not a general service registry. */
static StyxBacking *backing; /* Owned preapproved local grant, never a wire path. */
static int stopping;
#define Fileqid 0xfedcba9876543210ULL
#define Controlqid 2

/* EN/RU: Resolve only the fixture aliases inside the granted root.
 * Разрешить только aliases теста внутри выданного корня. */
static char *walk(Qid *q, char *name)
{
	if(q->path != Qroot) return "not a directory";
	if(!strcmp(name, "ctl")) *q = styxqid(Controlqid, 0);
	else if(!strcmp(name, "file") && backing) *q = styxqid(Fileqid, 0);
	else if(!strcmp(name, "..")) *q = styxqid(Qroot, 1);
	else return Enonexist;
	return nil;
}

/* EN/RU: Transfer one actual VFS open to this fid; never key handles by Qid.
 * Передать одно настоящее VFS-открытие этому fid; не хранить handles по Qid. */
static char *openfile(Qid *q, int mode)
{
	StyxOpen *opened;
	if(q->path == Qroot) return mode == OREAD ? nil : Eperm;
	if(q->path == Controlqid) return mode == OWRITE ? nil : Eperm;
	opened = styxfileopen(backing, mode);
	if(!opened) return Eperm;
	if(styxsetaux(&server, opened) < 0){ styxfileclose(opened); return Eperm; }
	return nil;
}

/* EN/RU: Metadata follows the open object when available, otherwise the live grant.
 * Metadata следуют открытому объекту, иначе живому grant. */
static char *statfile(Qid q, Dir *d)
{
	StyxMeta meta;
	StyxOpen *opened = styxgetaux(&server);
	if(!(opened ? styxfileinfo(opened, &meta) : styxbackinginfo(backing, &meta))) return Enonexist;
	memset(d, 0, sizeof(*d));
	d->qid = q;
	d->mode = meta.mode;
	d->length = meta.length;
	d->name = "file";
	d->uid = d->gid = d->muid = "fixture";
	return nil;
}

/* EN/RU: Preserve exact prefix acknowledgement; uncertainty is never hidden as success.
 * Сохранить точный prefix; неопределённые effects не превращать в успех. */
static char *transfer(StyxTransfer result, ulong *n)
{
	if(!result.confirmed || result.uncertain || (!result.success && !result.count)) return Eperm;
	*n = result.count;
	return nil;
}

/* EN/RU: Live one-entry directory or a retained VFS open, with real 64-bit offset.
 * Живой каталог одной записи либо удержанное VFS-открытие по полному offset. */
static char *readfile(Qid q, char *buf, ulong *n, vlong offset)
{
	Dir d;
	StyxTransfer result;
	if(q.path == Qroot){
		if(offset != 0){ *n = 0; return nil; }
		if(statfile(styxqid(Fileqid, 0), &d) != nil) return Enonexist;
		if(sizeD2M(&d) > *n) return "directory read too small";
		*n = convD2M(&d, (uchar*)buf, *n);
		return nil;
	}
	if(offset < 0) return Ebadarg;
	styxfileread(styxgetaux(&server), offset, buf, *n, &result);
	return transfer(result, n);
}

/* EN/RU: Test controls consume grant ownership; data writes delegate exactly once.
 * Управление тестом поглощает grant; запись данных делегируется ровно раз. */
static char *writefile(Qid q, char *buf, ulong *n, vlong offset)
{
	StyxTransfer result;
	if(q.path == Controlqid){
		if(*n == 9 && !memcmp(buf, "unpublish", 9)){ styxbackingfree(backing); backing = nil; }
		else if(*n == 4 && !memcmp(buf, "stop", 4)) stopping = 1;
		else return Ebadarg;
		return nil;
	}
	if(offset < 0) return Ebadarg;
	styxfilewrite(styxgetaux(&server), offset, buf, *n, &result);
	return transfer(result, n);
}

/* EN/RU: Serve only the caller-owned synthetic grant, with the original POSIX transport.
 * Обслужить только синтетический grant владельца исходным POSIX transport. */
int wirefixture(StyxBacking *root)
{
	Styxops ops;
	struct sockaddr_in address;
	socklen_t length = sizeof(address);
	backing = root;
	memset(&ops, 0, sizeof(ops));
	ops.walk = walk;
	ops.open = openfile;
	ops.read = readfile;
	ops.write = writefile;
	ops.stat = statfile;
	ops.freefid = styxfileclose;
	if(styxinit(&server, &ops, "0", 0555, 0) != nil) return 1;
	if(getsockname(server.connfd, (struct sockaddr*)&address, &length) < 0) return 2;
	print("%d\n", ntohs(address.sin_port));
	while(!stopping){
		if(styxwait(&server) != nil || styxprocess(&server) != nil) return 3;
	}
	if(styxend(&server) != nil) return 4;
	styxbackingfree(backing);
	return 0;
}
