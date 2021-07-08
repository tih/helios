;-------------------------------------------------------------------
;                                                    rectangles.s
;-------------------------------------------------------------------

; This is the 'rectangles' assembly part of the ABC drawing package for
;    the X-lookalike layer.

; Almost all graphics operations are performed by dividing the shape
;    to be drawn in the destination pixel map into rectangles (usually
;    one pixel high, ie. lines), and drawing these rectangles with
;    the patterning and masking described in the X graphics context

; The routines in this file manage various high-speed rectangle draw 
;    operations in bit-images of the format dictated by Dave in the 
;    Hercules document (screen layout).

; This file exports two symbols:
; _dpControlLookUp  : Look-up table for control     routines
; _dpCombineLookUp  : Look-up table for combination routines

; Also if debugging:
; _dpRectanglesStart : Start of this piece of code

;-------------------------------------------------------------------
;                                                    Print options
;-------------------------------------------------------------------

	OPT    32  ; SET,GBL,LCL directives not listed

;-------------------------------------------------------------------
;                                               The 'SELECT' macro
;-------------------------------------------------------------------

; This macro is used for string selection: It assigns $t to $l if
;   $c is true, otherwise it assigns $e to $l.

       MACRO
$l     SELECT  $c,$t,$e
       [ $c
$l     SETS   $t
       |
$l     SETS   $e
       ]
       MEND

;-------------------------------------------------------------------
;                                                externLabel macro
;-------------------------------------------------------------------

        MACRO
$label  externLabel
        [ heliosMode
$label  procsym
        |
        LCLS quotedLabel
quotedLabel SETS "$label"
quotedLabel SETS "|_" :CC: quotedLabel :CC: "|"
$quotedLabel
        ]
        MEND

;-------------------------------------------------------------------
;                                                   Helios control
;-------------------------------------------------------------------

; This file must assemble differently according to whether we are 
;    assembling for Helios or Unix: In Helios a proper Helios
;    module must be generated.

        GET    objects/giveMode.ss  ; Find out if in Helios mode
        
	; Conditionally include helios headers: We have to do this
	;    in the following way because you cannot have a 'GET' in
	;    the middle of a consditional peice of assembley:
	
        GBLS getCommand
