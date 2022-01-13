#include <init_stack.S>

    .section .init
    .file	"crt0.s"
    .globl	_start
    .type	_start,@function
_start:
    SEI			;disable interrupts
    CLD			;decimal mode off

    LDX	#$FF	;Setup the stack
    TXS			;Transfer Index X to Stack Pointer Register

    LDA #$8F
    STA $2100		;turn screen off for now, set brightness to normal

    LDY #$00

    LDX #$01
    LDA #$00
.L_Loop00:		;regs $2101-$210C
    STA $2100,x		;set Sprite,Character,Tile sizes to lowest, and set addresses to $0000
    INX
    CPX #$0D
    BNE .L_Loop00

.L_Loop01:		;regs $210D-$2114
    STA $2100,x		;Set all BG scroll values to $0000
    STA $2100,x
    INX
    CPX #$15
    BNE .L_Loop01

    LDA #$80		;reg $2115
    STA $2115		; Initialize VRAM transfer mode to word-access, increment by 1

    STZ $2116		;regs $2117-$2117
    STZ $2117		;VRAM address = $0000

            ;reg $2118-$2119
            ;VRAM write register... don't need to initialize

    STZ $211A		;clear Mode7 setting

    LDX #$1B
    LDA #$00
.L_Loop02:		;regs $211B-$2120
    STA $2100,x		;clear out the Mode7 matrix values
    STA $2100,x
    INX
    CPX #$21
    BNE .L_Loop02

            ;reg $2121 - Color address, doesn't need initilaizing
            ;reg $2122 - Color data, is initialized later

    LDX #$23
.L_Loop03:		;regs $2123-$2133
    STA $2100,x		;turn off windows, main screens, sub screens, color addition,
    INX			;fixed color = $00, no super-impose (external synchronization),
    CPX #$34	;no interlaced mode, normal resolution
    BNE .L_Loop03

            ;regs $2134-$2136  - multiplication result, no initialization needed
            ;reg $2137 - software H/V latch, no initialization needed
            ;reg $2138 - Sprite data read, no initialization needed
            ;regs $2139-$213A  - VRAM data read, no initialization needed
            ;reg $213B - Color RAM data read, no initialization needed
            ;regs $213C-$213D  - H/V latched data read, no initialization needed

    STZ $213E		;reg $213E - might not be necesary, but selects PPU master/slave mode
            ;reg $213F - PPU status flag, no initialization needed

            ;reg $2140-$2143 - APU communication regs, no initialization required

            ;reg $2180  -  read/write WRAM register, no initialization required
            ;reg $2181-$2183  -  WRAM address, no initialization required

            ;reg $4016-$4017  - serial JoyPad read registers, no need to initialize


    STZ $4200		;reg $4200  - disable timers, NMI,and auto-joyread

    LDA #$FF
    STA $4201		;reg $4201  - programmable I/O write port, initalize to allow reading at in-port

            ;regs $4202-$4203  - multiplication registers, no initialization required
            ;regs $4204-$4206  - division registers, no initialization required

            ;regs $4207-$4208  - Horizontal-IRQ timer setting, since we disabled this, it is OK to not init
            ;regs $4209-$420A  - Vertical-IRQ timer setting, since we disabled this, it is OK to not init

    STZ $420B		;reg $420B  - turn off all general DMA channels
    STZ $420C		;reg $420C  - turn off all H-MA channels

    STZ $420D		;reg $420D  - ROM access time to slow (2.68Mhz)

    LDA $4210		;reg $4210  - NMI status, reading resets

            ;reg $4211  - IRQ status, no need to initialize
            ;reg $4212  - H/V blank and JoyRead status, no need to initialize
            ;reg $4213  - programmable I/O inport, no need to initialize

            ;reg $4214-$4215  - divide results, no need to initialize
            ;reg $4216-$4217  - multiplication or remainder results, no need to initialize

            ;regs $4218-$421f  - JoyPad read registers, no need to initialize

            ;regs $4300-$437F
            ;no need to intialize because DMA was disabled above
            ;also, we're not sure what all of the registers do, so it is better to leave them at
            ;their reset state value

    JSR ClearVRAM      ;Reset VRAM
    JSR ClearPalette   ;Reset colors

    ;**** clear Sprite tables ********

    STZ $2102	;sprites initialized to be off the screen, palette 0, character 0
    STZ $2103
    LDX #$80
    LDA #$F0
