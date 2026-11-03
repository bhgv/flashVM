/* Checked guarded-copy C tail entry, selected only by startup experiment. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "owner_fast.h"
#include "bytes_fast.h"
#include "shared_bytes.h"
#include "disvm/native_context.h"

extern void movb(void); /* Actual hosted byte copy opcode. */
extern void movw(void); /* Actual hosted native WORD copy opcode. */
extern void movl(void); /* Actual hosted LONG copy opcode. */
extern void movf(void); /* Actual hosted bit-preserving REAL copy opcode. */
extern void movm(void); /* Actual hosted variable-size raw copy opcode. */
extern void cvtbw(void); /* Actual guarded integer conversion opcode. */
extern void cvtwb(void); /* Actual guarded integer conversion opcode. */
extern void cvtws(void); /* Actual guarded integer conversion opcode. */
extern void cvtsw(void); /* Actual guarded integer conversion opcode. */
extern void cvtwl(void); /* Actual guarded integer conversion opcode. */
extern void cvtlw(void); /* Actual guarded integer conversion opcode. */
extern void headb(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headw(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headl(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headf(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headm(void); /* Actual guarded scalar/raw list-head opcode. */


/* [GC:] Native identity is checked before dispatch/operands. No cleanup, code
 * metadata read or retry follows a completed opcode or propagated VM error. */
int dvm_runtime_bytes_fast(REG *registers, int opcode)
{
    if (!dvm_runtime_guarded_copies || !dvm_runtime_owner_entry_ready(registers) ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    switch (opcode)
    {
    case IMOVB:
        movb();
        break;
    case IMOVW:
        movw();
        break;
    case IMOVL:
        movl();
        break;
    case IMOVF:
        movf();
        break;
    case IMOVM:
        movm();
        break;
    case ICVTBW:
        cvtbw();
        break;
    case ICVTWB:
        cvtwb();
        break;
    case ICVTWS:
        cvtws();
        break;
    case ICVTSW:
        cvtsw();
        break;
    case ICVTWL:
        cvtwl();
        break;
    case ICVTLW:
        cvtlw();
        break;
    case IHEADB:
        headb();
        break;
    case IHEADW:
        headw();
        break;
    case IHEADL:
        headl();
        break;
    case IHEADF:
        headf();
        break;
    case IHEADM:
        headm();
        break;
    default:
        return 0;
    }
    return 1;
}
