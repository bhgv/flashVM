/* Included native SDL integration fixture: token-free node motion never enters Dis. */


/* Append one independent seven-word command to caller-owned fixture storage. */
static size_t local_command(char *bytes, size_t at, const DVM_VectorCommand *n)
{
    return at+(size_t)sprintf(bytes+at,"%08lx%08lx%08lx%08lx%08lx%08lx%08lx",
        n->verb,n->x,n->y,n->cx,n->cy,n->c2x,n->c2y);
}


/* Publish the document owner's copied cubic geometry and optional point plan. */
static void local_scene_finish(unsigned long ticket, int planned, unsigned long point, int begin)
{
    DVM_VectorCommand source[2] = {{0,200,200,0,0,0,0},{4,600,400,300,100,500,500}};
    DVM_VectorCommand overlay[9] = {{0,200,200,0,0,0,0},{4,600,400,300,100,500,500},
        {0,200,200,0,0,0,0},{1,300,100,0,0,0,0},{0,295,95,0,0,0,0},
        {1,305,95,0,0,0,0},{1,305,105,0,0,0,0},{1,295,105,0,0,0,0},{3,0,0,0,0,0,0}};
    unsigned long bindings[9] = {0,0,384,1280,1321,1322,1323,1324,0};
    char bytes[1024];
    size_t at;
    unsigned int i;
    snprintf(bytes,sizeof(bytes),"%08lx000000010000000000000064",ticket);
    if (begin)
    {
        send(ticket,DVM_DESK_BEGIN,960,640,bytes);
    }
    send(ticket,DVM_DESK_ITEM,1,0,"000000040000005400000030000003280000024400000001ffffffff00000000ffffffffVector");
    at = (size_t)sprintf(bytes,"%08lx00245ac400000008000000000000000000000000",planned == 4 ? 0x260007UL : planned == 3 ? 0x260006UL : planned == 2 ? 0x1e0007UL : planned ? 0x60007UL : 0x20007UL);
    if (planned)
    {
        at += (size_t)sprintf(bytes+at,"0000000000000000%08lx%08lx0000000200000009",point%1001,point/1001);
        if (planned >= 3)
        {
            at += (size_t)sprintf(bytes+at,"%08lx00000003",planned == 3 ? 2UL : 5UL);
        }
        at += (size_t)sprintf(bytes+at,"0000000000000002");
        for (i = 0; i < 9; ++i)
        {
            at += (size_t)sprintf(bytes+at,"%08lx",bindings[i]);
        }
    }
    at += (size_t)sprintf(bytes+at,"0000000200245ac4000000010000000000000002");
    for (i = 0; i < 2; ++i)
    {
        at = local_command(bytes,at,source+i);
    }
    at += (size_t)sprintf(bytes+at,"001687df000000010000000000000009");
    if (planned != 2)
    {
        for (i = 0; i < 9; ++i)
        {
            at = local_command(bytes,at,overlay+i);
        }
    }
    send(ticket,DVM_DESK_VECTOR_BEGIN,1,(long)at,"0000000000000002");
    send(ticket,DVM_DESK_VECTOR_DATA,1,0,bytes);
    send(ticket,DVM_DESK_END,0,0,"");
}


/* Publish one complete scene; the split helper also tests input during staging. */
static void local_scene(unsigned long ticket, int planned, unsigned long point)
{
    local_scene_finish(ticket,planned,point,1);
}


/* Dispatch600 real SDL moves through the production async owner loop without
 * a SWF frame advance or intermediate document reply; only Begin/End may emerge. */