.L_Loop08:
    STA $2104	;set X = 240
    STA $2104	;set Y = 240
    STZ $2104	;set character = $00
    STZ $2104	;set priority=0, no flips
    DEX
    BNE .L_Loop08

    LDX #$20
.L_Loop09:
    STZ $2104		;set size bit=0, x MSB = 0
    DEX
    BNE .L_Loop09

    ;**** clear WRAM ********

    STZ $2181		      ;Set WRAM address to $000000
    STZ $2182
    STZ $2183

    LDX #$08
    STX $4300         ;Set DMA mode to fixed source, BYTE to $2180
    LDX #$80
    STX $4301         ;Set DMA mode to fixed source, BYTE to $2180
    LDA #mos16lo(.Lwram_fill_byte)
    LDX #mos16hi(.Lwram_fill_byte)
    STA $4302         ;Set source offset
    STX $4303         ;Set source offset
    STZ $4304         ;Set source bank
    STZ $4305         ;Set transfer size to 64k bytes
    LDA #$01
    STA $420B         ;Initiate transfer

    LDA #$01          ;now set the next 64k bytes
    STA $420B         ;Initiate transfer

    ; .data section copy rom->ram
    LDA #mos16lo(__data_dma_length)
    BNE .LDataDMA
    LDA #mos16hi(__data_dma_length)
    BEQ .LSkipDataDMA
.LDataDMA:
    LDA #mos16lo(__start_data_ram)
    LDX #mos16hi(__start_data_ram)
    STA $2181		      ;Set WRAM address
    STX $2182
    STZ $2183
    STZ $4300         ;Set DMA mode to non-fixed source, BYTE to $2180
    LDX #$80
    STX $4301         ;Set DMA mode to non-fixed source, BYTE to $2180
    LDA #mos16lo(__start_data_rom)
    LDX #mos16hi(__start_data_rom)
    STA $4302         ;Set source offset
    STX $4303         ;Set source offset
    STZ $4304         ;Set source bank
    LDA #mos16lo(__data_dma_length)
    LDX #mos16hi(__data_dma_length)
    STA $4305         ;Set transfer size
    STX $4306         ;Set transfer size
    LDA #$01
    STA $420B         ;Initiate transfer

.LSkipDataDMA:
    CLI			;enable interrupts again

    __init_stack
    JSR main	;App entry point

.Lforever:
    JMP .Lforever

ClearVRAM:
    LDA #$80
    STA $2115         ;Set VRAM port to word access
    LDX #$09
    STX $4300         ;Set DMA mode to fixed source, WORD to $2118/9
    LDX #$18
    STX $4301         ;Set DMA mode to fixed source, WORD to $2118/9
    STZ $2116         ;Set VRAM port address to $0000
    LDX #mos16lo(.Lwram_fill_byte)
    STX $4302         ;Set source address to $xx:0000
    LDX #mos16hi(.Lwram_fill_byte)
    STX $4303         ;Set source address to $xx:0000
    STZ $4304         ;Set source bank to $00
    LDX #$FF
    STX $4305         ;Set transfer size to 64k-1 bytes
    STX $4306         ;Set transfer size to 64k-1 bytes
    LDA #$01
    STA $420B         ;Initiate transfer
    STZ $2119         ;clear the last byte of the VRAM
    RTS

ClearPalette:
    STZ $2121
    LDX #$FF
.LClearPaletteLoop:
    STZ $2122
    STZ $2122
    DEX
    BNE .LClearPaletteLoop
    STZ $2122
    STZ $2122
    RTS

.Lwram_fill_byte:
    .byte $00
    .byte $00
