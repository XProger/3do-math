    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT divTable

    EXPORT proj_v4m4_super_asm

CENTER_X        EQU ((320/2) << 13)
CENTER_Y        EQU ((240/2) << 13)

MADAM           EQU 0x03300000
MATH_STACK      EQU 0x0600
MATH_STATUS     EQU 0x07F8
MATH_START      EQU 0x07FC

CMD_V4M4        EQU 1

dst     RN r0
src     RN r1
matrix  RN r2
count   RN r3
last    RN count

ox      RN matrix
oy      RN r4

x       RN r5
y       RN r6
z       RN r7
w       RN r8 ; unused

px      RN z
py      RN r9

output  RN r10
cmd     RN r11

divLUT  RN r12

input   RN lr

proj_v4m4_super_asm
        stmfd sp!, {r4-r11, lr}

        mov	input, #(MADAM)
        add	input, input, #(MATH_STACK) ; MATRIX00

        ldmia matrix!, {r4-r11}
        stmia input!, {r4-r11}
        ldmia matrix!, {r4-r11}
        stmia input!, {r4-r11}          ; input = B0

        add output, input, #(8 * 4)     ; output = B1

        mov cmd, #(CMD_V4M4)

        ; start transform of the first vertex
        ldmia src!, {x, y, z}
        stmia input, {x, y, z} ; don't change 4th component (detault w = 1)
        str cmd, [input, #(MATH_START - MATH_STACK - 0x40)]

        ; pre-load division table pointer
        ldr divLUT, =divTable

        mov ox, #CENTER_X
        mov oy, #CENTER_Y

        add last, dst, count, lsl #3

        ; fill 17 nop's to get enough time to calc the first vertex
    GBLA cnt
cnt SETA 0
    WHILE cnt < 17
cnt SETA cnt+1
        nop
    WEND

loop
        ; read next vertex
        ldmia src!, {x, y, z}

    IF {FALSE}
        ; for debugging purposes, fail if the result isn't ready
        ldr w, [input, #(MATH_STATUS - MATH_STACK - 0x40)]
        tst w, #1
        bne stop
    ENDIF
        ; write the next vertex (MADAM had enough time)
        stmia input, {x, y, z}
        ; read transform result
        ldmia output, {x, y, z}
        ; start next vertex transform
        str cmd, [input, #(MATH_START - MATH_STACK - 0x40)]

        ; project
        mov z, z, lsr #3
        ldr z, [divLUT, z, lsl #2]
        mla py, y, z, oy
        mla px, x, z, ox

        ; write the result
        stmia dst!, {px, py}

        cmp dst, last
        blt loop
stop
        ldmfd sp!, {r4-r11, pc}
    END
