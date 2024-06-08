    AREA |C$$code|, CODE, READONLY
|x$codeseg|

MulManyVec3Mat33_F16 EQU (0x50000 + 2)

    IMPORT divTable

    EXPORT proj_v3m3_asm

x       RN r0
y       RN r1
z       RN r2
count   RN r3

dst     RN r4
src     RN r5
last    RN r6

cx      RN r7
cy      RN r8
cz      RN r9

ox      RN r10
oy      RN r12

px      RN z
py      RN count

divLUT  RN lr

CENTER_X        EQU ((320/2) << 13)
CENTER_Y        EQU ((240/2) << 13)

proj_v3m3_asm
        stmfd sp!, {r4-r10, lr}

        add src, r2, #36
        ldmia src, {cx, cy, cz}

        mov dst, r0
        mov src, dst
        add last, dst, count, lsl #3

        swi MulManyVec3Mat33_F16

        ldr divLUT, =divTable

        mov ox, #CENTER_X
        mov oy, #CENTER_Y

loop    ldmia src!, {x, y, z}

        add x, x, cx
        add y, y, cy
        add z, z, cz

        mov z, z, lsr #3

        ldr z, [divLUT, z, lsl #2]
        mla py, y, z, oy
        mla px, x, z, ox

        stmia dst!, {px, py}

        cmp dst, last
        blt loop

        ldmfd sp!, {r4-r10, pc}
    END
