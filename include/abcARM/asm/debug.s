        SUBT    Helios Kernel debugging MACROs                  > debug/s
        ;       (c) 1990, Active Book Company, Cambridge, United Kingdom
        ; ---------------------------------------------------------------------
        ; ---------------------------------------------------------------------
        ; ---------------------------------------------------------------------
        ; started:      900420  JGSmith
        ;
        ; Debugging MACROs
        ; ---------------------------------------------------------------------
        ; ---------------------------------------------------------------------
        ; ---------------------------------------------------------------------

old_opt SETA    {OPT}
        OPT     (opt_off)

        !       0,"Including debug.s"


        ; ---------------------------------------------------------------------

        MACRO
$label  Regs
        BL      |.PutRegs|
        MEND

        MACRO
$label  Mark
        BL      |.PutRegs|
        MEND

        ; ---------------------------------------------------------------------

        OPT     (old_opt)

        ; ---------------------------------------------------------------------
        END     ; EOF debug/s
