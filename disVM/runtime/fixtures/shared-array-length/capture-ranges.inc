/* Copyright (c) 2026 bhgv
 * EN: Range acquisition retry and address publication oracle.
 * RU: Oracle повторов захвата диапазонов и публикации адресов.
 * Extracted private oracle fragment; assembled only by its Python driver.
 */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    static Inst *busy_pc; /* First decoded index that reaches publication. */
    static int replayed; /* One matching address publication retry completed. */

    if (CONTROL == 0 && !active && !destination && !destination_size &&
        dvm_native_execution()->arithmetic_retry_opcode == ISLICELA && contend_capture(1))
    {
        return DVM_CELL_BUSY;
    }

    if (CONTROL == 0 && !active)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == ICVTCW || opcode == ICVTCL || opcode == ICVTCF)
        {
            size_t width = opcode == ICVTCW ? sizeof(WORD) :
                (opcode == ICVTCL ? sizeof(LONG) : sizeof(REAL));
            if (source != DVM_R.s || source_size != sizeof(String *) ||
                destination != DVM_R.d || destination_size != width)
            {
                fprint(2, "FAIL conversion must claim both decoded spans together\n");
                exit(10);
            }
            if ((opcode == ICVTCW && contend_capture(3)) ||
                (opcode == ICVTCL && contend_capture(4)) ||
                (opcode == ICVTCF && contend_capture(5)))
            {
                return DVM_CELL_BUSY;
            }
        }
    }

    if (CONTROL == 0 && !active && source && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == ICVTWC || opcode == ICVTLC)
        {
            String *previous;
            int stage;
            memmove(&previous, destination, sizeof(previous));
            stage = (opcode == ICVTWC ? 11 : 12) + (source == previous ? 2 : 0);
            if (contend_capture(stage))
            {
                return DVM_CELL_BUSY;
            }
        }
    }

    if (CONTROL == 0 && !active && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == ICVTFC && source)
        {
            String *previous;
            memmove(&previous, destination, sizeof(previous));
            if (contend_capture(source == previous ? 16 : 15))
            {
                return DVM_CELL_BUSY;
            }
        }
        if (!source && ((opcode == ICVTCA && contend_capture(17)) ||
                        (opcode == ICVTAC && contend_capture(18))))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (!source && opcode == ISLICEA)
        {
            WORD start, end;
            Array *previous;
            memmove(&start, DVM_R.s, sizeof(start));
            memmove(&end, DVM_R.m, sizeof(end));
            memmove(&previous, destination, sizeof(previous));
            int stage;
            if (start < 0 || end < 0)
            {
                stage = previous->t->np ? 21 : 26;
            }
            else
            {
                stage = previous->root == H ? 24 : 25;
            }
            if (contend_capture(stage))
            {
                return DVM_CELL_BUSY;
            }
        }
        if (source && opcode == ISLICELA &&
            ((source_size == 3 * sizeof(WORD) && contend_capture(22)) ||
             (source_size != 3 * sizeof(WORD) && contend_capture(23))))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && source && !destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if ((opcode == ICASE && contend_capture(27)) ||
            (opcode == ICASEL && contend_capture(28)) ||
            (opcode == ICASEC && contend_capture(29)) ||
            (opcode == IGOTO && contend_capture(30)))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && source && !destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if ((opcode == ISEND && contend_capture(31)) ||
            (opcode == IRECV && contend_capture(32)) ||
            (opcode == INEWCW && contend_capture(33)) ||
            (opcode == INEWCM && contend_capture(source == DVM_R.m ? 40 : 34)) ||
            (opcode == INEWCMP && contend_capture(source == DVM_R.m ? 41 : 35)) ||
            (opcode == INEWCB && contend_capture(36)) ||
            (opcode == INEWCL && contend_capture(37)) ||
            (opcode == INEWCF && contend_capture(38)) ||
            (opcode == INEWCP && contend_capture(39)))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && source && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (!getenv("DVM_TEST_RECEIVE_ERRORS") && ((ring_kind == 1 && ((opcode == ISEND && contend_capture(42)) ||
                                (opcode == IRECV && contend_capture(43)))) ||
            (ring_kind == 3 && ((opcode == ISEND && contend_capture(46)) ||
                                (opcode == IRECV && contend_capture(47))))))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (address_publication)
    {
        if (source || source_size || !destination || !destination_size)
        {
            fprint(2, "FAIL address publication acquired a private source guard\n");
            exit(10);
        }
        if (CONTROL == 0 && !replayed &&
            dvm_native_execution()->arithmetic_retry_opcode > 0)
        {
            if (!busy_pc)
            {
                busy_pc = DVM_R.PC;
                address_publication = 2;
                return DVM_CELL_BUSY;
            }
            if (busy_pc != DVM_R.PC)
            {
                fprint(2, "FAIL address publication replay changed PC\n");
                exit(10);
            }
            replayed = 1;
            print("PASS scheduled address publication retry\n");
        }
        address_publication = 2;
    }
    return __real_dvm_managed_ranges_begin(source, source_size, destination, destination_size, access);
}


/* Force Cycles after successful publication and before the guest consumes
 * the interior address. Only a real decoded index may suspend here. */
