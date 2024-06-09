    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT divTable

    EXPORT proj_v4m4_super_asm

CENTER_X        EQU ((320/2) << 13)
CENTER_Y        EQU ((240/2) << 13)

MADAM           EQU 0x03300000
MATH_STACK      EQU 0x0600
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
w       RN r8

px      RN z
py      RN r9

output  RN r10
cmd     RN r11

divLUT  RN r12

input   RN lr

proj_v4m4_super_asm
        stmfd sp!, {r4-r11, lr}

        add last, dst, count, lsl #3

        mov	input, #(MADAM)
        add	input, input, #(MATH_STACK) ; MATRIX00

        ldmia matrix!, {r4-r11}
        stmia input!, {r4-r11}
        ldmia matrix!, {r4-r11}
        stmia input!, {r4-r11}          ; input = B0

        add output, input, #(8 * 4)     ; output = B1
        mov cmd, #(CMD_V4M4)

        mov w, #1 ; HACK TIPS: we can use cmd instead to save register! 8)

        ; start transform of the first vertex
        ldmia src!, {x, y, z}
        stmia input, {x, y, z, w} ; (x, y, z, 1)
        str cmd, [input, #(MATH_START - MATH_STACK - 0x40)]

        ; pre-load division table pointer
        ldr divLUT, =divTable

        mov ox, #CENTER_X
        mov oy, #CENTER_Y

loop
        ; read next vertex
        ldmia src!, {x, y, z}
        ; write the next vertex (MADAM had enough time)
        stmia input, {x, y, z, w}
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

        ldmfd sp!, {r4-r11, pc}
    END
