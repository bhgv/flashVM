/* SDL memory-backed streams exercise file growth/short read/EOF error/close cleanup. */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <assert.h>
#include <stdlib.h>
static const unsigned char data[2]={65,66};
static Sint64 advertised;
static int available, closes, reads, probe_error, close_error, open_error;
static SDL_RWops *test_open(const char *path,const char *mode)
{ (void)path;(void)mode;return open_error?NULL:SDL_RWFromConstMem(data,available); }
static Sint64 test_size(SDL_RWops *file)
{ (void)file;return advertised; }
static size_t test_read(SDL_RWops *file,void *out,size_t width,size_t count)
{
    ++reads;
    if(probe_error && reads>1){SDL_SetError("injected EOF read error");return 0;}
    return SDL_RWread(file,out,width,count);
}
static int test_close(SDL_RWops *file)
{
    int result=SDL_RWclose(file);++closes;
    if(close_error){SDL_SetError("injected close error");return -1;}
    return result;
}
#define SDL_RWFromFile test_open
#define SDL_RWsize test_size
#define SDL_RWread test_read
#define SDL_RWclose test_close
#include "../common/player.c"
#undef SDL_RWFromFile
#undef SDL_RWsize
#undef SDL_RWread
#undef SDL_RWclose
static void check(int success)
{
    size_t size=91;unsigned char *bytes;
    closes=reads=0;bytes=read_movie("memory-fixture",&size);
    assert(closes==(open_error?0:1));
    if(success){assert(bytes && size==(size_t)advertised && bytes[0]==65);free(bytes);}
    else assert(!bytes && size==91);
}
int main(void)
{
    available=2;advertised=2;check(1);
    SDL_SetError("stale unrelated error");check(1);
    advertised=1;check(0);
    advertised=2;available=1;check(0);
    advertised=0;check(0);
    advertised=-1;check(0);
    advertised=(Sint64)FVM_SWF_DEFAULT_LIMIT+1;check(0);
    available=2;advertised=2;probe_error=1;check(0);probe_error=0;
    close_error=1;check(0);close_error=0;
    open_error=1;check(0);
    return 0;
}
