#include <init_stack.S>

    .section .init
    .file	"crt0.s"
    .globl	_start
    .type	_start,@function
_start:
    SEP
    LDX #$FF
    LDA #$00
.Lclrpg0:
    STA $00,x
    DEX
    BNE .Lclrpg0
    STA $00
    CLP

    __init_stack
    JSR main	;App entry point

.Lforever:
    JMP .Lforever
