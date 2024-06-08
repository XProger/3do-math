    AREA |C$$code|, CODE, READONLY
|x$codeseg|

MulManyVec3Mat33_F16 EQU (0x50000 + 2)

    IMPORT divTable

    EXPORT proj_v3m3_super_asm

CENTER_X        EQU ((320/2) << 13)
CENTER_Y        EQU ((240/2) << 13)

MADAM           EQU 0x03300000
MATH_STACK      EQU (MADAM + 0x0600)
MATH_STATUS     EQU (MADAM + 0x07F8)
MATH_START      EQU (MADAM + 0x07FC)

;CMD_V3M3        EQU 1   ; red
CMD_V3M3        EQU 2   ; green

dst     RN r0
src     RN r1
matrix  RN r2
count   RN r3
last    RN count

cx      RN matrix
cy      RN r4
cz      RN r5

x       RN r6
y       RN r7
z       RN r8

px      RN z
py      RN r9

output  RN r10
cmd     RN r11

divLUT  RN r12

input   RN lr

proj_v3m3_super_asm
        stmfd sp!, {r4-r11, lr}

        add last, dst, count, lsl #3

        mov	input, #(MADAM)
        add	input, input, #(MATH_STACK - MADAM) ; MATRIX00

        ldmia matrix!, {r4-r12}
        stmia input!, {r4, r7, r10}
        add input, input, #4
        stmia input!, {r5, r8, r11}
        add input, input, #4
        stmia input, {r6, r9, r12}

        add input, input, #(8 * 4)     ; B0
        add output, input, #(8 * 4)    ; B1
        mov cmd, #(CMD_V3M3)

        ; start transform of the first vertex
        ldmia src!, {x, y, z}
        stmia input, {x, y, z}
        str cmd, [input, #(MATH_START - MATH_STACK - 0x40)]

        ; load matrix offset
        ldmia matrix, {cx, cy, cz}

        ; pre-load division table pointer
        ldr divLUT, =divTable

loop
        ; read next vertex
        ldmia src!, {x, y, z}
        ; write the next vertex (MADAM had enough time)
        stmia input, {x, y, z}
        ; read transformed result
        ldmia output, {x, y, z}
        ; start next vertex transform
        str cmd, [input, #(MATH_START - MATH_STACK - 0x40)]

        ; apply matrix offset
        add x, x, cx
        add y, y, cy
        add z, z, cz

        ; project
        mov z, z, lsr #3
        ldr z, [divLUT, z, lsl #2]
        ;mla py, y, z, oy
        ;mla px, x, z, ox
        mul py, y, z
        add py, py, #CENTER_Y

        mul px, x, z
        add px, px, #CENTER_X

        ; write the result
        ;stmia dst!, {px, py}
        strt px, [dst], #4
        strt py, [dst], #4

        cmp dst, last
        blt loop

        ldmfd sp!, {r4-r11, pc}
    END
