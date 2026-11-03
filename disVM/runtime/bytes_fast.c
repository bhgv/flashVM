/* Checked guarded data-operation C tail entry, selected only by startup experiment. */
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
extern void mulb(void); /* Actual guarded product/quotient/remainder. */
extern void mulw(void); /* Actual guarded product/quotient/remainder. */
extern void mull(void); /* Actual guarded product/quotient/remainder. */
extern void divb(void); /* Actual guarded product/quotient/remainder. */
extern void divw(void); /* Actual guarded product/quotient/remainder. */
extern void divl(void); /* Actual guarded product/quotient/remainder. */
extern void modb(void); /* Actual guarded product/quotient/remainder. */
extern void modw(void); /* Actual guarded product/quotient/remainder. */
extern void modl(void); /* Actual guarded product/quotient/remainder. */
extern void addb(void); /* Actual guarded integer arithmetic opcode. */
extern void addw(void); /* Actual guarded integer arithmetic opcode. */
extern void addl(void); /* Actual guarded integer arithmetic opcode. */
extern void subb(void); /* Actual guarded integer arithmetic opcode. */
extern void subw(void); /* Actual guarded integer arithmetic opcode. */
extern void subl(void); /* Actual guarded integer arithmetic opcode. */
extern void andb(void); /* Actual guarded integer arithmetic opcode. */
extern void andw(void); /* Actual guarded integer arithmetic opcode. */
extern void andl(void); /* Actual guarded integer arithmetic opcode. */
extern void orb(void); /* Actual guarded integer arithmetic opcode. */
extern void orw(void); /* Actual guarded integer arithmetic opcode. */
extern void orl(void); /* Actual guarded integer arithmetic opcode. */
extern void xorb(void); /* Actual guarded integer arithmetic opcode. */
extern void xorw(void); /* Actual guarded integer arithmetic opcode. */
extern void xorl(void); /* Actual guarded integer arithmetic opcode. */
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
    case IMULB:
        mulb();
        break;
    case IMULW:
        mulw();
        break;
    case IMULL:
        mull();
        break;
    case IDIVB:
        divb();
        break;
    case IDIVW:
        divw();
        break;
    case IDIVL:
        divl();
        break;
    case IMODB:
        modb();
        break;
    case IMODW:
        modw();
        break;
    case IMODL:
        modl();
        break;
    case IADDB:
        addb();
        break;
    case IADDW:
        addw();
        break;
    case IADDL:
        addl();
        break;
    case ISUBB:
        subb();
        break;
    case ISUBW:
        subw();
        break;
    case ISUBL:
        subl();
        break;
    case IANDB:
        andb();
        break;
    case IANDW:
        andw();
        break;
    case IANDL:
        andl();
        break;
    case IORB:
        orb();
        break;
    case IORW:
        orw();
        break;
    case IORL:
        orl();
        break;
    case IXORB:
        xorb();
        break;
    case IXORW:
        xorw();
        break;
    case IXORL:
        xorl();
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
