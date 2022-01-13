    .section .text
    .file	"hello_sfc_asm.s"
    .globl	_nmi
    .type	_nmi,@function
_nmi:
    PHA
    LDA $4210
    PLA
    RTI
