#include <init_stack.S>

    .section .init
    .file	"crt0.s"
    .globl	_start
    .type	_start,@function
_start:
    ; Zero BSS
    ; Use first two bytes as page pointer
    LDA #0
    STA $0
    LDX #mos16hi(__start_bss)
    STX $1
    ; X indexes within page, we always zero to the end of the last page
    LDY #mos16lo(__start_bss)
.Lloop1:
    STA ($0),y
    INY
    BNE .Lloop1
    ; Check for completion
    CPX #mos16hi(__end_bss)
    BEQ .Ldone
    ; Next page
    INX
    STX $1
    BRA .Lloop1

.Ldone:
    __init_stack
    JSR __main	;App entry point

.Lforever:
    JMP .Lforever