getCommand  SELECT heliosMode,"GET",";"
        $getCommand heliosHdr/listopts.s
        $getCommand heliosHdr/arm.s
        $getCommand heliosHdr/basic.s
        $getCommand heliosHdr/structs.s
        $getCommand heliosHdr/module.s
        
        [ heliosMode                ; If in Helios Mode ...
        OPT    32                   ; Reset printer options
        IMPORT dpRectDebug          ; Import rectangle debugging
        StartModule rectangles,-1,0 ; Start of Module
        static                      ; Define externally availiable locations :
        static_extern_func dpLookUpControlAddress
        static_extern_func dpLookUpCombineAddress
        DATA               dpRectanglesStart,4 ; Initialized data
        ADR                ip,dpRealRectanglesStart
        STR                ip,[R2,#:MODOFF:dpRectanglesStart]
        static_end                  ; End of static area
        
        |                           ; If not in Helios :
        AOUT                        ; Output format directive
        AREA   rectanglesCode,CODE
        IMPORT  |_dpRectDebug|      ; Simply import/export symbols ...
|_dpRectanglesStart| DCD dpRealRectanglesStart
        EXPORT  |_dpRectanglesStart|
        EXPORT  |_dpLookUpControlAddress|
        EXPORT  |_dpLookUpCombineAddress|
        ]

dpRealRectanglesStart ; Start of section

;-------------------------------------------------------------------
;                                                'structures' file
;-------------------------------------------------------------------

; The following file contains the block-layout directives to determine
;   various structures shared by 'C' code and assembley code. This
;   file makes use of the 'BlitterControlBlock_t' structure

       GET    lochdr/structures.ss

;-------------------------------------------------------------------
;                                                 'Utilities' file
;-------------------------------------------------------------------

; The following file contains some useful macro defitions, and the
;   register aliases.
; Look in this file for defintions of 'RBC', 'NOR', 'ENR'
;   etc.

        GET    lochdr/utility.ss

;--------------------------------------------------------------------
;                                           The diagnostics routine
;--------------------------------------------------------------------

; To place a breakpoint in the code, a macro compiles to some
;    instructions which save the pc and lr registers, loads
;    the return address (past the macro), without flags into lr,
;    and branches to this routine, such that the saved pc points
;    to the diagnositcs message and value code. This routine saves
;    all the rest of the registers in the correct space in the
;    blitter control block, restores the stack frame, calls a 'C'
;    code to implement the diagnostics, and on return from this routine,
;    restores the registers and returns to the code.

doDiagnostics
        ; No load/store multiples are used in order to avoid
        ;   having to change the value of sp in the process:
        STR    R0, [sp,#cbRegBlock+ 0*4]
        STR    R1, [sp,#cbRegBlock+ 1*4]
        STR    R2, [sp,#cbRegBlock+ 2*4]
        STR    R3, [sp,#cbRegBlock+ 3*4]
        STR    R4, [sp,#cbRegBlock+ 4*4]
        STR    R5, [sp,#cbRegBlock+ 5*4]
        STR    R6, [sp,#cbRegBlock+ 6*4]
        STR    R7, [sp,#cbRegBlock+ 7*4]
        STR    R8, [sp,#cbRegBlock+ 8*4]
        STR    R9, [sp,#cbRegBlock+ 9*4]
        STR    R10,[sp,#cbRegBlock+10*4]
        STR    R11,[sp,#cbRegBlock+11*4]
        STR    R12,[sp,#cbRegBlock+12*4]
        STR    R13,[sp,#cbRegBlock+13*4]

        ; Now to get some semblance of a proper stack frame, and
        ;    call the 'C' routine:
        MOV    ip,pc                    ; pc -> ip
        MOV    R0,#2_11                 ; Get flags mask in R0 ...
        ORR    R0,R0,#2_1111 :SHL: 28   ; ... continued
        AND    ip,ip,R0                 ; Get only pc flags in ip
        BIC    lr,lr,R0                 ; Clear flags in lr
        ORR    lr,lr,ip                 ; Save pc flags in lr
        STR    lr,[sp,#cbDiagReturn]    ; Save breakpoint return address
        MOV    R0,sp                    ; Parameter: control block pointer
        LDR    sp,[sp,#cbStkPtr]        ; Load old stack pointer
        LDMIA  sp,{R4-R9,sl,fp,lr}      ; Load old registers
        STMFD  sp!,{R0}                 ; Save control block pointer
        [ heliosMode
        BL     dpRectDebug              ; Call 'C' routine
        |
        BL     |_dpRectDebug|           ; Call 'C' routine
        ]
        LDMFD  sp!,{R0}                 ; Get control block pointer
        MOV    sp,R0                    ; Move it back into sp
        
        LDR    R0, [sp,#cbRegBlock+ 0*4]
        LDR    R1, [sp,#cbRegBlock+ 1*4]
        LDR    R2, [sp,#cbRegBlock+ 2*4]
        LDR    R3, [sp,#cbRegBlock+ 3*4]
        LDR    R4, [sp,#cbRegBlock+ 4*4]
        LDR    R5, [sp,#cbRegBlock+ 5*4]
        LDR    R6, [sp,#cbRegBlock+ 6*4]
        LDR    R7, [sp,#cbRegBlock+ 7*4]
        LDR    R8, [sp,#cbRegBlock+ 8*4]
        LDR    R9, [sp,#cbRegBlock+ 9*4]
        LDR    R10,[sp,#cbRegBlock+10*4]
        LDR    R11,[sp,#cbRegBlock+11*4]
        LDR    R12,[sp,#cbRegBlock+12*4]
        LDR    R13,[sp,#cbRegBlock+13*4]
        
        LDR    lr,[sp,#cbDiagReturn]      ; Return to breakpoint ...
        MOVS   pc,lr                      ; ... continued

;--------------------------------------------------------------------
;                                             The diagnostics macro
;--------------------------------------------------------------------

; The following macro is a generic macro which comiles a breakpoint
;   into the code. The parameter $value is an action code which is
;   picked up by the 'C' routine (in debug.c) which uses it to
;   determine the nature of the diagnostics printed, and the
;   parameter $message is a string which is printed out as the
;   diagnostics message.

        MACRO
$label  DIAGNOSTICS   $value,$message
        STR    lr,[sp,#cbRegBlock+14*4]  ; Save link register
        ADR    lr,%F01                   ; Get return address
        STR    pc,[sp,#cbRegBlock+15*4]  ; Save PC (points to message)
        B      doDiagnostics             ; Call wrapper code
        &      $value                    ; Value parameter goes here
        =      $message,0                ; Message goes here
        ALIGN                            ; Align to word boundary
01      ; Return here                    ; Return here
        LDR    lr,[sp,#cbRegBlock+14*4]  ; Reload link register
        MEND

;--------------------------------------------------------------------
;                                             The 'DIAG' etc macros
;--------------------------------------------------------------------

; These macros are invokations of the appropriate 'DIAGNOSTICS' macro
;   with the correct action code

; The following macros can be used at most places in the code
;   namely where the registers R4-R9,sl,fp, and lr have been
;   stored on a full decrementing stack for which the subsequent
;   value of the pointer has been stored in the offset 'cbStkPtr'
;   of a control block whose address is pointed to by sp.

; It accepts a string message and a numeric value as parameters
;    and essentially causes a branch to some 'C' code which 
;    prints out diagnostic information, and then acts according
;    to the parameter:
; 0 => Return to code
; 1 => Prompt then return to code
        
        MACRO
$label  BKPT   $msg        ; Breakpoint macro : prompt before continuing
        DIAGNOSTICS 0,$msg
        MEND
        
        MACRO
$label  DIAG   $msg       ; Diagnostics macro : continue immediately
        DIAGNOSTICS 1,$msg
        MEND
        
        MACRO
$label  SDIAG  $msg       ; Short diagnostics macro
        DIAGNOSTICS 2,$msg
        MEND
        
;-------------------------------------------------------------------
;                                            How the blitter works
;-------------------------------------------------------------------

; There is a structure 'BlittingControlBlock_t' which is used to pass
;   information to the routines in this file.

; The routines in this file are executed by indirecting to the
;   appropriate 'control routine' through one of the pointers
;   in a validly set-up 'blitter control block' whose structure
;   is declared in the file 'lochdr/structures.sh' for assembly
;   code files, and 'lochdr/code_interface.h' for 'c' files.

; The blitting control block is set up primarily by a routine in the
;   file 'gc_decoder.c' which decodes the information in an X graphics
;   context and writes it into the control block. After this decoding 
;   routine has executed, the control block can be used repeatedly to
;   draw any amount of rectangles into the destination pixmap according
;   to that graphics context simply by calling 'execBlitter' repeatedly
;   with the control block as a parameter, but changing the co-ordinates 
;   of the destination rectangle each time.

; The only labels exported by this file are those for look-up tables
;   which are used to look-up the addresses of the appropriate control
;   and combination routines. The file 'gc_decoder.c' uses these
;   tables in order to create a valid blitter control block.

;--------------------------------------------------------------------
;                                                Pixel-Map formats 
;-------------------------------------------------------------------

; The format of bit-images is as follows:

; An n-bit-per-pixel bit image of size x by y is organised as <y>
;    contigous groups, each group being a contiguous sequence of
;    <n> vectors each an integer number of 32-bit words long, and
;    being a bit field consisting of <x> bits packed into contiguous 
;    locations in memory in the usual way, ie. in the low bit - high
;    bit fashion, there may be any fixed amount of spare words at
;    the end of the bit-field. Any extra words or bits on the end
;    of a bit field are undefined.
; The bit which represents the value of the bit in plane <n'> of the
;    pixel at location (x',y') of the bit image is found by examining
;    the n'th vector of the y'th grouping and looking at bit x' of that
;    bit field, ie. bit (x' mod 32) of the (x' div 32)'th word.

;-------------------------------------------------------------------
;                                             The Control Routines
;-------------------------------------------------------------------

; The X graphics context, whatever it may be, can be decoded into an
;    operation where one of the 16 possible combination modes is
;    chosen for each colour plane of the destination bit-image, and
;    for each colour plane in the destination bit-image, the source
;    (if applicable,) rectangle is combined with the destination
;    recatangle (through a mask, if applicable). The exact way in
;    which this is done is described in the file 'gc_decoder'. The
;    function of this code however is to execute this lower level
;    information.

; Thus there is a 'combination vector' in the 'BlitterControlBlock_t'
;    which contains an entry for each colour plane of the destination
;    bit-image, that entry being the address of a 'combination routine'
; Each 'combination routine' is designed to combine a row of the
;    destination pixel-map with a row of the source pixel map, through
;    a mask (if applicable) and write to the destination pixel map.
; The combination routines are divided into two major categories:
;    the ones that apply a mask bit-map, and the ones that do not apply
;    a mask bit-map. These two major categories are each sub-divided 
;    into two minor categories: those that write from left to right and 
;    those that write from right to left. Each of the four minor categories
;    are then divided into 16 categories, one for each way of combining 
;    two binary variables. Thus there are 64 different combination routines.

; The function of the control routine is to divide the destination
;    bit-image into manageable horizontal rows of pixels, and for
;    each row and destination pix-map colour plane to set-up
;    registers and indirect through the 'combination vector' to 
;    the appropriate combination routine for that colour plane.

; The control routines are divided up as listed below:

; 1. Unary operations (simplest type)
; This covers operations where no source or mask pixel map is
;    involved. The combination vector entries must point to the 
;    type of combination routine where no mask is involved, and
;    which write from left to right, and must be one of the four
;    combination routines which write to the destination
;    independently of the source bit-map (ie. invert destination,
;    no operation, write 0 or write 1). For each row of the
;    destination rectangle, and each plane, the control routine
;    indirects to the appropriate combination routine.

; 2. Binary operations, stippled (four types)
; This covers operations where no mask bit-map is supplied but a
;   a source pix-map is supplied with one particular colour plane
;   from it picked out. The routine divides the destination
;   rectangle up into horoizontal rows of pixel such that each row
;   is mapped from a part of the source pix-map which does not
;   cross any tiling boundaries, and for each colour plane, the
;   appropriate combination routine is called. The same fixed source
;   pixel-map colour plane is pointed to for each colour plane of
;   the destination pixel map. There are four types of thisroutine
;   according to whether the destination is to be written up-down or
;   down-to-up, and left-to-right or right-to-left. In the case
;   of the left-to-right versions, the combination routines must 
;   be of the type which write left-to-right with no mask bit-map.
;   In the case of the right-to-left, the combination routines must
;   be of the type which write right-to-left with no mask pix-map.

; 3. Binary operations, unstippled (four types)
; This is the same as the binary operations, stippled, above, except
;   that the colour-plane from the source pix-map is un-fixed,
;   and is the same as the colour-plane from the destination pix-map.

; 4. Tertiary operations, stippled (four types)
; Same as the binary operations, except a mask bit-map is also used
;   and different combination routines must obviously therefore
;   be used.

; 5. Tertiary operations, unstippled (four types)
; Similar to case 3 except a mask bit-map is supplied.

; All control routines deal with the tiling boundaries in the following
;    way ... first, the destination is divided into vertical strips,
;    called 'columns', at the vertical tiling boundaries of the source
;    pix-map. The columns are picked from left-to-right or right-to-
;    left according to the type of control routine. Each vertical
;    column is divided vertically into wedges at the horizontal tiling
;    boundaries of the source pix-map, and these wedges are picked
;    out from top-to-bottom or bottom-to-top according to the type
;    of control routine. Each wedge is then divided into rows, the rows
;    being picked out from top-to-bottom or bottom-to-top accordingly,
;    and each row is then divided into colour planes which are individually
;    plotted by indirecting to the combination routine via. the combination
;    routine vector. It is the responsibility of the routine in 
;    'gc_decoder' to get the right type of combination routine because
;    the registers are set to expect the correct type: ie. left-right
;    versus right-left, and apply mask versus don't apply mask

;--------------------------------------------------------------------
;                                Look-up table for control routines
;--------------------------------------------------------------------

; The following is a table of control routines used by the routines
;    in 'gc_decoder.c' to get a pointer to the appropriate control
;    routine.

; It is a table of pointers to functions which obey the procedure
;    call standard and accept as a single parameter a pointer to
;    the control block which they themselves were indirected to from.

; None of the routines in the table are directly accessible from
;    other files as their names are not exported.

; The offsets into the following look-up table are encoded
;   as follows:
;
; Bits 3,4 : 00 => Unary, 01 => Binary, 10 => Tertiary, 11 => Undefined
; Bit  2   : 0  => UnStippled     1  => Stippled
; Bit  1   : 0  => Top-to-Bottom  1  => Bottom-to-Top
; Bit  0   : 0  => Left-to-Right  1  => Right-to-Left

controlLookUp

        &      ctlUnary          - controlLookUp    ; 00000
        &      ctlUnary          - controlLookUp    ; 00001
        &      ctlUnary          - controlLookUp    ; 00010
        &      ctlUnary          - controlLookUp    ; 00011
        &      ctlUnary          - controlLookUp    ; 00100
        &      ctlUnary          - controlLookUp    ; 00101
        &      ctlUnary          - controlLookUp    ; 00110
        &      ctlUnary          - controlLookUp    ; 00111
        
        &      ctlBinaryLrTbUs   - controlLookUp    ; 01000
        &      ctlBinaryRlTbUs   - controlLookUp    ; 01001
        &      ctlBinaryLrBtUs   - controlLookUp    ; 01010
        &      ctlBinaryRlBtUs   - controlLookUp    ; 01011
        &      ctlBinaryLrTbSt   - controlLookUp    ; 01100
        &      ctlBinaryRlTbSt   - controlLookUp    ; 01101
        &      ctlBinaryLrBtSt   - controlLookUp    ; 01110
        &      ctlBinaryRlBtSt   - controlLookUp    ; 01111
        
        &      ctlTertiaryLrTbUs - controlLookUp   ; 11000
        &      ctlTertiaryRlTbUs - controlLookUp   ; 11001
        &      ctlTertiaryLrBtUs - controlLookUp   ; 11010
        &      ctlTertiaryRlBtUs - controlLookUp   ; 11011
        &      ctlTertiaryLrTbSt - controlLookUp   ; 11100
        &      ctlTertiaryRlTbSt - controlLookUp   ; 11101
        &      ctlTertiaryLrBtSt - controlLookUp   ; 11110
        &      ctlTertiaryRlBtSt - controlLookUp   ; 11111
        
;-------------------------------------------------------------------
;                                            Control table look-up
;-------------------------------------------------------------------

; This function is used in 'gc-decoder.c' to simply look-up in the
;    control routines table a particular control routine and compute
;    it's address. It was written in assembley to get over some
;    casting problems in ANSI C.

        ; At head of file : EXPORT |_dpLookUpControlAddress|

dpLookUpControlAddress externLabel

        ADR    ip,controlLookUp     ; Get control lookup address
        LDR    R0,[ip,R0,LSL#2]     ; Load address offset
        ADD    R0,R0,ip             ; Add to base of table
        MOVS   pc,lr                ; Return

;-------------------------------------------------------------------
;                                        The unary control routine
;-------------------------------------------------------------------

; This is the code for the unary control routine, which is the
;   simplest of all the control routines, and is the only one
;   which is not compiled via a macro

ctlUnary
        STMFD  sp!,{R4-R9,sl,fp,lr} ; Save registers
        STR    sp,[R0,#cbStkPtr]    ; Save stack pointer
        MOV    sp,R0                ; Point to control block
        
        LDR    R5,[sp,#cbDesRgtX]   ; Get destination right X
        LDR    R6,[sp,#cbDesLftX]   ; Get destination left X
        CMPS   R5,R6                ; Check if zero/neg block size ...
        BLE    unaryControlExit     ; ... exit if so

        LDR    R0,[sp,#cbDesBotY]   ; Get destination bottom Y
        LDR    R1,[sp,#cbDesTopY]   ; Get destination top Y
        SUBS   R0,R0,R1             ; Get number of rows
        BLE    unaryControlExit     ; Branch if zero/neg block size

        AND    ip,R6,#2_11111       ; Get bit offset, fst des word
        MVN    R9,#0                ; Compute first word mask ...   
        MOV    R8,R9,LSL ip         ; ... continued
        AND    ip,R5,#2_11111       ; Compute last word mask ...
        MVN    R9,R9,LSL ip         ; ... continued

        MOV    R6,R6,LSR#5          ; Compute wrd offset of des wrd
        LDR    R11,[sp,#cbDesWPV]   ; Load destination words per vector
        LDR    ip,[sp,#cbDesBPP]    ; Load destination pixmap depth
        MUL    lr,ip,R11            ; Compute destination words per line
        MUL    R2,lr,R1             ; Compute des start row offset
        LDR    R3,[sp,#cbDesBase]   ; Load destination base address
        ADD    R3,R3,R2,LSL#2       ; Compute des start row address
        ADD    R3,R3,R6,LSL#2       ; Compute first des word address
        
        RSB    R4,R6,R5,LSR#5       ; Compute mid word count +1
        SUB    R11,R11,R4           ; Compute des ptr increment
        SUBS   R4,R4,#1             ; Get MWC and N flag
        ANDMI  R8,R8,R9             ; Adjust masks if -1
        ORR    R4,R4,R4,LSL#16      ; Set up MWC properly

        LDR    R1,[sp,#cbDesBPP]    ; Load bit-planes in des
        
        MOV    lr,pc                    ; Set up flags, and return ...
        ADD    lr,lr,#  unyReturn - unyOffset
unyOffset                               ; ... address in lr.
        ADR    R7,unyReturn

;--------------------------------------------------------------------+
unyRowLoop                          ; Once per row                   |
        BIC    R1,R1,#(&FF :SHL: 16) ; Clear plane count             |
;------------------------------------------------------------------+ |
unyPlaneLoop                        ; Once per bit plane           | |
        LDR    R7,[R3]              ; Preload first dest word      | |
        TEQP   lr,#0                ; Set up N flag                | |
        LDR    pc,[sp,R1,LSR#14]    ; Call routine from vector     | |
unyReturn                           ; Return here                  | |
        STR    R5,[R3]              ; Save last destination word   | |
        AND    R4,R4,R4,ROR#16      ; Restore mid word count       | |
        ADD    R3,R3,R11,LSL#2      ; Des ptr to next bit plane    | |
        ADD    R1,R1,#(1 :SHL: 16)  ; Increment and branch ...     | |
        CMPS   R1,R1,LSL#16         ; ... continued ...            | |
        BCC    unyPlaneLoop         ; ... continued                | |
;------------------------------------------------------------------+ |
        [ {FALSE} ; heliosMode
        TSTS   R0,#&03              ; Helios bodge ...
        B      unyHeliosBodge       ; Give interrupt enable reqularly
unyHeliosBodgeReturn                ; Return here
        ]
        SUBS   R0,R0,#1             ; Decrement and branch ...       |
        BNE    unyRowLoop           ; ... continued                  |
;--------------------------------------------------------------------+

unaryControlExit
        LDR    sp,[sp,#cbStkPtr]    ; Restore stack pointer
        LDMFD  sp!,{R4-R9,sl,fp,pc}^; Restore regs and rtn with flags

        [ {FALSE} ; heliosMode                ; Here to perform bodge        
unyHeliosBodge
        ADD    sp,sp,#cbRegBlock    ; Point to reg block
        STMIA  sp,{R0-R15}          ; Save all registers
        MOV    R1,#&740000          ; Debugging line on screen ...
        STR    R0,[R1]              ; ... continued
        SUB    R0,sp,#cbRegBlock    ; Get control block pointer in R4
        LDR    sp,[R0,#cbStkPtr]    ; Get real sp
        LDMFD  sp,{R4-R9,sl,fp}     ; Get acceptable registers
        MOV    R1,pc                ; Load R1 with pc flags
        BIC    R1,R1,#&0C000000     ; Enable interrupt bits
        TEQP   pc,R1                ; Enable interrupts
        AND    R0,R0,R0             ; NOP
        AND    R0,R0,R0             ; NOP
        AND    R0,R0,R0             ; NOP
        LDR    R1,[R0,#cbRegBlock+4*15] ; Get old PC (to restore flags)
        TEQP   pc,R1                ; Restore flags
        ADD    R0,R0,#cbRegBlock    ; Point to register block
        LDMIA  R0,{R0-R14}          ; Restore most of registers
        SUB    sp,sp,#cbRegBlock    ; Point to control block
        B      unyHeliosBodgeReturn ; Return to code
        ]                           ; End of story
        
;-------------------------------------------------------------------
;                                          Binary control routines
;-------------------------------------------------------------------

; This section includes a wrapper macro which wraps the macros which
;   compile binary control functions with a procedure call standard,
;   and then this macro is invoked several times to compile the macros
;   themselves ...

; Register usage is as follows:

;          Entry                    Exit
; R0  -    Ptr to control block     Undefined
; R1  -    Undefined                Undefined
; R2  -    Undefined                Undefined
; R3  -    Undefined                Undefined
; R4  -    Undefined                  Preserved
; R5  -    Undefined                  Preserved
; R6  -    Undefined                  Preserved
; R7  -    Undefined                  Preserved
; R8  -    Undefined                  Preserved
; R9  -    Undefined                  Preserved
; R10 -    Undefined                  Preserved
; R11 -    Undefined                  Preserved
; ip  -    Undefined                Undefined
; sp  -    Stack pointer              Preserved
; lr  -    Return address w flags   Undefined
; pc  -    PC + no special flags      lr including flags

        MACRO
$l      binaryControl_Pcs $h,$v,$s
        
        LCLS   id
id      SETS   $h :CC: $v :CC: $s

$l.$id
        STMFD  sp!,{R4-R9,sl,fp,lr}    ; Save registers
        STR    sp,[R0,#cbStkPtr]       ; Save stack pointer
        MOV    sp,R0                   ; Point to control block
bc      binaryControl $h,$v,$s         ; Routine goes here
        LDR    sp,[sp,#cbStkPtr]       ; Restore stack pointer
        LDMFD  sp!,{R4-R9,sl,fp,pc}^   ; Restore regs and rtn with flags
        
        MEND
        
ctlBinary binaryControl_Pcs "Lr","Tb","Us"
ctlBinary binaryControl_Pcs "Lr","Bt","Us"
ctlBinary binaryControl_Pcs "Rl","Tb","Us"
ctlBinary binaryControl_Pcs "Rl","Bt","Us"

ctlBinary binaryControl_Pcs "Lr","Tb","St"
ctlBinary binaryControl_Pcs "Lr","Bt","St"
ctlBinary binaryControl_Pcs "Rl","Tb","St"
ctlBinary binaryControl_Pcs "Rl","Bt","St"

;-------------------------------------------------------------------
;                                    Binary control routines macro
;-------------------------------------------------------------------

; This macro is used to compile each of the eight control routines
;   used for binary operations.

; The macro accepts three parameters: $h,$v, and $s as follows:
; $h :   "Lr" => left-to-right ; "Rl" =>right-to-left
; $v :   "Tb" => top-to-bottom ; "Bt" =>bottom-to-top
; $s :   "Us" => not stippled  ; "St" =>stippled

; This macro compiles code which splits the destination rectangle
;   up into vertical columns as desribed above, and sets up registers
;   it invokes a macro from within the body of the loop which then
;   splits the vertical column into horizontal wedges, then into
;   rows, and splits the rows into colour planes. That macro could
;   well have been written within this macro but was separated
;   to keep the code sections small and reasonable.

; Register usage is as follows:

;          Entry                         Exit
; R0  -    Undefined                     Undefined
; R1  -    Undefined                     Undefined
; R2  -    Undefined                     Undefined
; R3  -    Undefined                     Undefined
; R4  -    Undefined                     Undefined
; R5  -    Undefined                     Undefined
; R6  -    Undefined                     Undefined
; R7  -    Undefined                     Undefined
; R8  -    Undefined                     Undefined
; R9  -    Undefined                     Undefined
; R10 -    Undefined                     Undefined
; R11 -    Undefined                     Undefined
; ip  -    Undefined                       Preserved
; sp  -    Ptr to control block            Preserved
; lr  -    Return address and flags      Undefined
; pc  -    Flags not specified             at end of macro, flags destroyed

;--------------------------------- Macro defintion

        MACRO
$label  binaryControl $h,$v,$s

        ; Create a local string variable to append to each label
        ;   to make them unique
        
        LCLS   id
id      SETS   $h :CC: $v :CC: $s

        ; Label the code with the identifier sppended to the label
$label.$id

        ; Create a local string variable to determine the instruction
        ;   to use to compute the source/destination increment/decrement
        ;   quantities ...
        
        LCLS   add_sub
add_sub SELECT (($h="Lr":LAND:$v="Tb"):LOR:($h="Rl":LAND:$v="Bt")),"SUB","ADD"

        ; Compute instructions to use when computing first and last
        ;    word masks:
        
        LCLS   mvn_mov
        LCLS   mov_mvn
mvn_mov SELECT $h="Lr","MVN","MOV"
mov_mvn SELECT $h="Lr","MOV","MVN"

        ; Compute the instruction to use to find the mid word count (+1)
        
        LCLS   rsb_sub
rsb_sub SELECT $h="Lr","RSB","SUB"

        ; The following macro reads and computes the various co-ordinate
        ;   information, and adjusts the source-to-destination offsets
        ;   by the appropriate amounts so that the source co-ordinates
        ;   lie within their appropriate residue range. It returns with
        ;   various registers set-up with the various parameters. The
        ;   macro will cause a branch to the label given as one of the
        ;   parameters if the destination rectangle is zero or negatively
        ;   sized
        
        getCoordinates exitBinaryControl$id,"Um",$h,$v,$s

        ; Now we get:

        ; R5  - Block  size  : X
        ; R6  - Des    start : X
        ; R7  - Source start : X (correct residue mod source X size)

        ; R8  - Block  size  : Y
        ; R9  - Des    start : Y
        ; R10 - Source start : Y (correct residue mod source Y size)
        
        ; Note: The bottom and right hand limit co-ordinates are in
        ;    fact the co-ordintes plus 1 of the rectangle to be plotted

        STR    R10,[sp,#cbSrcFstY]  ; Save source start Y co-ordinate
        STR    R5,[sp,#cbBlkSizX]   ; Save block X size
        STR    R8,[sp,#cbBlkSizY]   ; Save block Y size
        LDR    R1,[sp,#cbDesBPP]    ; Load destination bits per pixel

        LDR    R3,[sp,#cbDesWPV]    ; Load destination words per vector
        MUL    ip,R1,R3             ; Compute destination words per line
        MUL    R2,ip,R9             ; Compute des start row offset
        [ $v="Bt"                   ; If bottom-to-top
        SUB    R2,R2,R3             ; Get last bit plane of last row
        ]                           ; ... End Of Condition
        LDR    R3,[sp,#cbDesBase]   ; Load destination base address
        ADD    R2,R3,R2,LSL#2       ; Get des start row address
        STR    R2,[sp,#cbDesFirst]  ; Save des start row address
        
        LDR    R3,[sp,#cbSrcWPL]    ; Load source words per line
        MUL    R2,R3,R10            ; Compute src start row offset
        [ $v = "Bt"                 ; If bottom-to-top ...
        LDR    ip,[sp,#cbSrcWPV]    ; Get source words per vector
        SUB    R2,R2,ip             ; Get last bit plane of last row
        ]                           ; ... End Of Condition
        LDR    R3,[sp,#cbSrcBase]   ; Load source base address
        ADD    R2,R3,R2,LSL#2       ; Compute src start row address
        STR    R2,[sp,#cbSrcFirst]  ; Save source start row address
        
        [ $v = "Bt"                 ; If plotting bottom-to-top ...
        SUB    R10,R10,#1           ; Get correct bottom y start value,
        SUB    R9,R9,#1             ;    for source and destination
        ]                           ; ... End Of Condition

        MOV    R1,R1,LSL#16         ; Move BPP into bits 16-23

        [ $s = "St"                 ; If stippled ...
        LDR    R10,[sp,#cbSrcWPV]   ; ... load words per vector as
        ]                           ; source increment ... End of condition
        
;-------------------------------------------------------------------+
binColLoop$id                       ; Once per column generated     |
        ; R5 - Remaining amount of columns                          |
        ; R6 - Next  column : Destination (+1 if right-to-left)     |
        ; R7 - Next  column : Source      (+1 if right-to-left)     |
        [ $h="Lr"                   ; If left-to-right ...          |
        LDR    R8,[sp,#cbSrcSizX]   ; Load src pixmap X size        |
        SUB    R8,R8,R7             ; Compute max no of columns     |
        SUBS   R4,R5,R8             ; Compute remainder             |
        MOVGT  R5,R8                ; Find no of columns this time  |
        |                           ; ... Else If right-to-left ... |
        SUBS   R4,R5,R7             ; Compute remainder columns     |
        MOVGT  R5,R7                ; Determine columns this time   |
        ]                           ; ... End Of Condition          |
        STR    R4,[sp,#cbBlkRemX]   ; Save remainder of columns     |
        [ $h = "Lr"                 ; If left to right ...          |
        ADD    R5,R6,R5             ; Add to compute end column     |
        |                           ; ... Else if right to left ... |
        SUB    R5,R6,R5             ; Subtract to compute end column|
        ]                           ; ... End Of Condition          |
        STR    R5,[sp,#cbDesPosX]   ; And save as next start pos    |
        ; R5 - End column for destination                           |
        ; R6 - Start column for destination                         |
        ; R7 - Start column for source                              |
        AND    R0,R6,#2_11111       ; Get bit offset, fst des word  |
        MVN    R9,#0                ; Compute first word mask ...   |
        $mov_mvn R8,R9,LSL R0       ; ... continued                 |
        AND    ip,R5,#2_11111       ; Compute last word mask ...    |
        $mvn_mov R9,R9,LSL ip       ; ... continued                 |
        MOV    R3,R6,LSR#5          ; Compute wrd offset of des wrd |
        $rsb_sub R4,R3,R5,LSR#5     ; Compute mid word count +1     |
        SUB    R2,R7,R0             ; Get source offset for first   |
        ADD    R2,R2,#31            ;  source word +1 ...           |
        MOV    R2,R2,LSR#5          ; ... continued                 |
        ; R2 - First source word +1, offset into row (in words)     |
        ; R3 - First dest word offset in row                        |
        ; R4 - Mid word count +1, not special format                |
        ; R6 - First destination column                             |
        ; R7 - Start column for source                              |
        ; R8 - First word mask                                      |
        ; R9 - Last word mask                                       |
        LDR    ip,[sp,#cbDesFirst]  ; Get base addr, first des row  |
        ADD    R3,ip,R3,LSL#2       ; Compute first des word address|
        [ $s = "Us"                 ; If unstippled ...             |
        LDR    R10,[sp,#cbSrcWPV]   ; compute source increment or   |
        $add_sub R10,R10,R4         ;   decrement ...               |
        $add_sub R10,R10,#1         ; ... continued                 |
        ]                           ; ... End Of Condition          |
        LDR    R11,[sp,#cbDesWPV]   ; Compute destination increment |
        $add_sub R11,R11,R4         ;   or decrement                |
        SUB    R7,R6,R7             ; Compute 32-shift count ...    |
        AND    R0,R7,#2_11111       ; ... continued                 |
        RSB    R7,R0,#32            ; Compute shift count           |
        BIC    R1,R1,#2_1111111     ; Clear old shift count         |
        ORR    R1,R1,R7             ; Load new shift count          |
        SUBS   R4,R4,#1             ; Get MWC and N flag            |
        ANDMI  R8,R8,R9             ; Adjust masks if -1            |
        ORR    R4,R4,R4,LSL#16      ; Set up MWC properly           |
        ; At this point, the registers are set up satisfactorily    |
        ;    for the processing of a single column.                 |
        binaryColumn $h,$v,$s       ; Process this column           |
        LDR    R5,[sp,#cbBlkRemX]   ; Load remaining columns        |
        LDR    R6,[sp,#cbDesPosX]   ; Load next dest position       |
        [ $h = "Lr"                 ; If going left-right ...       |
        MOV    R7,#0                ; Next source is leftmost       |
        |                           ; ... Otherwise ...             |
        LDR    R7,[sp,#cbSrcSizX]   ; Next source is rightmost      |
        ]                           ; ... End Of Condition          |
        CMPS   R5,#0                ; Test remainder                |
        BGT    binColLoop$id        ; Loop if more columns          |
;-------------------------------------------------------------------+

exitBinaryControl$id                ; Exit point here

        MEND                        ; End Of Macro

;-------------------------------------------------------------------
;                                 Binary column control code macro
;-------------------------------------------------------------------

; This macro will assemble a section of code which blits a column of
;    the destination pixel map, where that column is mapped from a
;    part of the source pixel map which does not cross any vertical
;    tiling boundaries.

; The macro accepts three parameters to identify which section of code
;   to compile:
; $h :   "Lr" => left-to-right ; "Rl" =>right-to-left
; $v :   "Tb" => top-to-bottom ; "Bt" =>bottom-to-top
; $s :   "Us" => not stippled  ; "St" =>stippled

; The routine works by three nested loops.

; In the outer loop, the vertical column is split horizontally into
;    blocks at the horizontal tiling boundaries of the source pixel
;    map. Each of these blocks is then drawn separately by the next
;    loop which splits the block into horizontal lines, and the inner
;    most loop which splits the horizontal lines up into the separate
;    bit-planes, and called the appropriate combination routine for
;    that bit-plane.

; The register usage is as follows:

; R0  -   Bits  0 -  7 : 32 - Shift Count ) Preserved
;         Bits  8 - 31 : Undefined        ) Undefined
; R1  -   Bits  0 -  7 : Shift Count      ) Preserved
;         Bits  8 - 15 : Zeros            ) Preserved
;         Bits 16 - 21 : bits per pixel   ) Preserved
;         Bits 22 - 23 : Zeros            ) Preserved
;         Bits 24 - 31 : Undefined        ) Undefined
; R2  -   Fst src word + 1, wrd ofst in row   Undefined
; R3  -   Ptr to first des word               Undefined
; R4  -   Mid word count, special format    Preserved
; R5  -   Undefined                           Undefined
; R6  -   Undefined                           Undefined
; R7  -   Undefined                           Undefined
; R8  -   First word mask                     Undefined
; R9  -   Last Word mask                      Undefined
; R10 -   Source increment/decrement        Preserved
; R11 -   Destination increment/decrement     Undefined
; ip  -   Undefined                           Undefined
; sp  -   Control Block Pointer             Preserved
; lr  -   Undefined                           Undefined
; pc  -   'N' flag set if MWC=-1            at end of macro, flags destroyed

; Further Co-ordinate information :
; [sp,#cbSrcFstY]   : src row for fst des row, ( +1 if bottom-to-top )
; [sp,#cbBlkSizY]   : y size of destination column
; [sp,#cbSrcSizY]   : y size of source pixmap

; Further Address information :
; [sp,#cbSrcBase]   : ptr to top    source row (for top-bottom plotting)
; [sp,#cbSrcLast]   : ptr to bottom source row (for bottom-top plotting)
; [sp,#cbSrcFirst]  : ptr to src row from which fst des row is mapped

; Note that a special way of storing and using the counts for the
;   number of bit planes (bits-per-pixel,) and number of rows in
;   any particular horizontal wedge are stored in bits 8-31 of
;   registers R0 and R1. Note in the code below how R1 is used to
;   count up from zero to the number of bits-per-pixel using part
;   of itself to store the limit value, and is then reset by a single
;   instruction.

;------------------ The macro body -------------------

        MACRO
$label  binaryColumn $h,$v,$s

        ; Create a local string variable to append to each label
        ;   to make them unique
        
        LCLS   id
id      SETS   $h :CC: $v :CC: $s

        ; Set increment/decrement according as to plotting top-bottom
        ;    or bottom-top:
        
        LCLS   incdec
incdec  SELECT $v="Tb","ADD","SUB"

        MOV    lr,pc                    ; Set up flags, and return ...
        ADD    lr,lr,#  bcReturn$id - bcOffset$id
bcOffset$id                             ; ... address in lr.

        [ $v = "Tb"                     ; If top-bottom ...
        LDR    ip,[sp,#cbSrcBase]       ; Load top source row pointer
        |                               ; ... Otherwise ...
        LDR    ip,[sp,#cbSrcLast]       ; Load bottom source pointer
        ]                               ; ... End Of Condition
        ADD    R5,ip,R2,LSL#2           ; Get source pointer reload value
        STR    R5,[sp,#cbSrcPtrRld]     ; Save source pointer reload
        LDR    ip,[sp,#cbSrcFirst]      ; Get row start address
        ADD    R2,ip,R2,LSL#2           ; Get source start pointer

        LDR    R5,[sp,#cbBlkSizY]       ; Load number of rows to copy
        LDR    R7,[sp,#cbSrcFstY]       ; Load src first row number

        ; The three level loop follows ...
        
;----------------------------------------------------------------------+
bcBoxLoop$id                            ; Once per box drawn           |
        ; R5 - Remaining number of rows to be plotted                  |
        ; R7 - Number of next source row (+1 if bottom-to-top)         |
        [ $v = "Tb"                 ; If plotting top-to-bottom ...    |
        LDR    R6,[sp,#cbSrcSizY]   ; Load src pixmap y size           |
        SUB    R7,R6,R7             ; Compute max no of rows           |
        ]                           ; ... End Of Condition             |
        SUBS   R6,R5,R7             ; Compute remainder                |
        MOVGT  R5,R7                ; Find no of rows this time        |
        STR    R6,[sp,#cbBlkRemY]   ; Save remainder of rows           |
        ;   R5 - Number of source rows to plot this time round         |
        ; ( R6 - Number of rows left over (or a number <=0) )          |
        SUB    R5,R5,#1             ; Decrement no of rows by 1        |
        AND    R0,R0,#2_11111111    ; Mask out old row count           |
        ORR    R0,R0,R5,LSL#8       ; Load number of rows              |
;--------------------------------------------------------------------+ |
bcRowLoop$id                        ; Once per row                   | |
        [ $s = "St"                 ; If stippled ...                | |
        MOV    ip,R2                ; Save source pointer            | |
        ]                           ; End of condition               | |
        BIC    R1,R1,#(&FF :SHL: 24) ; Clear bit-plane count         | |
;------------------------------------------------------------------+ | |
bcPlaneLoop$id                      ; Once per bit plane           | | |
        [ $h = "Lr"                 ; If left-to-right ...         | | |
        LDR    R5,[R2,#-4]          ; Load first source word       | | |
        LDR    R6,[R2],#4           ; Load second source word      | | |
        MOV    R5,R5,LSR R1         ; Align first source word      | | |
        ORR    R5,R5,R6,LSL R0      ; And combine with second      | | |
        |                           ; ... Else If right-to-left ...| | |
        LDR    R6,[R2,#-4]          ; Load second source word      | | |
        LDR    R5,[R2],#-4          ; Load first source word       | | |
        MOV    R5,R5,LSL R0         ; Align second source word     | | |
        ORR    R5,R5,R6,LSR R1      ; And combine with first word  | | |
        ]                           ; ... End Of Condition         | | |
        LDR    R7,[R3]              ; Preload first dest word      | | |
        TEQP   lr,#0                ; Set up N flag                | | |
        LDR    pc,[sp,R1,LSR#22]    ; Call routine from vector     | | |
bcReturn$id                         ; Return here                  | | |
        STR    R5,[R3]              ; Save last destination word   | | |
        AND    R4,R4,R4,ROR#16      ; Restore mid word count       | | |
        [ $s = "St"                 ; If stippled ...              | | |
        MOV    R2,ip                ; Restore source pointer       | | |
        |                           ; ... Otherwise ...            | | |
        $incdec R2,R2,R10,LSL#2     ; Inc/Dec source pointer       | | |
        ]                           ; ... End of Condition         | | |
        $incdec R3,R3,R11,LSL#2     ; Des ptr to next bit plane    | | |
        ADD    R1,R1,#(1 :SHL: 24)  ; Increment and branch ...     | | |
        CMPS   R1,R1,LSL#8          ; ... continued ...            | | |
        BCC    bcPlaneLoop$id       ; ... continued                | | |
;------------------------------------------------------------------+ | |
        [ $s = "St"                 ; If stippled ...                | |
        $incdec R2,R2,R10,LSL#2     ; Increment source to next row   | |
        ]                           ; ... End of Condition           | |
        SUBS   R0,R0,#(1 :SHL: 8)   ; Decrement and branch ...       | |
        BCS    bcRowLoop$id         ; ... continued                  | |
;--------------------------------------------------------------------+ |
        LDR    R5,[sp,#cbBlkRemY]   ; Get remainder no of rows         |
        LDR    R2,[sp,#cbSrcPtrRld] ; Get reload value for src ptr     |
        [ $v = "Tb"                 ; If plotting top-to-bottom ...    |
        MOV    R7,#0                ; Start from top row               |
        |                           ; ... Otherwise ...                |
        LDR    R7,[sp,#cbSrcSizY]   ; Start from bottom row            |
        ]                           ; ... End Of Condition             |
        CMPS   R5,#0                ; Check remainder                  |
        BGT    bcBoxLoop$id         ; Branch if more to do             |
;----------------------------------------------------------------------+
        
        MEND

;-------------------------------------------------------------------
;                                        Tertiary control routines
;-------------------------------------------------------------------

; This wrapper macro for the tertiary control routines is very similar
;   to the same system for the binary routines ...

; Register usage is as follows:

;          Entry                    Exit
; R0  -    Ptr to control block     Undefined
; R1  -    Undefined                Undefined
; R2  -    Undefined                Undefined
; R3  -    Undefined                Undefined
; R4  -    Undefined                  Preserved
; R5  -    Undefined                  Preserved
; R6  -    Undefined                  Preserved
; R7  -    Undefined                  Preserved
; R8  -    Undefined                  Preserved
; R9  -    Undefined                  Preserved
; R10 -    Undefined                  Preserved
; R11 -    Undefined                  Preserved
; ip  -    Undefined                Undefined
; sp  -    Stack pointer              Preserved
; lr  -    Return address w flags   Undefined
; pc  -    PC + no special flags      lr including flags

        MACRO
$l      tertiaryControl_Pcs $h,$v,$s
        
        LCLS   id
id      SETS   $h :CC: $v :CC: $s

$l.$id

        STMFD  sp!,{R4-R9,sl,fp,lr}    ; Save registers
        STR    sp,[R0,#cbStkPtr]       ; Save stack pointer
        MOV    sp,R0                   ; Point to control block
tc      tertiaryControl $h,$v,$s       ; Routine goes here
        LDR    sp,[sp,#cbStkPtr]       ; Restore stack pointer
        LDMFD  sp!,{R4-R9,sl,fp,pc}^   ; Restore regs and rtn with flags
        
        MEND
        
ctlTertiary tertiaryControl_Pcs "Lr","Tb","Us"
ctlTertiary tertiaryControl_Pcs "Lr","Bt","Us"
ctlTertiary tertiaryControl_Pcs "Rl","Tb","Us"
ctlTertiary tertiaryControl_Pcs "Rl","Bt","Us"

ctlTertiary tertiaryControl_Pcs "Lr","Tb","St"
ctlTertiary tertiaryControl_Pcs "Lr","Bt","St"
ctlTertiary tertiaryControl_Pcs "Rl","Tb","St"
ctlTertiary tertiaryControl_Pcs "Rl","Bt","St"

;-------------------------------------------------------------------
;                                  Tertiary control routines macro
;-------------------------------------------------------------------

; This macro is similar in basic priciples to the binaryControl macro,
;   except that it performs tertiary (masked), combination. Note
;   that more use is made of the control block area due to greater
;   register usage requirements.

; Register usage is as follows:

;          Entry                         Exit
; R0  -    Undefined                     Undefined
; R1  -    Undefined                     Undefined
; R2  -    Undefined                     Undefined
; R3  -    Undefined                     Undefined
; R4  -    Undefined                     Undefined
; R5  -    Undefined                     Undefined
; R6  -    Undefined                     Undefined
; R7  -    Undefined                     Undefined
; R8  -    Undefined                     Undefined
; R9  -    Undefined                     Undefined
; R10 -    Undefined                     Undefined
; R11 -    Undefined                     Undefined
; ip  -    Undefined                       Preserved
; sp  -    Ptr to control block            Preserved
; lr  -    Return address and flags      Undefined
; pc  -    Flags not specified             at end of macro, flags destroyed

;--------------------------------- Macro defintion

        MACRO
$label  tertiaryControl $h,$v,$s

        ; Create a local string variable to append to each label
        ;   to make them unique
        
        LCLS   id
id      SETS   $h :CC: $v :CC: $s

        ; Label the code with the identifier appended to the label
$label.$id

        ; Create a local string variable to determine the instruction
        ;   to use to compute the source/destination increment/decrement
        ;   quantities ...
        
        LCLS   add_sub
add_sub SELECT (($h="Lr":LAND:$v="Tb"):LOR:($h="Rl":LAND:$v="Bt")),"SUB","ADD"

        ; Compute instructions to use when computing first and last
        ;    word masks:
        
        LCLS   mvn_mov
        LCLS   mov_mvn
mvn_mov SELECT $h="Lr","MVN","MOV"
mov_mvn SELECT $h="Lr","MOV","MVN"

        ; Compute the instruction to use to find the mid word count (+1)
        
        LCLS   rsb_sub
rsb_sub SELECT $h="Lr","RSB","SUB"

         ; Use same macro as in 'binaryControl' to compute source and
         ;   destination co-ordinates
        
        getCoordinates exitTertiaryControl$id,"Ms",$h,$v,$s

        ; R5 - X size ; R6 - Des X start ; R7  - Src X start
        ; R8 - Y size ; R9 - Des Y start ; R10 - Src Y start
        
        STR    R10,[sp,#cbSrcFstY]  ; Save source start Y co-ordinate
        STR    R5,[sp,#cbBlkSizX]   ; Save block X size
        STR    R8,[sp,#cbBlkSizY]   ; Save block Y size
        LDR    R1,[sp,#cbDesBPP]    ; Load destination bits per pixel

        LDR    R3,[sp,#cbDesWPV]    ; Load destination words per vector
        MUL    ip,R1,R3             ; Compute destination words per line
        MUL    R2,ip,R9             ; Compute des start row offset
        [ $v="Bt"                   ; If bottom-to-top
        SUB    R2,R2,R3             ; Get last bit plane of last row
        ]                           ; ... End Of Condition
        LDR    R3,[sp,#cbDesBase]   ; Load destination base address
        ADD    R2,R3,R2,LSL#2       ; Get des start row address
        STR    R2,[sp,#cbDesFirst]  ; Save des start row address
        
        LDR    R3,[sp,#cbSrcWPL]    ; Load source words per line
        MUL    R2,R3,R10            ; Compute src start row offset
        [ $v = "Bt"                 ; If bottom-to-top ...
        LDR    ip,[sp,#cbSrcWPV]    ; Get source words per vector
        SUB    R2,R2,ip             ; Get last bit plane of last row
        ]                           ; ... End Of Condition
        LDR    R3,[sp,#cbSrcBase]   ; Load source base address
        ADD    R2,R3,R2,LSL#2       ; Compute src start row address
        STR    R2,[sp,#cbSrcFirst]  ; Save source start row address
        
        LDR    R8,[sp,#cbMskOffY]   ; Load mask Y offset
        SUB    R8,R9,R8             ; Compute mask Y start
        LDR    R3,[sp,#cbMskWPV]    ; Load mask words per vector
        MUL    R2,R3,R8             ; Compute mask start row offset
        [ $v="Bt"                   ; If bottom-to-top
        SUB    R2,R2,R3             ; Get addr of first row, not first row +1
        ]                           ; ... End Of Condition
        LDR    R3,[sp,#cbMskBase]   ; Load mask base address
        ADD    R2,R3,R2,LSL#2       ; Get mask start row address
        STR    R2,[sp,#cbMskFirst]  ; Save mask start row address
        
        [ $v = "Bt"                 ; If plotting bottom-to-top ...
        SUB    R10,R10,#1           ; Get correct bottom y start value,
        SUB    R9, R9, #1           ;    for source and destination
        ]                           ; ... End Of Condition

        MOV    R1,R1,LSL#16         ; Move BPP into bits 16-23

;-------------------------------------------------------------------+
terColLoop$id                       ; Once per column generated     |
        ; R5 - Remaining amount of columns                          |
        ; R6 - Next  column : Destination (+1 if right-to-left)     |
        ; R7 - Next  column : Source      (+1 if right-to-left)     |
        [ $h="Lr"                   ; If left-to-right ...          |
        LDR    R8,[sp,#cbSrcSizX]   ; Load src pixmap X size        |
        SUB    R8,R8,R7             ; Compute max no of columns     |
        SUBS   R4,R5,R8             ; Compute remainder             |
        MOVGT  R5,R8                ; Find no of columns this time  |
        |                           ; ... Else If right-to-left ... |
        SUBS   R4,R5,R7             ; Compute remainder columns     |
        MOVGT  R5,R7                ; Determine columns this time   |
        ]                           ; ... End Of Condition          |
        STR    R4,[sp,#cbBlkRemX]   ; Save remainder of columns     |
        [ $h = "Lr"                 ; If left to right ...          |
        ADD    R5,R6,R5             ; Add to compute end column     |
        |                           ; ... Else if right to left ... |
        SUB    R5,R6,R5             ; Subtract to compute end column|
        ]                           ; ... End Of Condition          |
        STR    R5,[sp,#cbDesPosX]   ; Save next des start pos       |
        ; R5 - End column for destination                           |
        ; R6 - Start column for destination                         |
        ; R7 - Start column for source                              |
        AND    R0,R6,#2_11111       ; Get bit offset, fst des word  |
        MVN    R9,#0                ; Compute first word mask ...   |
        $mov_mvn R8,R9,LSL R0       ; ... continued                 |
        AND    ip,R5,#2_11111       ; Compute last word mask ...    |
        $mvn_mov R9,R9,LSL ip       ; ... continued                 |
        MOV    R3,R6,LSR#5          ; Compute wrd offset of des wrd |
        $rsb_sub R4,R3,R5,LSR#5     ; Compute mid word count +1     |
        SUB    R2,R7,R0             ; Get source offset for first   |
        ADD    R2,R2,#31            ;  source word +1 ...           |
        MOV    R2,R2,LSR#5          ; ... continued                 |
        LDR    R5,[sp,#cbMskOffX]   ; Load Msk-Des X offset         |
        SUB    R5,R6,R5             ; Get first mask column         |
        SUB    R1,R5,R0             ; Get mask offset for first     |
        ADD    R1,R1,#31            ;    mask word +1 ...           |
        MOV    R1,R1,LSR#5          ; ... continued                 |
        ; R1 - First mask   word +1, offset into row (in words)     |
        ; R2 - First source word +1, offset into row (in words)     |
        ; R3 - First dest word offset in row                        |
        ; R4 - Mid word count +1, not special format                |
        ; R5 - First mask column                                    |
        ; R6 - First destination column                             |
        ; R7 - First source column                                  |
        ; R8 - First word mask                                      |
        ; R9 - Last word mask                                       |
        LDR    ip,[sp,#cbDesFirst]  ; Get base addr, first des row  |
        ADD    R3,ip,R3,LSL#2       ; Compute first des word address|
        LDR    ip,[sp,#cbMskFirst]  ; Get base addr, first msk row  |
        ADD    R1,ip,R1,LSL#2       ; Compute first msk word address|
        [ $s = "Us"                 ; If unstippled ...             |
        LDR    R10,[sp,#cbSrcWPV]   ; compute source increment or   |
        $add_sub R10,R10,R4         ;   decrement ...               |
        $add_sub R10,R10,#1         ; ... continued                 |
        STR    R10,[sp,#cbSrcIncDec]; ... and save it               |
        ]                           ; ... End Of Condition          |
        LDR    R11,[sp,#cbDesWPV]   ; Compute destination increment |
        $add_sub R11,R11,R4         ;   or decrement ...            |
        STR    R11,[sp,#cbDesIncDec]; ... and save it               |
        SUB    R7,R6,R7             ; Compute 32-(src shift count), |
        AND    R7,R7,#2_11111       ; ... continued                 |
        RSB    R0,R7,#32            ; Compute src shift count       |
        ORR    R0,R0,R7,LSL#16      ; Load 32-shift count           |
        SUB    R5,R6,R5             ; Compute 32-(msk shift count), |
        AND    R5,R5,#2_11111       ; ... continued                 |
        ORR    R0,R0,R5,LSL#24      ; Load into R0                  |
        RSB    R5,R5,#32            ; Compute msk shift count       |
        ORR    R0,R0,R5,LSL#8       ; Load into R0                  |
        [ $h="Rl"                   ; If right to left ...          |
        MOV    R0,R0,ROR#16         ; Adjust shift count orientation|
        ]                           ; ... End Of Condition          |
        SUBS   R4,R4,#1             ; Get MWC and N flag            |
        ANDMI  R8,R8,R9             ; Adjust masks if -1            |
        ORR    R4,R4,R4,LSL#16      ; Set up MWC properly           |
        ; At this point, the registers are set up satisfactorily    |
        ;    for the processing of a single column.                 |
        tertiaryColumn $h,$v,$s     ; Process this column           |
        LDR    R5,[sp,#cbBlkRemX]   ; Load remaining columns        |
        LDR    R6,[sp,#cbDesPosX]   ; Load next dest position       |
        [ $h = "Lr"                 ; If going left-right ...       |
        MOV    R7,#0                ; Next source is leftmost       |
        |                           ; ... Otherwise ...             |
        LDR    R7,[sp,#cbSrcSizX]   ; Next source is rightmost      |
        ]                           ; ... End Of Condition          |
        CMPS   R5,#0                ; Test remainder                |
        BGT    terColLoop$id        ; Loop if more columns          |
;-------------------------------------------------------------------+

exitTertiaryControl$id              ; Exit point here

        MEND                        ; End Of Macro

;-------------------------------------------------------------------
;                               Tertiary column control code macro
;-------------------------------------------------------------------

; Same as binary control routines macro except for tertiary control

; The macro accepts three parameters to identify which section of code
;   to compile:
; $h :   "Lr" => left-to-right ; "Rl" =>right-to-left
; $v :   "Tb" => top-to-bottom ; "Bt" =>bottom-to-top
; $s :   "Us" => not stippled  ; "St" =>stippled

; Follows the same basic principles as the binary control, except that
;   the control block is used much more to cache intermediate values,
;   as register usage requirements are much greater.

; The register usage is as follows:

; R0  -   Shift counts                      Preserved
; R1  -   First mask word +1 address          Undefined
; R2  -   Fst src word + 1, wrd ofst in row   Undefined
; R3  -   Ptr to first des word               Undefined
; R4  -   Mid word count, special format    Preserved
; R5  -   Undefined                           Undefined
; R6  -   Undefined                           Undefined
; R7  -   Undefined                           Undefined
; R8  -   First word mask                     Undefined
; R9  -   Last Word mask                      Undefined
; R10 -   Undefined                           Undefined
; R11 -   Undefined                           Undefined
; ip  -   Undefined                           Undefined
; sp  -   Control Block Pointer             Preserved
; lr  -   Undefined                           Undefined
; pc  -   'N' flag set if MWC=-1            at end of macro, flags destroyed

; Further Co-ordinate information :
; [sp,#cbSrcFstY]   : src row for fst des row, ( +1 if bottom-to-top )
; [sp,#cbBlkSizY]   : y size of destination column
; [sp,#cbSrcSizY]   : y size of source pixmap

; Further General information :
; [sp,#cbSrcBase]   : ptr to top    source row (for top-bottom plotting)
; [sp,#cbSrcLast]   : ptr to bottom source row (for bottom-top plotting)
; [sp,#cbSrcFirst]  : ptr to src row from which fst des row is mapped
; [sp,#cbSrcIncDec] : source      increment/decrement (unstippled only)
; [sp,#cbDesIncDec] : destination increment/decrement

; All control block information is preserved.

;------------------ The macro body -------------------

        MACRO
$label  tertiaryColumn $h,$v,$s

        ; Create a local string variable to append to each label
        ;   to make them unique
        
        LCLS   id
id      SETS   $h :CC: $v :CC: $s

        ; Set increment/decrement according as to plotting top-bottom
        ;    or bottom-top:
        
        LCLS   incdec
incdec  SELECT $v="Tb","ADD","SUB"
       
        MOV    lr,pc                    ; Set up flags, and return ...
        ADD    lr,lr,#  tcReturn$id - tcOffset$id
tcOffset$id                             ; ... address in lr.

        [ $v = "Tb"                     ; If top-bottom ...
        LDR    ip,[sp,#cbSrcBase]       ; Load top source row pointer
        |                               ; ... Otherwise ...
        LDR    ip,[sp,#cbSrcLast]       ; Load bottom source pointer
        ]                               ; ... End Of Condition
        ADD    R5,ip,R2,LSL#2           ; Get source pointer reload value
        STR    R5,[sp,#cbSrcPtrRld]     ; Save source pointer reload
        LDR    ip,[sp,#cbSrcFirst]      ; Get row start address
        ADD    R2,ip,R2,LSL#2           ; Get source start pointer

        LDR    R5,[sp,#cbBlkSizY]       ; Load number of rows to copy
        LDR    R7,[sp,#cbSrcFstY]       ; Load src first row number
        LDR    ip,[sp,#cbDesBPP]        ; Load bit plane count

        ; The three level loop follows ...
        
;----------------------------------------------------------------------+
tcBoxLoop$id                        ; Once per box drawn               |
        ; R5 - Remaining number of rows to be plotted                  |
        ; R7 - Number of next source row (+1 if bottom-to-top)         |
        [ $v = "Tb"                 ; If plotting top-to-bottom ...    |
        LDR    R6,[sp,#cbSrcSizY]   ; Load src pixmap y size           |
        SUB    R7,R6,R7             ; Compute max no of rows           |
        ]                           ; ... End Of Condition             |
        SUBS   R6,R5,R7             ; Compute remainder                |
        MOVGT  R5,R7                ; Find no of rows this time        |
        STR    R6,[sp,#cbBlkRemY]   ; Save remainder of rows           |
        ;   R5 - Number of source rows to plot this time round         |
        ; ( R6 - Number of rows left over (or a number <=0) )          |
;--------------------------------------------------------------------+ |
tcRowLoop$id                        ; Once per row                   | |
        STR    R5,[sp,#cbRowCnt]    ; Save row counter               | |
        STR    R1,[sp,#cbMskSave]   ; Save mask row pointer          | |
        [ $s = "St"                 ; If stippled ...                | |
        STR    R2,[sp,#cbSrcSave]   ; Save source pointer            | |
        ]                           ; End of condition               | |
        BIC    ip,ip,#&FF0000       ; Clear bit plane count          | |
;------------------------------------------------------------------+ | |
tcPlaneLoop$id                      ; Once per bit plane           | | |
        [ $h = "Lr"                 ; If left-to-right ...         | | |
        LDR    R5,[R2,#-4]          ; Load first source word       | | |
        LDR    R6,[R2],#4           ; Load second source word      | | |
        LDR    R10,[R1,#-4]         ; Load first mask word         | | |
        LDR    R11,[R1],#4          ; Load second mask word        | | |
        MOV    R5,R5,LSR R0         ; Align first source word      | | |
        MOV    R0,R0,ROR#8          ; Get next shift count         | | |
        MOV    R10,R10,LSR R0       ; Align first mask word        | | |
        MOV    R0,R0,ROR#8          ; Get next shift count         | | |
        ORR    R5,R5,R6,LSL R0      ; Combine source words         | | |
        MOV    R0,R0,ROR#8          ; Get next shift count         | | |
        ORR    R10,R10,R11,LSL R0   ; Combine mask words           | | |
        MOV    R0,R0,ROR#8          ; Rotate R0 back correctly     | | |
        |                           ; ... Else If right-to-left ...| | |
        LDR    R6,[R2,#-4]          ; Load second source word      | | |
        LDR    R5,[R2],#-4          ; Load first source word       | | |
        LDR    R11,[R1,#-4]         ; Load second mask word        | | |
        LDR    R10,[R1],#-4         ; Load first mask word         | | |
        MOV    R5,R5,LSL R0         ; Align first source word      | | |
        MOV    R0,R0,ROR#8          ; Get next shift count         | | |
        MOV    R10,R10,LSL R0       ; Align first mask word        | | |
        MOV    R0,R0,ROR#8          ; Get next shift count         | | |
        ORR    R5,R5,R6,LSR R0      ; Combine source words         | | |
        MOV    R0,R0,ROR#8          ; Get next shift count         | | |
        ORR    R10,R10,R11,LSR R0   ; Combine mask words           | | |
        MOV    R0,R0,ROR#8          ; Rotate R0 back correctly     | | |
        ]                           ; ... End Of Condition         | | |
        LDR    R7,[R3]              ; Preload first dest word      | | |
        TEQP   lr,#0                ; Set up N flag                | | |
        LDR    pc,[sp,ip,LSR#14]    ; Call routine from vector     | | |
tcReturn$id                         ; Return here                  | | |
        STR    R5,[R3]              ; Save last destination word   | | |
        AND    R4,R4,R4,ROR#16      ; Restore mid word count       | | |
        LDR    R1,[sp,#cbMskSave]   ; Restore mask pointer         | | |
        [ $s = "St"                 ; If stippled ...              | | |
        LDR    R2,[sp,#cbSrcSave]   ; Restore source pointer       | | |
        |                           ; ... Otherwise ...            | | |
        LDR    R10,[sp,#cbSrcIncDec]; Load source inc/dec          | | |
        $incdec R2,R2,R10,LSL#2     ; Inc/Dec source pointer       | | |
        ]                           ; ... End of Condition         | | |
        LDR    R11,[sp,#cbDesIncDec]; Load des inc/dec             | | |
        $incdec R3,R3,R11,LSL#2     ; Inc/Dec dest pointer         | | |
        ADD    ip,ip,#(1 :SHL: 16)  ; Increment and branch ...     | | |
        CMPS   ip,ip,LSL#16         ; ... continued ...            | | |
        BCC    tcPlaneLoop$id       ; ... continued                | | |
;------------------------------------------------------------------+ | |
        [ $s = "St"                 ; If stippled ...                | |
        LDR    R10,[sp,#cbSrcWPV]   ; Load source inc.dec            | |
        $incdec R2,R2,R10,LSL#2     ; Increment source to next row   | |
        ]                           ; ... End of Condition           | |
        LDR    R10,[sp,#cbMskWPV]   ; Load mask inc.dec              | |
        $incdec R1,R1,R10,LSL#2     ; Increment mask to next row     | |
        LDR    R5,[sp,#cbRowCnt]    ; Load row counter               | |
        SUBS   R5,R5,#1             ; Decrement and branch ...       | |
        BHI    tcRowLoop$id         ; ... continued                  | |
;--------------------------------------------------------------------+ |
        LDR    R5,[sp,#cbBlkRemY]   ; Get remainder no of rows         |
        LDR    R2,[sp,#cbSrcPtrRld] ; Get reload value for src ptr     |
        [ $v = "Tb"                 ; If plotting top-to-bottom ...    |
        MOV    R7,#0                ; Start from top row               |
        |                           ; ... Otherwise ...                |
        LDR    R7,[sp,#cbSrcSizY]   ; Start from bottom row            |
        ]                           ; ... End Of Condition             |
        CMPS   R5,#0                ; Check remainder                  |
        BGT    tcBoxLoop$id         ; Branch if more to do             |
;----------------------------------------------------------------------+
        
        MEND

;-------------------------------------------------------------------
;                                            getCoordiates (Macro)
;-------------------------------------------------------------------

; This macro compiles a section of code which reads in the appropriate
;   co-ordinate information (according to the orientation: left-to-right
;   versus right-to-left, etc.) and sets it up into various registers
;   it also adjusts the source-to-destination offsets so that the source
;   co-ordinates lie within their correct residue ranges modulo the
;   size of the source pix-map. To do this, it uses the macro
;   'getResidue'

; Register usage is as follows:

;        Entry                             Exit
; R0  -  Undefined                     Undefined
; R1  -  Undefined                     Undefined
; R2  -  Undefined                     Undefined
; R3  -  Undefined                     Undefined
; R4  -  Undefined                     Undefined
; R5  -  Undefined                       Block  size  : X
; R6  -  Undefined                       Des    start : X
; R7  -  Undefined                       Source start : X
; R8  -  Undefined                       Block  size  : Y
; R9  -  Undefined                       Des    start : Y
; R10 -  Undefined                       Source start : Y
; R11 -  Undefined                     Undefined
; ip  -  Undefined                     Undefined
; sp  -  Control block pointer           Preserved
; lr  -  Undefined                       Preserved
; pc  -  no special flags                at end of macro, flags destroyed

;---------------------- Start of macro

        MACRO
$label  getCoordinates $exit,$c,$h,$v,$s
$label

        LCLS   id
id      SETS   $c :CC: $h :CC: $v :CC: $s

        ; First get the X information :
        
; ----------- X co-ordiate quantities ------------------------------

        [ $h="Lr"                     ; If LEFT TO RIGHT ------------
        LDR    R5,[sp,#cbSrcOffX]     ; Get src-des offset X
        LDR    R6,[sp,#cbDesLftX]     ; Get des       left X
        SUBS   R7,R6,R5               ; Compute src  start X
        LDR    R0,[sp,#cbSrcSizX]     ; Get source X size
        getResidue $c,$h,$v,$s,"X", R0,R7,R5,"Nn" ; Compute correct residue
        STR    R5,[sp,#cbSrcOffX]     ; Save back newly computed offset
        LDR    R5,[sp,#cbDesRgtX]     ; Load right X
        SUBS   R5,R5,R6               ; Compute block size
        BLE    $exit                  ; Exit if block too small
        
        |                             ; If RIGHT-TO_LEFT -------------
        LDR    R5,[sp,#cbSrcOffX]     ; Get src-des offset X
        LDR    R6,[sp,#cbDesRgtX]     ; Get des      right X
        SUBS   R7,R6,R5               ; Compute src  start X
        LDR    R0,[sp,#cbSrcSizX]     ; Get source X size
        getResidue $c,$h,$v,$s,"X", R0,R7,R5,"Ps" ; Compute correct residue
        STR    R5,[sp,#cbSrcOffX]     ; Save back newly computed offset
        LDR    R5,[sp,#cbDesLftX]     ; Load left X
        SUBS   R5,R6,R5               ; Compute block size
        BLE    $exit                  ; Exit if block too small

        ]                             ; End of codition

; ----------- Y co-ordiate quantities ------------------------------

        [ $v="Tb"                     ; If TOP TO BOTTOM ------------
        LDR    R8,[sp,#cbSrcOffY]     ; Get src-des offset Y
        LDR    R9,[sp,#cbDesTopY]     ; Get des        top Y
        SUBS   R10,R9,R8              ; Compute src  start Y
        LDR    R0,[sp,#cbSrcSizY]     ; Get source Y size
        getResidue $c,$h,$v,$s,"Y", R0,R10,R8,"Nn" ; Compute correct residue
        STR    R8,[sp,#cbSrcOffY]     ; Save back newly computed offset
        LDR    R8,[sp,#cbDesBotY]     ; Load bottom Y
        SUBS   R8,R8,R9               ; Compute block size
        BLE    $exit                  ; Exit if block too small
        
        |                             ; If BOTTOM TO TOP ------------
        LDR    R8,[sp,#cbSrcOffY]     ; Get src-des offset Y
        LDR    R9,[sp,#cbDesBotY]     ; Get des     bottom Y
        SUBS   R10,R9,R8              ; Compute src  start Y
        LDR    R0,[sp,#cbSrcSizY]     ; Get source Y size
        getResidue $c,$h,$v,$s,"Y", R0,R10,R8,"Ps" ; Compute correct residue
        STR    R8,[sp,#cbSrcOffY]     ; Save back newly computed offset
        LDR    R8,[sp,#cbDesTopY]     ; Load top Y
        SUBS   R8,R9,R8               ; Compute block size
        BLE    $exit                  ; Exit if block too small

        ]                             ; End of condition
        
        MEND                          ; End of Macro        

;-------------------------------------------------------------------
;                                               getResidue (Macro)
;-------------------------------------------------------------------

; This macro will compile code to compute the (appropriate) residue
;   of some quantity modulo some other quantity, in addition, it
;   a third register the corresponding offset.

; In simpler terms: The macro effectively will add an appropriate 
;   (posotive or negative) multiple of the register $p to the
;   register $r until it lies within a certain range. The quantity
;   added to the register $r will also be added to the register $o.

; If the parameter $t is "Ps" (standing for "least PoSotive residue")
;   the code will get $r to be in the range 1 ... $p.
; If the parameter $t is "Nn" (standing for "least Non Negative residue")
;   the code will get $r to be in the range 0 ... ($p-1)

; Register usage is as follows:

;       Entry                        Exit
; $p  - The divisor                  Undefined
; $r  - The numerator                The correct residue
; $o  - Some quantity                Same with offset added to it
; ip  - Undefined                    Undefined
; sp  - Undefined                    Preserved
; lr  - Undefined                    Preserved
; pc  - N flag (see below)           End of macro, flags destroyed
; All pother registers:
;     - Undefined                    Preserved

; The N flag must be set according to whether the value in register
;   $r is posotive or negative.

;--------------------- Start of macro

        MACRO
$label  getResidue $c,$h,$v,$s,$a $p,$r,$o, $t

        ; $c,$h,$v,$s, and $a are used to compute a unique ID to
        ;   append to the labels used by this macro

        LCLS   id
id      SETS   $c :CC: $h :CC: $v :CC: $s :CC: "_" :CC: $a

        MOV    ip,#0                  ; Initialize shift count
        BMI    negativeCase$id        ; Branch if $r is negative
;-----------------------------------------------------------------+
shlLoopP$id                           ; Once per shift            |
        MOV    $p,$p,ASL#1            ; Shift denominator         |
        ADD    ip,ip,#1               ; Increment shift count     |
        CMPS   $r,$p                  ; Compare with numarator    |
        [ $t="Nn"                     ; If non-negative required, |
        BGE    shlLoopP$id            ; Loop until GE             |
        |                             ; Else posotive required,   |
        BGT    shlLoopP$id            ; Loop until GT             |
        ]                             ; End of condition          |
;-----------------------------------------------------------------+
;-----------------------------------------------------------------+
shrLoopP$id                           ; Once per shift            |
        MOV    $p,$p,ASR#1            ; Shift denominator         |
        CMPS   $r,$p                  ; Compare with residue      |
        [ $t="Nn"                     ; If non-negative required, |
        SUBGE  $r,$r,$p               ; Adjust residue and offset |
        ADDGE  $o,$o,$p               ;   if applicable           |
        |                             ; Else posotive required,   |
        SUBGT  $r,$r,$p               ; Adjust residue and offset |
        ADDGT  $r,$r,$p               ;   if applicable           |
        ]                             ; End of condition          |
        SUBS   ip,ip,#1               ; Decrement shift count     |
        BNE    shrLoopP$id            ; And loop conditionally    |
;-----------------------------------------------------------------+
        B      completedResidue$id    ; Branch to macro end

negativeCase$id
;-----------------------------------------------------------------+
shlLoopN$id                           ; Once per shift            |
        MOV    $p,$p,ASL#1            ; Shift denominator         |
        ADD    ip,ip,#1               ; Increment shift count     |
        CMNS   $r,$p                  ; Compare with numarator    |
        [ $t="Nn"                     ; If non-negative required, |
        BLT    shlLoopN$id            ; Loop until GE             |
        |                             ; Else posotive required,   |
        BLE    shlLoopN$id            ; Loop until GT             |
        ]                             ; End of condition          |
;-----------------------------------------------------------------+
;-----------------------------------------------------------------+
shrLoopN$id                           ; Once per shift            |
        MOV    $p,$p,ASR#1            ; Shift denominator         |
        CMNS   $r,$p                  ; Compare with residue      |
        [ $t="Nn"                     ; If non-negative required, |
        ADDLT  $r,$r,$p               ; Adjust residue and offset |
        SUBLT  $o,$o,$p               ;   if applicable           |
        |                             ; Else posotive required,   |
        ADDLE  $r,$r,$p               ; Adjust residue and offset |
        SUBLE  $r,$r,$p               ;   if applicable           |
        ]                             ; End of condition          |
        SUBS   ip,ip,#1               ; Decrement shift count     |
        BNE    shrLoopN$id            ; And loop conditionally    |
;-----------------------------------------------------------------+
        ADD    $r,$r,$p               ; Get residue >=0.
        SUB    $o,$o,$p               ; Corresponding change in offset

completedResidue$id                   ; End of macro here
        MEND

;-------------------------------------------------------------------
;                           Look-up table for Combination Routines
;-------------------------------------------------------------------

; The following look-up table allows routines in the file 
;  'gc_decoder.c' to look-up the address of the appropriate
;  combination routines for the various bit-planes of the
;  destination pix-map.

; Each entry in the table is a pointer to a routine which definately
;   does not obey the procedure call standard, and whose names are
;   not avaliable directly from other files (as they are not exported)

; The offsets into this table are computed as follows:
; Bit  5   : 0 => Unary/Binary     , 1 => Tertiary (Masked)
; Bit  4   : 0 => Left-to-right    , 1 => Right-to-left
; Bits 0-3 : The combination mode for this bit-plane (X encoded)

combineLookUp

;----- Unmasked, left-to-right
       &      plotUmLr_0000 - combineLookUp
       &      plotUmLr_0001 - combineLookUp
       &      plotUmLr_0010 - combineLookUp
       &      plotUmLr_0011 - combineLookUp
       &      plotUmLr_0100 - combineLookUp
       &      plotUmLr_0101 - combineLookUp
       &      plotUmLr_0110 - combineLookUp
       &      plotUmLr_0111 - combineLookUp
       &      plotUmLr_1000 - combineLookUp
       &      plotUmLr_1001 - combineLookUp
       &      plotUmLr_1010 - combineLookUp
       &      plotUmLr_1011 - combineLookUp
       &      plotUmLr_1100 - combineLookUp
       &      plotUmLr_1101 - combineLookUp
       &      plotUmLr_1110 - combineLookUp
       &      plotUmLr_1111 - combineLookUp

;----- Unmasked, right-to-left
       &      plotUmRl_0000 - combineLookUp
       &      plotUmRl_0001 - combineLookUp
       &      plotUmRl_0010 - combineLookUp
       &      plotUmRl_0011 - combineLookUp
       &      plotUmRl_0100 - combineLookUp
       &      plotUmRl_0101 - combineLookUp
       &      plotUmRl_0110 - combineLookUp
       &      plotUmRl_0111 - combineLookUp
       &      plotUmRl_1000 - combineLookUp
       &      plotUmRl_1001 - combineLookUp
       &      plotUmRl_1010 - combineLookUp
       &      plotUmRl_1011 - combineLookUp
       &      plotUmRl_1100 - combineLookUp
       &      plotUmRl_1101 - combineLookUp
       &      plotUmRl_1110 - combineLookUp
       &      plotUmRl_1111 - combineLookUp

;----- Masked, left-to-right
       &      plotMsLr_0000 - combineLookUp
       &      plotMsLr_0001 - combineLookUp
       &      plotMsLr_0010 - combineLookUp
       &      plotMsLr_0011 - combineLookUp
       &      plotMsLr_0100 - combineLookUp
       &      plotMsLr_0101 - combineLookUp
       &      plotMsLr_0110 - combineLookUp
       &      plotMsLr_0111 - combineLookUp
       &      plotMsLr_1000 - combineLookUp
       &      plotMsLr_1001 - combineLookUp
       &      plotMsLr_1010 - combineLookUp
       &      plotMsLr_1011 - combineLookUp
       &      plotMsLr_1100 - combineLookUp
       &      plotMsLr_1101 - combineLookUp
       &      plotMsLr_1110 - combineLookUp
       &      plotMsLr_1111 - combineLookUp

;----- Masked, right-to-left
       &      plotMsRl_0000 - combineLookUp
       &      plotMsRl_0001 - combineLookUp
       &      plotMsRl_0010 - combineLookUp
       &      plotMsRl_0011 - combineLookUp
       &      plotMsRl_0100 - combineLookUp
       &      plotMsRl_0101 - combineLookUp
       &      plotMsRl_0110 - combineLookUp
       &      plotMsRl_0111 - combineLookUp
       &      plotMsRl_1000 - combineLookUp
       &      plotMsRl_1001 - combineLookUp
       &      plotMsRl_1010 - combineLookUp
       &      plotMsRl_1011 - combineLookUp
       &      plotMsRl_1100 - combineLookUp
       &      plotMsRl_1101 - combineLookUp
       &      plotMsRl_1110 - combineLookUp
       &      plotMsRl_1111 - combineLookUp

;-------------------------------------------------------------------
;                                            Combine table look-up
;-------------------------------------------------------------------

; This function is used in 'gc-decoder.c' to simply look-up in the
;    combine routines table a particular combine routine and compute
;    it's address. It was written in assembley to get over some
;    casting problems in ANSI C.

        ; At head of file : EXPORT |_dpLookUpCombineAddress|

dpLookUpCombineAddress externLabel

        ADR    ip,combineLookUp     ; Get combine lookup address
        LDR    R0,[ip,R0,LSL#2]     ; Load address offset
        ADD    R0,R0,ip             ; Add to base of table
        MOVS   pc,lr                ; Return

;-------------------------------------------------------------------
;                        Register usage in the unmasked opertaions
;-------------------------------------------------------------------

; This section of documentation describes the next few sections of
;    code which define and invoke macros to compile sixteen small
;    sections of code. The purpose of each section is to combine,
;    in one of the sixteen possible combination modes, a row from
;    a colour-plane of the source pixel-image with a row from the
;    colour-plane of the destination pixel-image where no mask
;    bit-plane is provided.

; The following describes the register usage for these pieces of code:
; This is the register usage for left-to-right operations, but the
;    right-to-left register usage is nearly the same.

;           Entry                     Exit
; R0  - *   32-Shift count            Preserved
; R1  - *   Shift count               Preserved
; R2  - *   Source pointer              +=/-= (MWC+1)*4
; R3  -     Destination pointer         +=/-= (MWC+1)*4
; R4  -     MWC (see below)           Semi MWC (see below)
; R5  - *   Aligned src preload       Destination pre-write
; R6  - *   Unaligned src carry         Destroyed
; R7  -     Destination preload         Destroyed
; R8  -     First word mask           Preserved
; R9  -     Last word mask            Preserved
; R10 -     Undefined                 Preserved
; R11 -     Undefined                 Preserved
; ip  -     Undefined                 Preserved
; sp  -     Undefined                 Preserved
; lr  -     Return address            Preserved (v.important)
; pc  -     PC + N flag see below     R14 with flags copied from R14

; '*': In the case of the four combination modes not dependent on the
;       source pixel-map data, these registers are undefined on entry
;       and are preserved on exit. Those combination modes are 0000,
;       0101, 1010, and 1111

; To understand how such an operation occurs consider the following:
; In order to write into the destination, we must write a contigous
;   sequence of words. The first and last words of which are only
;   partially written, hence the first and last word masks. These 
;   masks contain a '1' where the destination should be written and 
;   a '0' otherwise.

; We must align the source words to the destination words in order
;   to combine them (in the appropriate way) with the destination
;   words before writing the result into the destination .....
; (Draw a diagram as you read this bit.)
; Let us suppose that the first destination word with bits to be
;   altered in any particular colour plane of any particular row
;   lies in memory location <d>: Then bit 31 of this destination
;   word certainly needs to be altered, and suppose that it is
;   to be combined with bit <b'> of the source word at location
;   <s>. Then in order to obtain a word containing source
;   pixmap information aligned bit-for-bit with the destination
;   word we need to load the source word at location <s-1> and the
;   source word at location <s> and consider them as a lsw-msw
;   64-bit pair and shift them RIGHT (in terms of pixel positions),
;   or LEFT (in terms of instruction set mnemomics), by <31-b'>
;   bits, and extract the higher order 32-bit word. Now assuming 
;   that the left most pixel of any bit-map is derived from the bit 0's 
;   of the memory (as they are,) then bit 0 of any destination word 
;   is mapped from bit <dx mod 32> (where dx is the offset between 
;   the source and destination x values) of the appropriate source 
;   word, and it follows that bit 31 of the destination word is mapped 
;   from bit <(dx+31) mod 32> of the appropriate source word, and 
;   therefore that b' is equal to <(dx+31) mod 32>, and hence that
;   the shift count is 31-((dx+31)mod32) which is in fact <32-(dx mod 32)>.

; In order to save unnecassary repeated instructions in these routines,
;    they all enter with the first destination word of the row
;    preloaded into R7 whilst R3 points to the location from which
;    it was loaded, and enter with R5 loaded with the version
;    of the source data aligned as described above for that first
;    destination word, and R6 loaded with the original (unaligned,)
;    high-order part of that word, which is used in computing the
;    next aligned source word to write to the next destination word, and
;    R2 pointing to the next word after that to be read in.
; The routines exit with the last (modified) destination word un-written,
;    and stored in R5, with the address to store it in R3.

; The routine combines and writes the entire row of source and
;   destination rows except that it does not write the last
;   destination word, instead it leaves it in R5 prior to exiting,
;   and R3 will point to the location where that word would be
;   stored.

; The total number of words in the destination image to be written
;   less 2 is called the mid-word-count (MWC). This value is loaded 
;   into R4 bits 0-15 and bits 16-31. In the loops used below R4 is
;   decremented by 2<<16 until carry is set so that on exit, R4
;   contains MWC in the lower sixteen bits and all 1's in the
;   upper sixteen bits: The routine may also exit with R4 unchanged.
; Note that MWC is in the range (-1) upwards. If R4 is -1 then the N 
;   flag is set on entry to the routine. This corresponds to only one 
;   destination word being written. The loop present in almost all the 
;   combination routines will loop (MWC+1) times. Thus if MWC is -1 
;   the loop is not excecuted at all.

;-------------------------------------------------------------------
;                                    General operations : Unmasked
;-------------------------------------------------------------------

; This is a macro which is used twenty times: ten times for compiling
;    left-to-right combination routines, and ten times for compiling
;    right-to-left combination routines. The combination routines
;    are without a mask in all cases, and are the ten types of combination
;    modes in which the destination is formally dependent on both the
;    source and the destination. The macro accepts three parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right ; "Rl" => right-to-left
; $Op   : The operand which bit-wise combines the two registers in the
;           manner prescribed by the combination mode.

        MACRO
$label  combineUmSrcDes $code,$h,$Op

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

$label.Um$id

        UMDIAG id                   ; Conditional diagnostics

        $Op    R5,R5,R7              ; Combine first src and des words
        BIC    R7,R7,R8              ; Mask out des bits to be written
        AND    R5,R5,R8              ; Mask out src bits unwritten
        ORR    R5,R5,R7              ; Combine words
        MOVMIS R15,R14               ; Exit if only one word
;---------------------------------------------------------------------+
hrzLoopUm$id                         ; Start of middle word loop      |
        [ $h = "Lr"                  ; If left-to-right ...           |
        STR    R5,[R3],#4            ; Save prev dest word            |
        MOV    R5,R6,LSR R1          ; Shift and align prev. src word |
        LDR    R6,[R2],#4            ; Load next src word             |
        ORR    R5,R5,R6,LSL R0       ; Align and combine sources      |
        |                            ; ... Else If right-to-left ...  |
        STR    R5,[R3],#-4           ; Save prev. dest word           |
        MOV    R5,R6,LSL R0          ; Shift and align prev src word  |
        LDR    R6,[R2,#-4]!          ; Load next source word          |
        ORR    R5,R5,R6,LSR R1       ; Align and combine words        |
        ]                            ; ... End Of Condition           |
        LDR    R7,[R3]               ; Get destination word           |
        $Op    R5,R5,R7              ; Combine source and dest        |
        SUBS   R4,R4,#(1:SHL:16)     ; Decrement and branch ...       |
        BCS    hrzLoopUm$id          ; ... continued                  |
;---------------------------------------------------------------------+
        BIC    R7,R7,R9              ; Mask out last bits to write
        AND    R5,R5,R9              ; Mask out bits not to write
        ORR    R5,R5,R7              ; Combine words.
        MOVS   pc,lr                 ; And return

        MEND

;-------------------------------------------------------------------
;                    Opeations dependent only on source : Unmasked
;-------------------------------------------------------------------

; This macro is used four times : twice for left-to-right operations
;   and twice for right-to-left operations, in both cases compiling
;   unmasked combination routines for one of the two types of
;   combination modes which are formally dependent on the source
;   pix-map values and only the source pixmap values: Namely copying
;   the source, or copying the inverted source.

; The macro accepts three parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left
; $i    : "Iv" => inverted source ; "Ni" => not-inverted source

        MACRO
$label  combineUmSrc $code,$h,$i

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

$label.Um$id

        UMDIAG id                   ; Conditional diagnostics

        [ $i = "Iv"                  ; If to invert source ...
        MVN    R5,R5                 ; Then do so ...
        ]                            ; End of condition
        BIC    R7,R7,R8              ; Mask out des bits to be written
        AND    R5,R5,R8              ; Mask out src bits unwritten
        ORR    R5,R5,R7              ; Combine words
        MOVMIS R15,R14               ; Exit if only one word
;---------------------------------------------------------------------+
hrzLoopUm$id                         ; Start of middle word loop      |
        [ $h = "Lr"                  ; If left-to-right ...           |
        STR    R5,[R3],#4            ; Save prev dest word            |
        MOV    R5,R6,LSR R1          ; Shift and align prev. src word |
        LDR    R6,[R2],#4            ; Load next src word             |
        ORR    R5,R5,R6,LSL R0       ; Align and combine sources      |
        |                            ; ... Else If right-to-left ...  |
        STR    R5,[R3],#-4           ; Save prev. dest word           |
        MOV    R5,R6,LSL R0          ; Shift and align prev src word  |
        LDR    R6,[R2,#-4]!          ; Load next source word          |
        ORR    R5,R5,R6,LSR R1       ; Align and combine words        |
        ]                            ; ... End Of Condition           |
        [ $i = "Iv"                  ; If to invert source ...        |
        MVN    R5,R5                 ; Then do so ...                 |
        ]                            ; End of condition               |
        SUBS   R4,R4,#(1:SHL:16)     ; Decrement and branch ...       |
        BCS    hrzLoopUm$id          ; ... continued                  |
;---------------------------------------------------------------------+
        LDR    R7,[R3]               ; Get destination word
        BIC    R7,R7,R9              ; Mask out last bits to write
        AND    R5,R5,R9              ; Mask out bits not to write
        ORR    R5,R5,R7              ; Combine words.
        MOVS   pc,lr                 ; And return
        
        MEND

;-------------------------------------------------------------------
;                               Direct write operations : Unmasked
;-------------------------------------------------------------------

; This macro is used four times : twice for left-to-right operations
;   and twice for right-to-left operations, in both cases compiling
;   unmasked combination routines for one of the two types of
;   combination modes which are formally dependent on neither the
;   source nor the destination pix-map values: ie. writing all 0's
;   or writing all 1's to a bit-plane of the destination bit-image.

; The macro accepts three parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left
; $v    : "0"  => All zeros       ; "1"  => All ones

        MACRO
$label  combineUm $code,$h,$v

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

        ; Set orientation-dependent information
        LCLS   desDir
        LCLS   addSub
desDir  SELECT $h="Lr" , "4",    "-4"
addSub  SELECT $h="Lr" , "ADD" , "SUB"

$label.Um$id

        UMDIAG id                   ; Conditional diagnostics

        [ $v="0"                     ; If writing zeros ...
        BIC    R5,R7,R8              ; Clear bits in destination
        |                            ; Otherwise ...
        ORR    R5,R7,R8              ; Set bits in destination
        ]                            ; ... End of condition
        MOVMIS R15,R14               ; Exit if only one word
        $addSub R2,R2,R4,LSR#14      ; Adjust the 'source' pointer
        $addSub R2,R2,#4             ;   as if it were to be used as well
        STR    R5,[R3],#$desDir      ; Save destination word
        SUBS   R4,R4,#(1:SHL:16)     ; Test loop end
        BCC    hrzLoopEnd$id         ; Branch to loop end if finished
        [ $v="0"                     ; If writing zeros ...
        MOV    R5,#0                 ; Load all zeros
        |                            ; Otherwise ...
        MVN    R5,#0                 ; Load all ones
        ]                            ; ... End of condition
;---------------------------------------------------------------------+
hrzLoop$id                           ; Start of middle word loop      |
        STR    R5,[R3],#$desDir      ; Save prev dest word            |
        SUBS   R4,R4,#(1:SHL:16)     ; Decrement and branch ...       |
        BCS    hrzLoop$id            ; ... continued                  |
hrzLoopEnd$id                        ; Here for end of loop           |
;---------------------------------------------------------------------+
        LDR    R7,[R3]               ; Load last dest. word
        [ $v = "0"                   ; If writing zeros ...
        BIC    R5,R7,R9              ; Clear last few bits
        |                            ; Otherwise ...
        ORR    R5,R7,R9              ; Set last few bits
        ]                            ; End of condition
        MOVS   pc,lr                 ; And return
        
        MEND

;-------------------------------------------------------------------
;                          Invert destination operation : Unmasked
;-------------------------------------------------------------------

; The following macro is used twice : Once for left-to-right and
;   once for right-to-left. Both invokations compile a routine
;   to combine source with destination in a manner which is formally
;   dependent on the destination and only the destination: ie. to
;   invert the destination. The other way to plot in such a manner
;   is to do a -no operation- on the destination which is a much
;   simpler process and is compiled differently below.

; The macro accepts two parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left

        MACRO
$label  combineUmDesInvt $code,$h

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

        ; Set orientation-dependent information
        LCLS   desDir
        LCLS   addSub
desDir  SELECT $h="Lr" , "4",    "-4"
addSub  SELECT $h="Lr" , "ADD" , "SUB"

$label.Um$id

        UMDIAG id                   ; Conditional diagnostics

        EOR    R5,R7,R8              ; Invert appropriate des. bits
        MOVMIS R15,R14               ; Exit if only one word
        $addSub R2,R2,R4,LSR#14      ; Adjust the 'source' pointer
        $addSub R2,R2,#4             ;   as if it were to be used as well
;---------------------------------------------------------------------+
InvtUmLoop$id                        ; Start of middle word loop      |
        STR    R5,[R3],#$desDir      ; Save prev dest word            |
        LDR    R7,[R3]               ; Load next dest. word           |
        MVN    R5,R7                 ; Invert it                      |
        SUBS   R4,R4,#(1:SHL:16)     ; Decrement and branch ...       |
        BCS    InvtUmLoop$id         ; ... continued                  |
;---------------------------------------------------------------------+
        EOR    R5,R7,R9              ; Invert last lot of bits
        MOVS   pc,lr                 ; And return
        
        MEND

;-------------------------------------------------------------------
;                              No operation combination : Unmasked
;-------------------------------------------------------------------

; The 'NOP' operation is considerably more complicated than would be
;   imagined since the control routines which call the combination
;   routines expect certain side-effects set-out in the register
;   entry/exit usage description (above) for these combination
;   routines: Namely, R3 must be incremented/decremented by
;   the value (MWC+1)<<2, R2 must be incremented by MWC<<2, and R5
;   must contain the appropriate write-back value for the destination
;   word.

; The 'NOP routine is compiled twice: once for left-to-right and once
;   for right-to-left

; The macro accepts two parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left

        MACRO
$label  combineUmDesNop $code,$h

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

$label.Um$id

        UMDIAG id                   ; Conditional diagnostics

        BMI    mwcNegUm$id          ; Trap MWC is -1
        [ $h = "Lr"                 ; If going left-to-right ...
        ADD    R2,R2,R4,LSR#14      ; Add (MWC+1)*4 to R2 ...
        ADD    R2,R2,#4             ; ... continued
        ADD    R3,R3,R4,LSR#14      ; Add (MWC+1)*4 to R3, and load
        LDR    R5,[R3,#4]!          ;   destination write-back value
        |                           ; ... Else If right-to-left ...
        SUB    R2,R2,R4,LSR#14      ; Subtract (MWC+1)*4 from R2 ...
        SUB    R2,R2,#4             ; ... continued
        SUB    R3,R3,R4,LSR#14      ; Subtract (MWC+1)*4 from R3, and load
        LDR    R5,[R3,#4]!          ;   destination write-back value
        ]                           ; ... End Of Condition
        MOVS   pc,lr                ; Return
mwcNegUm$id                         ; Here if MWC is -1
        MOV    R5,R7                ; Get destination write-back
        MOVS   pc,lr                ; And return
        
        MEND

;-------------------------------------------------------------------
;                         Unmasked combination routine diagnostics 
;-------------------------------------------------------------------

; This macro can (conditionally) cause this file to generate diagnostics
;    which are printed on entry to any of the above unmasked
;    combination routines ...

        MACRO
$l      UMDIAG $id
        [ {FALSE}
$l
        DIAG "Unmasked combination routine entered : " :CC: $id
        ]
        MEND

;-------------------------------------------------------------------
;                             Macro invokations : Unmaked routines
;-------------------------------------------------------------------

; Now to actually invoke all the macros just defined:
; Left-to-right routines :

plot combineUm        "0000","Lr","0"
plot combineUmSrcDes  "0001","Lr",AND
plot combineUmSrcDes  "0010","Lr",BIC
plot combineUmSrc     "0011","Lr","Ni"
plot combineUmSrcDes  "0100","Lr",RBC
plot combineUmDesNop  "0101","Lr"
plot combineUmSrcDes  "0110","Lr",EOR
plot combineUmSrcDes  "0111","Lr",ORR
plot combineUmSrcDes  "1000","Lr",NOR
plot combineUmSrcDes  "1001","Lr",ENR
plot combineUmDesInvt "1010","Lr"
plot combineUmSrcDes  "1011","Lr",BST
plot combineUmSrc     "1100","Lr","Iv"
plot combineUmSrcDes  "1101","Lr",RBS
plot combineUmSrcDes  "1110","Lr",NAN
plot combineUm        "1111","Lr","1"

; Right-to-left routines :

plot combineUm        "0000","Rl","0"
plot combineUmSrcDes  "0001","Rl",AND
plot combineUmSrcDes  "0010","Rl",BIC
plot combineUmSrc     "0011","Rl","Ni"
plot combineUmSrcDes  "0100","Rl",RBC
plot combineUmDesNop  "0101","Rl"
plot combineUmSrcDes  "0110","Rl",EOR
plot combineUmSrcDes  "0111","Rl",ORR
plot combineUmSrcDes  "1000","Rl",NOR
plot combineUmSrcDes  "1001","Rl",ENR
plot combineUmDesInvt "1010","Rl"
plot combineUmSrcDes  "1011","Rl",BST
plot combineUmSrc     "1100","Rl","Iv"
plot combineUmSrcDes  "1101","Rl",RBS
plot combineUmSrcDes  "1110","Rl",NAN
plot combineUm        "1111","Rl","1"

;-------------------------------------------------------------------
;                          Register usage in the masked opertaions
;-------------------------------------------------------------------

; In the above sections of code, code was compiled to implement the
;   sixteen combination modes between a row of source and a row of
;   destination colour-planes, where no mask was required. The
;   following code does the same except where a mask is required.

; The trouble with this code is that not only do words from the source
;   colour-plane need to be aligned with the destination colour-plane,
;   but the same applies to the mask colour plane:

; The following describes the register usage for these pieces of code:

;           Entry                     Exit
; R0  -  ++ Shift count               Preserved
; R1  -  ++ Mask pointer                +=/-= (MWC+1)*4
; R2  -     Source pointer              +=/-= (MWC+1)*4
; R3  -     Destination pointer         +=/-= (MWC+1)*4
; R4  -     MWC (see below)           Semi MWC (see below)
; R5  -     Aligned src preload       Destination pre-write
; R6  -     Unaligned src carry          Destroyed
; R7  -     Destination preload          Destroyed
; R8  -     First word mask           Preserved
; R9  -     Last word mask            Preserved
; R10 -  ++ Aligned mask preload         Destroyed
; R11 -  ++ Unaligned mask carry         Destroyed
; ip  -     Undefined                 Preserved
; sp  -     Undefined                 Preserved
; lr  -     Return address            Preserved
; pc  -     PC + N flag see below     lr with flags

; The general methods, techniques and register usages are similar
;    to that of the combination modes where a mask is not required,
;    except that the mask also has to be re-aligned. Those registers
;    marked with a '++' are the ones which behave differently from
;    the same registers in the case of unmasked operations.

; Bacause two realignments are now required, there need to be two
;    shift counts: one for the source and one for the mask. The
;    value of 32-<shift count> also has to be held for each of the
;    shift counts. The total of the four shift counts are held in the
;    four bytes of R0, and are accessed by rotating the register around:

;                    Left-to-Right               Right-to-left
; bits  0-7  :      <source shift count>   32 - <source shift count>
; bits  8-15 :       <mask shift count>    32 -  <mask shift count>
; bits 16-23 : 32 - <source shift count>     <source shift count>
; bits 24-31 : 32 -  <mask shift count>       <mask shift count>

; R1 now contains the mask pointer, which is the equivalent of the
;   source pointer, and the mask pre-load values are passed in
;   R10 and R11. These pre-load values being exactly the same sort
;   of thing as for the destination.

; Note that the four combination modes that are not dependent on
;    the source, or not dependent on either the source or destination
;    are not implemented with a mask, as they can all be translated 
;    into a mode where a stippled transparent source replaces the mask 
;    bit-image and then implemented more efficiently.

;-------------------------------------------------------------------
;                                      General operations : Masked
;-------------------------------------------------------------------

; This macro is for writing to the destination values which are
;   dependent on both the source and destination values.
;   (through a mask)

; The macro accepts three parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left
; $Op   : Mnemomic or macro name for the binary combination to use

        MACRO
$label  combineMsSrcDes $code,$h,$Op

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

        ; Set up directionally dependent information
        
        LCLS    desDir
        LCLS    shfPrv
        LCLS    shfNxt
        LCLS    srcAdr
        LCLS    mskAdr

desDir  SELECT  $h="Lr" , "4"       , "-4"
shfPrv  SELECT  $h="Lr" , "LSR R0"  , "LSL R0"
shfNxt  SELECT  $h="Lr" , "LSL R0"  , "LSR R0"
srcAdr  SELECT  $h="Lr" , "[R2],#4" , "[R2,#-4]!"
mskAdr  SELECT  $h="Lr" , "[R1],#4" , "[R1,#-4]!"

$label.Ms$id

        MSDIAG id                   ; Conditional diagnostics

        $Op    R5,R5,R7            ; Combine source and dest words
        AND    R10,R10,R8          ; Combine fisrt word mask and first
                                   ;   aligned mask pix-map word
        BIC    R7,R7,R10           ; Mask out bits to be written in dest
        AND    R5,R5,R10           ; Mask out bits not to write in src
        ORR    R5,R5,R7            ; Combine src and dest
        MOVMIS R15,R14             ; Return if only one word
;-------------------------------------------------------------------------+
hrzLoop$id                         ; Start of middle word loop            |
        STR    R5,[R3],#$desDir    ; Store previous dest word             |
        MOV    R5,R6,$shfPrv       ; Align and shift previous source word |
        MOV    R0,R0,ROR#8         ; Obtain next shift count              |
        MOV    R10,R11,$shfPrv     ; Align and shift previous mask word   |
        MOV    R0,R0,ROR#8         ; Obtain next shift count              |
        LDR    R6, $srcAdr         ; Load next source word                |
        LDR    R11,$mskAdr         ; Load next mask word                  |
        ORR    R5,R5,R6,$shfNxt    ; Combine aligned source words         |
        MOV    R0,R0,ROR#8         ; Obtain next shift count              |
        ORR    R10,R10,R11,$shfNxt ; Combine aligned mask words           |
        MOV    R0,R0,ROR#8         ; Rotate back to normal shift counts   |
        LDR    R7,[R3]             ; Load destination word to be altered  |
        $Op    R5,R5,R7            ; Operate between source and dest.     |
        AND    R5,R5,R10           ; Mask out bits not written from src   |
        BIC    R10,R7,R10          ; Mask out bits to be written in dest. |
        ORR    R5,R5,R10           ; Combine dest. and new bits           |
        SUBS   R4,R4,#1:SHL:16     ; Decrement and branch ...             |
        BCS    hrzLoop$id          ; ... continued                        |
;-------------------------------------------------------------------------+
        BIC    R7,R7,R9            ; Mask out dest. word with last word mask
        AND    R5,R5,R9            ; Mask out processed word with lst wrd msk
        ORR    R5,R5,R7            ; Combine processed word and destination
        MOVS   pc,lr               ; And return
        
        MEND

;-------------------------------------------------------------------
;                                   Source dependent only : Masked
;-------------------------------------------------------------------

; This macro is for writing to the destination values which are
;   dependent on the source and only the source pixmap values 
;   (through a mask)

; The macro accepts three parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left
; $i    : "Iv" => inverted source ; "Ni" => straight copy

        MACRO
$label  combineMsSrc    $code,$h,$i

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

        ; Set up directionally dependent information
        
        LCLS    desDir
        LCLS    shfPrv
        LCLS    shfNxt
        LCLS    srcAdr
        LCLS    mskAdr

desDir  SELECT  $h="Lr" , "4"       , "-4"
shfPrv  SELECT  $h="Lr" , "LSR R0"  , "LSL R0"
shfNxt  SELECT  $h="Lr" , "LSL R0"  , "LSR R0"
srcAdr  SELECT  $h="Lr" , "[R2],#4" , "[R2,#-4]!"
mskAdr  SELECT  $h="Lr" , "[R1],#4" , "[R1,#-4]!"

$label.Ms$id

        MSDIAG id                   ; Conditional diagnostics

        [ $i="Iv"                  ; If source to be inverted ...
        MVN    R5,R5               ; Invert source
        ]                          ; End of condition
        AND    R10,R10,R8          ; Combine fisrt word mask and first
                                   ;   aligned mask pix-map word
        BIC    R7,R7,R10           ; Mask out bits to be written in dest
        AND    R5,R5,R10           ; Mask out bits not to write in src
        ORR    R5,R5,R7            ; Combine src and dest
        MOVMIS R15,R14             ; Return if only one word
;-------------------------------------------------------------------------+
hrzLoopMs$id                       ; Start of middle word loop            |
        STR    R5,[R3],#$desDir    ; Store previous dest word             |
        MOV    R5,R6,$shfPrv       ; Align and shift previous source word |
        MOV    R0,R0,ROR#8         ; Obtain next shift count              |
        MOV    R10,R11,$shfPrv     ; Align and shift previous mask word   |
        MOV    R0,R0,ROR#8         ; Obtain next shift count              |
        LDR    R6, $srcAdr         ; Load next source word                |
        LDR    R11,$mskAdr         ; Load next mask word                  |
        ORR    R5,R5,R6,$shfNxt    ; Combine aligned source words         |
        MOV    R0,R0,ROR#8         ; Obtain next shift count              |
        ORR    R10,R10,R11,$shfNxt ; Combine aligned mask words           |
        MOV    R0,R0,ROR#8         ; Rotate back to normal shift counts   |
        LDR    R7,[R3]             ; Load destination word to be altered  |
        [ $i = "Iv"                ; If to invert source ...              |
        MVN    R5,R5               ; Then invert the source               |
        ]                          ; End of condition                     |
        AND    R5,R5,R10           ; Mask out bits not written from src   |
        BIC    R10,R7,R10          ; Mask out bits to be written in dest. |
        ORR    R5,R5,R10           ; Combine dest. and new bits           |
        SUBS   R4,R4,#1:SHL:16     ; Decrement and branch ...             |
        BCS    hrzLoopMs$id        ; ... continued                        |
;-------------------------------------------------------------------------+
        BIC    R7,R7,R9            ; Mask out dest. word with last word mask
        AND    R5,R5,R9            ; Mask out processed word with lst wrd msk
        ORR    R5,R5,R7            ; Combine processed word and destination
        MOVS   pc,lr               ; And return
        
        MEND

;-------------------------------------------------------------------
;                                 Direct write operations : Masked
;-------------------------------------------------------------------

; This macro is for writing into the destination either all ones, or
;    all zeros (through a mask)

; The macro accepts three parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left
; $v    : "0"  => all zeros       ; "1"  => all ones

        MACRO
$label  combineMs       $code,$h,$v

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

        ; Set up directionally dependent information
        
        LCLS    desDir
        LCLS    shfPrv
        LCLS    shfNxt
        LCLS    mskAdr
        LCLS    addSub

desDir  SELECT  $h="Lr" , "4"       , "-4"
shfPrv  SELECT  $h="Lr" , "LSR R0"  , "LSL R0"
shfNxt  SELECT  $h="Lr" , "LSL R0"  , "LSR R0"
mskAdr  SELECT  $h="Lr" , "[R1],#4" , "[R1,#-4]!"
addSub  SELECT  $h="Lr" , "ADD"     , "SUB"

$label.Ms$id

        MSDIAG id                   ; Conditional diagnostics

        AND    R10,R10,R8          ; Combine FWM and mask pix-map word
        [ $v="0"                   ; If writing zeros ...
        BIC    R5,R7,R10           ; Write zeros where appropriate in des
        |                          ; ... Else writing ones ...
        ORR    R5,R7,R10           ; Write ones where appropriate in des
        ]                          ; ... end of condition
        MOVMIS R15,R14             ; Return if only one word
        $addSub R2,R2,R4,LSR#14    ; Adjust source pointer as if
        $addSub R2,R2,#4           ;  it were to be used as well
        MOV    R0,R0,ROR#8         ; Point to mask shift count
;-------------------------------------------------------------------------+
hrzLoopMs$id                       ; Start of middle word loop            |
        STR    R5,[R3],#$desDir    ; Store previous dest word             |
        MOV    R10,R11,$shfPrv     ; Align and shift previous mask word   |
        MOV    R0,R0,ROR#16        ; Obtain next shift count              |
        LDR    R11,$mskAdr         ; Load next mask word                  |
        ORR    R10,R10,R11,$shfNxt ; Combine aligned mask words           |
        MOV    R0,R0,ROR#16        ; Rotate back to other mask shft count |
        LDR    R7,[R3]             ; Load destination word to be altered  |
        [ $v="0"                   ; If writing zeros ...                 |
        BIC    R5,R7,R10           ; Write appropriate zeros in des.      |
        |                          ; ... Otherwise ...                    |
        ORR    R5,R7,R10           ; Write appropriate ones in des.       |
        ]                          ; ... End Of Condition                 |
        SUBS   R4,R4,#1:SHL:16     ; Decrement and branch ...             |
        BCS    hrzLoopMs$id        ; ... continued                        |
;-------------------------------------------------------------------------+
        AND    R10,R10,R9          ; Combine last mask word with LWM
        [ $v="0"                   ; If writing zeros ...
        BIC    R5,R7,R10           ; Write zeros appropriately to des
        |                          ; ... Otherwise ...
        ORR    R5,R7,R10           ; Write appropriate ones to des
        ]                          ; ... End Of Condition
        MOV    R0,R0,ROR#24        ; Restore shift counts register
        MOVS   pc,lr               ; And return
        
        MEND

;-------------------------------------------------------------------
;                            Invert destination operation : Masked
;-------------------------------------------------------------------

; This macro for the operation to invert a destination (through a 
;   mask)

; The macro accepts two parameters:

; $code : Quoted binary four digit number corresponding to the combination
;              mode to be used.
; $h    : "Lr" => left-to-right   ; "Rl" => right-to-left

        MACRO
$label  combineMsDesInvt       $code,$h

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

        ; Set up directionally dependent information
        
        LCLS    desDir
        LCLS    shfPrv
        LCLS    shfNxt
        LCLS    mskAdr
        LCLS    addSub

desDir  SELECT  $h="Lr" , "4"       , "-4"
shfPrv  SELECT  $h="Lr" , "LSR R0"  , "LSL R0"
shfNxt  SELECT  $h="Lr" , "LSL R0"  , "LSR R0"
mskAdr  SELECT  $h="Lr" , "[R1],#4" , "[R1,#-4]!"
addSub  SELECT  $h="Lr" , "ADD"     , "SUB"

$label.Ms$id

        MSDIAG id                   ; Conditional diagnostics

        AND    R10,R10,R8          ; Combine FWM and mask pix-map word
        EOR    R5,R7,R10           ; Invert appropriate bits in des
        MOVMIS R15,R14             ; Return if only one word
        $addSub R2,R2,R4,LSR#14    ; Adjust source pointer as if it
        $addSub R2,R2,#4           ;   were to be used as well.
        MOV    R0,R0,ROR#8         ; Point to mask shift count
;-------------------------------------------------------------------------+
hrzLoopMs$id                       ; Start of middle word loop            |
        STR    R5,[R3],#$desDir    ; Store previous dest word             |
        MOV    R10,R11,$shfPrv     ; Align and shift previous mask word   |
        MOV    R0,R0,ROR#16        ; Obtain next shift count              |
        LDR    R11,$mskAdr         ; Load next mask word                  |
        ORR    R10,R10,R11,$shfNxt ; Combine aligned mask words           |
        MOV    R0,R0,ROR#16        ; Rotate back to other mask shft count |
        LDR    R7,[R3]             ; Load destination word to be altered  |
        EOR    R5,R7,R10           ; Invert appropriate sections of dest  |
        SUBS   R4,R4,#1:SHL:16     ; Decrement and branch ...             |
        BCS    hrzLoopMs$id        ; ... continued                        |
;-------------------------------------------------------------------------+
        AND    R10,R10,R9          ; Combine last mask word with LWM
        EOR    R5,R7,R10           ; Invert appropriate bits of des
        MOV    R0,R0,ROR#24        ; Restore shift counts register
        MOVS   pc,lr               ; And return
        
        MEND

;-------------------------------------------------------------------
;                                No operation combination : Masked
;-------------------------------------------------------------------

; This operation although apparently simple, has to alter registers
;   as prescribed in the register usage for masked operations
;  shown above, so ...

        MACRO
$label  combineMsDesNop $code,$h

        ; Set up a local string to append to all labels which
        ;    makes them unique
        LCLS   id
id      SETS   $h :CC: "_" :CC: $code

$label.Ms$id

        MSDIAG id                   ; Conditional diagnostics

        BMI    mwcNegMs$id          ; Trap MWC is -1
        [ $h = "Lr"                 ; If going left-to-right ...
        ADD    R1,R1,R4,LSR#14      ; Add (MWC+1)*4 to R1 ...
        ADD    R1,R1,#4             ; ... continued
        ADD    R2,R2,R4,LSR#14      ; Add (MWC+1)*4 to R2 ...
        ADD    R2,R2,#4             ; ... continued
        ADD    R3,R3,R4,LSR#14      ; Add (MWC+1)*4 to R3, and load
        LDR    R5,[R3,#4]!          ;   destination write-back value
        |                           ; ... Else If right-to-left ...
        SUB    R1,R1,R4,LSR#14      ; Subtract (MWC+1)*4 from R1 ...
        SUB    R1,R1,#4             ; ... continued
        SUB    R2,R2,R4,LSR#14      ; Subtract (MWC+1)*4 from R2 ...
        SUB    R2,R2,#4             ; ... continued
        SUB    R3,R3,R4,LSR#14      ; Subtract (MWC+1)*4 from R3, and load
        LDR    R5,[R3,#4]!          ;   destination write-back value
        ]                           ; ... End Of Condition
        MOVS   pc,lr                ; Return
mwcNegMs$id                         ; Here if MWC is -1
        MOV    R5,R7                ; Get destination write-back
        MOVS   pc,lr                ; And return
        
        MEND
        
;-------------------------------------------------------------------
;                           Masked combination routine diagnostics 
;-------------------------------------------------------------------

; This macro can (conditionally) cause this file to generate diagnostics
;    which are printed on entry to any of the above masked
;    combination routines ...

        MACRO
$l      MSDIAG $id
        [ {FALSE}
$l
        DIAG "Masked combination routine entered : " :CC: $id
        ]
        MEND

;-------------------------------------------------------------------
;                        Macro invokations : Masked combinations
;-------------------------------------------------------------------

; We invoke the macros for masked combination below:

; See above for a description of the macros RBC,NOR,ENR,BST,RBS,NAN

plot combineMs        "0000","Lr","0"
plot combineMsSrcDes  "0001","Lr",AND
plot combineMsSrcDes  "0010","Lr",BIC
plot combineMsSrc     "0011","Lr","Ni"
plot combineMsSrcDes  "0100","Lr",RBC
plot combineMsDesNop  "0101","Lr"
plot combineMsSrcDes  "0110","Lr",EOR
plot combineMsSrcDes  "0111","Lr",ORR
plot combineMsSrcDes  "1000","Lr",NOR
plot combineMsSrcDes  "1001","Lr",ENR
plot combineMsDesInvt "1010","Lr"
plot combineMsSrcDes  "1011","Lr",BST
plot combineMsSrc     "1100","Lr","Iv"
plot combineMsSrcDes  "1101","Lr",RBS
plot combineMsSrcDes  "1110","Lr",NAN
plot combineMs        "1111","Lr","1"

; Right-to-left routines :

plot combineMs        "0000","Rl","0"
plot combineMsSrcDes  "0001","Rl",AND
plot combineMsSrcDes  "0010","Rl",BIC
plot combineMsSrc     "0011","Rl","Ni"
plot combineMsSrcDes  "0100","Rl",RBC
plot combineMsDesNop  "0101","Rl"
plot combineMsSrcDes  "0110","Rl",EOR
plot combineMsSrcDes  "0111","Rl",ORR
plot combineMsSrcDes  "1000","Rl",NOR
plot combineMsSrcDes  "1001","Rl",ENR
plot combineMsDesInvt "1010","Rl"
plot combineMsSrcDes  "1011","Rl",BST
plot combineMsSrc     "1100","Rl","Iv"
plot combineMsSrcDes  "1101","Rl",RBS
plot combineMsSrcDes  "1110","Rl",NAN
plot combineMs        "1111","Rl","1"

;-------------------------------------------------------------------
;                                                      End-Of-File
;-------------------------------------------------------------------

        ; If in Helios Mode, send a directive ...
        
        [ heliosMode
        EndModule
        ]

        END

