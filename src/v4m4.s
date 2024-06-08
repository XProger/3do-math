    AREA |C$$code|, CODE, READONLY
|x$codeseg|

MulManyVec4Mat44_F16 EQU (0x50000 + 9)

    IMPORT divTable

    EXPORT proj_v4m4_asm

x       RN r0
y       RN r1
z       RN r2
count   RN r3

last    RN r4
dst     RN r5
src     RN r6

ox      RN r7
oy      RN r12

py      RN count
px      RN z

divLUT  RN lr

CENTER_X        EQU ((320/2) << 13)
CENTER_Y        EQU ((240/2) << 13)

proj_v4m4_asm
        stmfd sp!, {r4-r7, lr}

        mov dst, r0
        add last, dst, count, lsl #3

        swi MulManyVec4Mat44_F16

        mov src, dst
        ldr divLUT, =divTable

        mov ox, #CENTER_X
        mov oy, #CENTER_Y

loop    ldmia src, {x, y, z}
        add src, src, #16

        mov z, z, lsr #3

        ldr z, [divLUT, z, lsl #2]
        mla py, y, z, oy
        mla px, x, z, ox

        stmia dst!, {px, py}

        cmp dst, last
        blt loop

        ldmfd sp!, {r4-r7, pc}
    END
