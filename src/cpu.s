    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT divTable

    EXPORT proj_cpu_asm

dst     RN r0
src     RN r1
matrix  RN r2
count   RN r3
last    RN count

x       RN r4
y       RN r5
z       RN r6

mx      RN r7
my      RN r8
mz      RN r9

cx      RN r10
cy      RN r11
cz      RN r12

tmp     RN mz

divLUT  RN lr

CENTER_X        EQU ((320/2) << 13)
CENTER_Y        EQU ((240/2) << 13)

proj_cpu_asm
        stmfd sp!, {r4-r11, lr}

        add last, dst, count, lsl #3

        ldr divLUT, =divTable

loop    ldmia src!, {x, y, z}

        ldmia matrix!, {mx, my, mz, cx}
        mla cx, mx, x, cx
        mla cx, my, y, cx
        mla cx, mz, z, cx

        ldmia matrix!, {mx, my, mz, cy}
        mla cy, mx, x, cy
        mla cy, my, y, cy
        mla cy, mz, z, cy

        ldmia matrix!, {mx, my, mz, cz}
        mla cz, mx, x, cz
        mla cz, my, y, cz
        mla cz, mz, z, cz

        sub matrix, matrix, #48

        mov x, cx, asr #16
        mov y, cy, asr #16

        mov z, cz, lsr #(3 + 16)
        ldr z, [divLUT, z, lsl #2]

        mul tmp, x, z
        add x, tmp, #CENTER_X

        mul tmp, y, z
        add y, tmp, #CENTER_Y

        stmia dst!, {x, y}

        cmp dst, last
        blt loop

        ldmfd sp!, {r4-r11, pc}
    END