static void local_motion(unsigned long *ticket)
{
    DVM_UIMessage event;
    char word[9];
    unsigned long down;
    unsigned long widget;
    long latest;
    unsigned int i;
    DVM_EditorFrame *before;
    DVM_EditorFrame *after;
    const FVM_RasterSurface *a;
    FVM_RasterSurface composed;
    size_t copied;
    local_scene(++*ticket,2,0);
    pointer(SDL_MOUSEBUTTONDOWN,348,160);
    event = proposal();
    require(event.code == DVM_DESK_POINTER && event.size == 32 && !memcmp(event.text,"00000001",8),
        "node Begin is the sole selection admission");
    memcpy(word,event.text+24,8);
    word[8] = 0;
    down = strtoul(word,NULL,16);
    pointer(SDL_MOUSEMOTION,368,180);
    {
        DVM_UIMessage ignored;
        for (i = 0; i < 32; ++i)
        {
            require(dvm_editor_event(&ignored) == 0,"eager motion does not wait for a Dis Begin reply");
            if (dvm_desktop_local_position(&widget,&latest) && latest != (long)down)
            {
                break;
            }
            SDL_Delay(1);
        }
        require(dvm_desktop_local_position(&widget,&latest) && latest != (long)down,
            "visible handle moves before any owner acknowledgement");
    }
    {
        long eager = latest;
        local_scene(++*ticket,1,down);
        require(dvm_desktop_local_position(&widget,&latest) && latest == eager,
            "owner acknowledgement retains the eager pointer instead of rewinding");
    }
    for (i = 0; i < 20; ++i)
    {
        DVM_UIMessage ignored;
        require(dvm_editor_event(&ignored) == 0,"baseline asynchronous presentation emits no Dis proposal");
        SDL_Delay(1);
    }
    before = dvm_editor_frame_create(960,640);
    require(before != NULL,"baseline frame allocation");
    require(dvm_desktop_prepare_cached(before,NULL),"baseline frame capture");
    require(dvm_editor_frame_incremental(before,NULL),"baseline node target admission");
    require(dvm_editor_frame_render(before) == FVM_OK,"baseline frame raster");
    for (i = 0; i < 600; ++i)
    {
        pointer(SDL_MOUSEMOTION,348+(int)i%71,160+(int)i%71);
    }
    for (i = 0; i < 100; ++i)
    {
        DVM_UIMessage ignored;
        require(dvm_editor_event(&ignored) == 0,"captured motion emits no intermediate Dis proposal");
        SDL_Delay(1);
    }
    require(dvm_desktop_local_position(&widget,&latest) && latest%1001 > (long)(down%1001),
        "local geometry changes while mouse is still held");
    after = dvm_editor_frame_create(960,640);
    require(after && dvm_desktop_prepare_cached(after,before) && dvm_editor_frame_incremental(after,before) &&
        dvm_editor_frame_render(after) == FVM_OK,"local motion rerasterizes against retained baseline");
    a = dvm_editor_frame_surface(before);
    composed = *a;
    composed.pixels = malloc((size_t)a->stride*a->height);
    require(composed.pixels != NULL,"independent presentation buffer");
    memcpy(composed.pixels,a->pixels,(size_t)a->stride*a->height);
    require(dvm_editor_frame_present(after,&composed,0,&copied) && copied > 0 &&
        memcmp(a->pixels,composed.pixels,(size_t)a->stride*a->height),
        "actual changed pixels appear during drag, without End or Dis reply");
    free(composed.pixels);
    dvm_editor_frame_destroy(after);
    {
        char begin[40];
        DVM_UIMessage ignored;
        snprintf(begin,sizeof(begin),"%08lx000000010000000000000064",++*ticket);
        send(*ticket,DVM_DESK_BEGIN,960,640,begin);
        pointer(SDL_MOUSEMOTION,398,210);
        require(dvm_editor_event(&ignored) == 0 && SDL_HasEvent(SDL_MOUSEMOTION),
            "partial owner snapshot cannot consume captured motion against staged geometry");
        local_scene_finish(*ticket,1,down,0);
        for (i = 0; i < 30; ++i)
        {
            require(dvm_editor_event(&ignored) == 0,"resumed native motion still avoids Dis");
            SDL_Delay(1);
        }
        require(dvm_desktop_local_position(&widget,&latest) && latest%1001 >= (long)(down%1001)+69,
            "capture survives an unrelated staged view");
    }
    pointer(SDL_MOUSEBUTTONUP,418,230);
    event = proposal();
    require(event.code == DVM_DESK_POINTER && event.size == 32 && !memcmp(event.text,"00000003",8),
        "all600 node motions stay in FlashVM; only release crosses to Dis");
    require(dvm_desktop_local_position(&widget,&latest) && widget == 1 && latest%1001 >= (long)(down%1001)+99 &&
        latest/1001 >= (long)(down/1001)+99,"release adopts last valid local position");
    local_scene(++*ticket,1,down);
    require(dvm_desktop_local_position(&widget,&latest) && latest%1001 >= (long)(down%1001)+99,
        "unrelated canonical snapshot cannot rewind local motion");
    dvm_desktop_local_cancel();
    dvm_editor_frame_destroy(before);
    require(!dvm_desktop_local_active(),"native cancel retires modifier baseline");
    scene(++*ticket);
}


/* Captured scale/marquee use copied generic modifiers with only Begin/End
 * crossing the document boundary. The test supplies the owner's admitted plan. */
static void local_frames(unsigned long *ticket)
{
    unsigned int mode;
    for (mode = 3; mode <= 4; ++mode)
    {
        DVM_UIMessage event;
        char word[9];
        unsigned long down;
        unsigned long widget;
        long latest = 0;
        unsigned int i;
        local_scene(++*ticket,0,0);
        pointer(SDL_MOUSEBUTTONDOWN,348,160);
        event = proposal();
        require(event.code == DVM_DESK_POINTER && !memcmp(event.text,"00000001",8),
            "frame Begin reaches document once");
        memcpy(word,event.text+24,8);
        word[8] = 0;
        down = strtoul(word,NULL,16);
        local_scene(++*ticket,(int)mode,down);
        for (i = 0; i < 100; ++i)
        {
            pointer(SDL_MOUSEMOTION,348+(int)i%41,160+(int)i%41);
        }
        for (i = 0; i < 64; ++i)
        {
            DVM_UIMessage ignored;
            require(dvm_editor_event(&ignored) == 0,"frame motion emits no intermediate Dis requests");
            SDL_Delay(1);
        }
        require(dvm_desktop_local_position(&widget,&latest) && widget == 1 && latest != (long)down,
            "scale/marquee progress locally before release");
        pointer(SDL_MOUSEBUTTONUP,388,200);
        event = proposal();
        require(event.code == DVM_DESK_POINTER && !memcmp(event.text,"00000003",8),
            "frame release publishes one final point");
        dvm_desktop_local_cancel();
        scene(++*ticket);
    }
}


/* A tap with no geometry change must retire its baseline at the idle reply,
 * otherwise a later drag would reuse the previous pointer origin. */
static void local_taps(unsigned long *ticket)
{
    unsigned int tap;
    for (tap = 0; tap < 2; ++tap)
    {
        DVM_UIMessage event;
        char word[9];
        unsigned long down;
        local_scene(++*ticket,2,0);
        pointer(SDL_MOUSEBUTTONDOWN,348,160);
        event = proposal();
        require(dvm_desktop_local_active(),"each tap begins a fresh prepared capture");
        memcpy(word,event.text+24,8);
        word[8] = 0;
        down = strtoul(word,NULL,16);
        local_scene(++*ticket,1,down);
        pointer(SDL_MOUSEBUTTONUP,348,160);
        event = proposal();
        require(!memcmp(event.text,"00000003",8),"zero-motion tap publishes End");
        local_scene(++*ticket,2,down);
        require(!dvm_desktop_local_active(),"idle acknowledgement retires zero-motion capture");
    }
    scene(++*ticket);
}
