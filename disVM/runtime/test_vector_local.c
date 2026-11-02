/* Independent copied-geometry tests; no parser or document implementation mirrored. */
#include "vector_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Keep release checks active and identify the failed value-boundary invariant. */
static void require(int ok, const char *message)
{
    if (!ok)
    {
        fprintf(stderr,"FAIL local vector: %s\n",message);
        exit(1);
    }
}


/* Exercise independent cubic controls, point bindings, clipping and transactionality. */
int main(void)
{
    DVM_VectorLocalPlan plan;
    DVM_VectorCommand source[2] = {{0,200,200,0,0,0,0},{4,600,400,300,100,500,500}};
    DVM_VectorCommand overlay[9];
    DVM_VectorCommand actual[2];
    DVM_VectorCommand decorated[9];
    DVM_VectorLocal *edit;
    long x;
    long y;
    unsigned int i;
    memset(&plan,0,sizeof(plan));
    memset(overlay,0,sizeof(overlay));
    plan.count = 2;
    plan.overlay = 9;
    plan.x = 300;
    plan.y = 100;
    plan.masks[1] = 2;
    overlay[0] = source[0];
    overlay[1] = source[1];
    overlay[2] = (DVM_VectorCommand){0,200,200,0,0,0,0};
    overlay[3] = (DVM_VectorCommand){1,300,100,0,0,0,0};
    overlay[4] = (DVM_VectorCommand){0,295,95,0,0,0,0};
    overlay[5] = (DVM_VectorCommand){1,305,95,0,0,0,0};
    overlay[6] = (DVM_VectorCommand){1,305,105,0,0,0,0};
    overlay[7] = (DVM_VectorCommand){1,295,105,0,0,0,0};
    overlay[8].verb = 3;
    plan.bindings[2] = 1UL<<8;
    plan.bindings[3] = 5UL<<8;
    for (i = 0; i < 4; ++i)
    {
        plan.bindings[4+i] = (5UL<<8)|(5UL<<3)|(i+1);
    }
    plan.bindings[2] |= 128;
    require(dvm_vector_local_decorate(&plan,source,decorated) &&
        !memcmp(overlay,decorated,sizeof(overlay)),"copied recipe reconstructs exact guide/square geometry");
    plan.bindings[2] &= ~128UL;
    edit = dvm_vector_local_create(&plan,source,overlay);
    require(edit != NULL,"valid plan copied");
    require(dvm_vector_local_move(edit,400,200,actual,decorated),"control drag");
    require(actual[1].cx == 400 && actual[1].cy == 200 && actual[1].c2x == 500 && actual[1].x == 600,
        "only selected first cubic control changes");
    require(decorated[1].cx == 400 && decorated[3].x == 400 && decorated[4].x == 395 && decorated[6].y == 205,
        "source prefix, guide and square share copied point bindings");
    require(!memcmp(source,overlay,2*sizeof(*source)) && dvm_vector_local_matches(edit,&plan,source,overlay),
        "baseline remains immutable and can reconcile another owner view");
    plan.revision++;
    require(!dvm_vector_local_matches(edit,&plan,source,overlay),"new generation refuses old preview");
    plan.revision--;
    require(!dvm_vector_local_move(edit,-1,0,actual,decorated) && actual[1].cx == 400,
        "out-of-page control move preserves previous output");
    dvm_vector_local_point(edit,&x,&y);
    require(x == 400 && y == 200,"release retains last valid endpoint");
    require(dvm_vector_local_move(edit,300,100,actual,decorated) && !memcmp(actual,source,sizeof(source)),
        "return-to-origin uses original coordinates without drift");
    require(dvm_vector_local_move(edit,0,0,actual,decorated) && decorated[4].x == 0 && decorated[4].y == 0 &&
        decorated[5].x == 5 && decorated[6].y == 5,"page-edge marker clipping keeps original radius");
    dvm_vector_local_restore(edit,actual,decorated);
    require(!memcmp(decorated,overlay,sizeof(overlay)),"cancel restores exact source and overlay");
    dvm_vector_local_destroy(edit);
    plan.bindings[4] = 0xffffffffUL;
    require(!dvm_vector_local_valid(&plan,source,overlay),"malformed binding refuses before dereference");
    plan.bindings[4] = (5UL<<8)|(5UL<<3)|1;
    plan.masks[0] = 1;
    edit = dvm_vector_local_create(&plan,source,overlay);
    require(edit && !dvm_vector_local_move(edit,0,0,actual,decorated),"group motion rejects an out-of-page endpoint");
    dvm_vector_local_destroy(edit);
    plan.masks[0] = 2;
    require(!dvm_vector_local_valid(&plan,source,overlay),"Move has no curve control");
    {
        DVM_VectorCommand closed[3] = {{0,800,300,0,0,0,0},{4,800,300,800,500,500,300},{3,0,0,0,0,0,0}};
        DVM_VectorCommand output[3];
        DVM_VectorCommand outline[3];
        memset(&plan,0,sizeof(plan));
        plan.count = plan.overlay = 3;
        plan.x = 800;
        plan.y = 300;
        plan.masks[0] = 1;
        edit = dvm_vector_local_create(&plan,closed,closed);
        require(edit && dvm_vector_local_move(edit,850,350,output,outline),"closed anchor drag");
        require(output[0].x == 850 && output[1].x == 850 && output[0].y == output[1].y,
            "closed cubic endpoint aliases stay welded");
        dvm_vector_local_destroy(edit);
    }
    {
        DVM_VectorCommand curve[3] = {{0,100,200,0,0,0,0},
            {4,300,200,150,200,220,200},{4,500,200,340,200,450,200}};
        DVM_VectorCommand output[3];
        DVM_VectorCommand outline[3];
        memset(&plan,0,sizeof(plan));
        plan.count = plan.overlay = 3;
        plan.x = 340;
        plan.y = 200;
        plan.masks[1] = 8; /* Smooth relationship at anchor1. */
        plan.masks[2] = 2; /* Only outgoing first cubic control is selected. */
        edit = dvm_vector_local_create(&plan,curve,curve);
        require(edit && dvm_vector_local_move(edit,300,300,output,outline),"native smooth control move");
        require(output[1].c2x == 300 && output[1].c2y == 120,
            "smooth opposite control keeps its independent80-unit length");
        dvm_vector_local_destroy(edit);
        plan.masks[1] = 16; /* Symmetric relationship. */
        edit = dvm_vector_local_create(&plan,curve,curve);
        require(edit && dvm_vector_local_move(edit,300,300,output,outline) && output[1].c2y == 100,
            "symmetric controls mirror both angle and length");
        require(!dvm_vector_local_move(edit,300,500,output,outline) && output[1].c2y == 100,
            "out-of-page reflected handle preserves the last valid transaction");
        dvm_vector_local_destroy(edit);
        plan.masks[1] = 24;
        require(!dvm_vector_local_valid(&plan,curve,curve),"unknown relationship mode refuses");
    }
    {
        DVM_VectorCommand output[2];
        DVM_VectorCommand outline[7];
        DVM_VectorCommand frame[7];
        memset(&plan,0,sizeof(plan));
        plan.count = plan.overlay = 2;
        plan.mode = 1;
        plan.x = 300;
        plan.y = 100;
        edit = dvm_vector_local_create(&plan,source,source);
        require(edit && dvm_vector_local_move(edit,350,150,output,outline) &&
            output[1].x == 650 && output[1].cx == 350 && output[1].c2y == 550,
            "object translation transforms endpoints and both cubic controls locally");
        dvm_vector_local_destroy(edit);
        plan.mode = 2;
        plan.handle = 3;
        plan.x = 600;
        plan.y = 500;
        edit = dvm_vector_local_create(&plan,source,source);
        require(edit && dvm_vector_local_move(edit,800,600,output,outline) &&
            output[1].x == 800 && output[1].y == 475 && output[1].c2y == 600,
            "scale uses fixed opposite corner and immutable conservative bounds");
        require(dvm_vector_local_move(edit,600,500,output,outline) && !memcmp(output,source,sizeof(source)),
            "scale returns to exact source without incremental drift");
        dvm_vector_local_destroy(edit);
        plan.mode = 3;
        plan.x = 600;
        plan.y = 300;
        edit = dvm_vector_local_create(&plan,source,source);
        require(edit && dvm_vector_local_move(edit,400,500,output,outline) &&
            output[0].x == 500 && output[0].y == 100 && output[1].x == 300 && output[1].y == 500,
            "rotation transforms all explicit controls about source bounds centre");
        dvm_vector_local_destroy(edit);
        memset(frame,0,sizeof(frame));
        memcpy(frame,source,sizeof(source));
        for (i = 2; i < 6; ++i)
        {
            frame[i].verb = i == 2 ? 0 : 1;
            frame[i].x = frame[i].y = 100;
        }
        frame[6].verb = 3;
        plan.mode = 5;
        plan.x = plan.y = 100;
        plan.overlay = 7;
        edit = dvm_vector_local_create(&plan,source,frame);
        require(edit && dvm_vector_local_move(edit,200,300,output,outline) &&
            !memcmp(output,source,sizeof(source)) && outline[2].x == 100 && outline[4].x == 200 &&
            outline[4].y == 300,"marquee rectangle updates without changing document geometry");
        dvm_vector_local_destroy(edit);
    }
    puts("PASS local vector: detached modifier plan, independent controls, bindings, reconciliation and rollback");
    return 0;
}
