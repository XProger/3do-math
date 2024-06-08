#include <displayutils.h>
#include <debug.h>
#include <nodes.h>
#include <kernelnodes.h>
#include <list.h>
#include <folio.h>
#include <audio.h>
#include <task.h>
#include <kernel.h>
#include <mem.h>
#include <operamath.h>
#include <semaphore.h>
#include <io.h>
#include <event.h>
#include <controlpad.h>
#include <graphics.h>
#include <celutils.h>
#include <timerutils.h>
#include <hardware.h>

#include "folio/super.h"

#include "math_tables.h"

#include "stdio.h"

#define FIX_SHIFT 14
#define SIZE 17
#define SCALE 8

#define CENTER_X ((320 / 2) << 13)
#define CENTER_Y ((240 / 2) << 13)

typedef struct {
    sint32 e[3][4];
} mat34i;

typedef struct {
    sint32 e[4][3];
} mat43i;

typedef struct {
    sint32 e[4][4];
} mat44i;

typedef struct {
    sint32 x, y;
} vec2i;

typedef struct {
    sint32 x, y, z;
} vec3i;

typedef struct {
    sint32 x, y, z, w;
} vec4i;

ScreenContext context;

Item irqVBL;
Item irqVRAM;
Item irqTimer;

sint32 fps_frame;
sint32 fps_time;
sint32 fps;

uint32 time_trans;
uint32 time_prep;
uint32 time_draw;

uint32 angleY;
mat34i m0;
//mat43i m1 = m0
mat43i m2;
//mat44i m3 = m2
mat44i m4;

uint32 old_pad;
sint32 mode_index = 0;

vec3i cube_v3[SIZE * SIZE * SIZE + 1];
vec4i cube_v4[SIZE * SIZE * SIZE + 1];
vec4i cube_out[SIZE * SIZE * SIZE];
CCB ccb[SIZE * SIZE * SIZE];

uint16 plut = 0x7FFF;

IOInfo clearInfo = {
    FLASHWRITE_CMD, IO_QUICK, 0, 0, -1, 0, 0,
    { NULL, 0 },
    { NULL, 320 * 240 * 2 }
};

typedef struct {
    const char *name;
    void (*proj)(void *dst, void *src, void *matrix, sint32 count);
    void *src;
    void *matrix;
} TEST_MODE;

void proj_cpu_c(void *dst, void *src, void *matrix, sint32 count);
void proj_cpu_asm(void *dst, void *src, void *matrix, sint32 count);
void proj_v3m3_c(void *dst, void *src, void *matrix, sint32 count);
void proj_v3m3_asm(void *dst, void *src, void *matrix, sint32 count);
void proj_v4m4_c(void *dst, void *src, void *matrix, sint32 count);
void proj_v4m4_asm(void *dst, void *src, void *matrix, sint32 count);
void proj_v3m3_super_asm(void *dst, void *src, void *matrix, sint32 count);
void proj_v3m3_super(void *dst, void *src, void *matrix, sint32 count);

static const TEST_MODE modes[] = {
    { "CPU C", proj_cpu_c, cube_v3, &m0 },
    { "CPU ASM", proj_cpu_asm, cube_v3, &m0 },
    { "MulMany Vec3Mat33 C", proj_v3m3_c, cube_v3, &m2 },
    { "MulMany Vec3Mat33 ASM", proj_v3m3_asm, cube_v3, &m2 },
    { "MulMany Vec4Mat44 C", proj_v4m4_c, cube_v4, &m4 },
    { "MulMany Vec4Mat44 ASM", proj_v4m4_asm, cube_v4, &m4 },
    { "MulMany Vec3Mat33 ASM SUPER", proj_v3m3_super, cube_v3, &m2 },
};

#define X_SINCOS(x,s,c) {\
    sint32 sc = gSinCosTable[x & 4095];\
    s = sc >> 16;\
    c = sc << 16 >> 16;\
}

void init(void)
{
    sint32 x, y, z;
    vec3i *v3 = cube_v3;
    vec4i *v4 = cube_v4;
    CCB *dst = ccb;
    CCB *last = ccb + SIZE * SIZE * SIZE;

    for (z = 0; z < SIZE; z++)
    {
        for (y = 0; y < SIZE; y++)
        {
            for (x = 0; x < SIZE; x++)
            {
                // vertex coords are positive, which is important for the fast ARMv3 mul
                v4->x = v3->x = (x << SCALE);
                v4->y = v3->y = (y << SCALE);
                v4->z = v3->z = (z << SCALE);
                v4->w = 1;

                v3++;
                v4++;
            }
        }
    }

    memset(dst, 0, sizeof(CCB) * SIZE * SIZE * SIZE);
    while (dst < last)
    {
        dst->ccb_NextPtr = dst + 1;
        dst->ccb_Flags =
            CCB_NPABS  |
            CCB_SPABS  |
            CCB_PPABS  |
            CCB_LDSIZE |
            CCB_LDPRS  |
            CCB_LDPPMP |
            CCB_LDPLUT |
            CCB_CCBPRE |
            CCB_YOXY   |
            CCB_ACSC   | CCB_ALSC |
            CCB_ACW    | CCB_ACCW |
            CCB_ACE    |
            CCB_NOBLK  |
            CCB_BGND;

        dst->ccb_HDX = 1 << 20;
        dst->ccb_HDY = 0;
        dst->ccb_VDX = 0;
        dst->ccb_VDY = 1 << 16;
        dst->ccb_HDDX = 0;
        dst->ccb_HDDY = 0;
        dst->ccb_PIXC = PPMPC_MF_8 | PPMPC_SF_8;
        dst->ccb_PRE0 = PRE0_BGND | PRE0_LINEAR | PRE0_BPP_16;
        dst->ccb_PRE1 = PRE1_TLLSB_PDC0;
        dst->ccb_SourcePtr = (CelData*)&plut;

        dst++;
    }
    LAST_CEL((last - 1));
}

void update(void)
{
    ControlPadEventData event;
    if (GetControlPad(1, 0, &event))
    {
        uint32 new_pad = event.cped_ButtonBits;
        uint32 state = (new_pad ^ old_pad) & new_pad;
        old_pad = new_pad;

        if (state & ControlA)
        {
            mode_index = (mode_index + 1) % (sizeof(modes) / sizeof(modes[0]));
        }
    }

    angleY += 4;

    {
        sint32 s, c, d;

        X_SINCOS(angleY, s, c);
        d = -((SIZE - 1) << (SCALE - 1));

        memset(&m0, 0, sizeof(m0));
        memset(&m2, 0, sizeof(m2));
        memset(&m4, 0, sizeof(m4));

        m0.e[0][0] = c;
        m0.e[0][2] = s;
        m0.e[1][1] = 1 << FIX_SHIFT;
        m0.e[2][0] = -s;
        m0.e[2][2] = c;
        m0.e[0][3] = (m0.e[0][0] + m0.e[0][1] + m0.e[0][2]) * d;
        m0.e[1][3] = (m0.e[1][0] + m0.e[1][1] + m0.e[1][2]) * d;
        m0.e[2][3] = (m0.e[2][0] + m0.e[2][1] + m0.e[2][2]) * d;
        m0.e[2][3] += 1024 << 16;

        m2.e[0][0] = c;
        m2.e[2][0] = s;
        m2.e[1][1] = 1 << FIX_SHIFT;
        m2.e[0][2] = -s;
        m2.e[2][2] = c;
        m2.e[3][0] = (m2.e[0][0] + m2.e[1][0] + m2.e[2][0]) * d >> 16;
        m2.e[3][1] = (m2.e[0][1] + m2.e[1][1] + m2.e[2][1]) * d >> 16;
        m2.e[3][2] = (m2.e[0][2] + m2.e[1][2] + m2.e[2][2]) * d >> 16;
        m2.e[3][2] += 1024;

        m4.e[0][0] = c;
        m4.e[2][0] = s;
        m4.e[1][1] = 1 << FIX_SHIFT;
        m4.e[0][2] = -s;
        m4.e[2][2] = c;
        m4.e[3][0] = (m4.e[0][0] + m4.e[1][0] + m4.e[2][0]) * d;
        m4.e[3][1] = (m4.e[0][1] + m4.e[1][1] + m4.e[2][1]) * d;
        m4.e[3][2] = (m4.e[0][2] + m4.e[1][2] + m4.e[2][2]) * d;
        m4.e[3][2] += 1024 << 16;
    }
}

void proj_cpu_c(void *dst, void *src, void *matrix, sint32 count)
{
    mat34i *m = (mat34i*)matrix;
    vec3i *s = (vec3i*)src;
    vec2i *d = (vec2i*)dst;
    vec2i *last = d + count;

    while (d < last)
    {
        sint32 x = s->x;
        sint32 y = s->y;
        sint32 z = s->z;

        sint32 cx = (m->e[0][0] * x + m->e[0][1] * y + m->e[0][2] * z + m->e[0][3]) >> 16;
        sint32 cy = (m->e[1][0] * x + m->e[1][1] * y + m->e[1][2] * z + m->e[1][3]) >> 16;
        sint32 cz = (m->e[2][0] * x + m->e[2][1] * y + m->e[2][2] * z + m->e[2][3]) >> 16;

        z = divTable[cz >> 3];
        x = CENTER_X + cx * z;
        y = CENTER_Y + cy * z;

        d->x = x;
        d->y = y;

        s++;
        d++;
    }
}

void proj_v3m3_c(void *dst, void *src, void *matrix, sint32 count)
{
    mat43i *m = (mat43i*)matrix;
    vec3i *s = (vec3i*)dst;
    vec2i *d = (vec2i*)dst;
    vec2i *last = d + count;

    sint32 cx = m->e[3][0];
    sint32 cy = m->e[3][1];
    sint32 cz = m->e[3][2];

    MulManyVec3Mat33_F16((vec3f16*)dst, (vec3f16*)src, *(mat33f16*)matrix, count);

    while (d < last)
    {
        sint32 x = s->x;
        sint32 y = s->y;
        sint32 z = s->z;

        x += cx;
        y += cy;
        z += cz;

        z = divTable[z >> 3];
        x = CENTER_X + x * z;
        y = CENTER_Y + y * z;

        d->x = x;
        d->y = y;

        s++;
        d++;
    }
}

void proj_v4m4_c(void *dst, void *src, void *matrix, sint32 count)
{
    vec4i *s = (vec4i*)dst;
    vec2i *d = (vec2i*)dst;
    vec2i *last = d + count;

    MulManyVec4Mat44_F16((vec4f16*)dst, (vec4f16*)src, *(mat44f16*)matrix, count);

    while (d < last)
    {
        sint32 x = s->x;
        sint32 y = s->y;
        sint32 z = s->z;

        z = divTable[z >> 3];
        x = CENTER_X + x * z;
        y = CENTER_Y + y * z;

        d->x = x;
        d->y = y;

        s++;
        d++;
    }
}

void* super_dst, *super_src, *super_matrix;
sint32 super_count;

void proj_v3m3_super_wrap(void)
{
    proj_v3m3_super_asm(super_dst, super_src, super_matrix, super_count);
}

void proj_v3m3_super(void *dst, void *src, void *matrix, sint32 count)
{
    super_dst = dst;
    super_src = src;
    super_matrix = matrix;
    super_count = count;
    Super(proj_v3m3_super_wrap);
}

void prep(void)
{
    vec2i *vert = (vec2i*)cube_out;
    CCB *src = ccb;
    CCB *last = src + SIZE * SIZE * SIZE;

    while (src < last)
    {
        src->ccb_XPos = vert->x << 3;
        src->ccb_YPos = vert->y << 3;

        vert++;
        src++;
    }
}

void draw(void)
{
    DrawScreenCels(context.sc_ScreenItems[context.sc_CurrentScreen], ccb);
}

void draw_str(sint32 x, sint32 y, const char *str)
{
    GrafCon con;
    con.gc_PenX = x;
    con.gc_PenY = y;
    DrawText8(&con, context.sc_BitmapItems[context.sc_CurrentScreen], (uint8*)str);
}

void draw_int(sint32 x, sint32 y, uint32 value)
{
    char buf[32];
    sprintf(buf, "%lu", value);
    draw_str(x, y, buf);
}

uint32 getTimeU(void)
{
    uint32 s, u;
    GetUSecTime(irqTimer, &s, &u);
    return s * 1000000 + u;
}

int main(int argc, char *argv[])
{
    printf("MADAM Benchmark\n");

    FindAndOpenFolio("super");

    OpenGraphicsFolio();
    InitEventUtility(1, 0, LC_Observer);
    CreateBasicDisplay(&context, DI_TYPE_DEFAULT, 2);

    irqVBL = GetVBLIOReq();
    irqVRAM = GetVRAMIOReq();
    irqTimer = GetTimerIOReq();

    fps_time = GetMSecTime(irqTimer) + 1000;

    init();

    while (1)
    {
        const TEST_MODE *mode = modes + mode_index;

        update();

        time_trans = getTimeU();
        mode->proj(cube_out, mode->src, mode->matrix, SIZE * SIZE * SIZE);
        time_trans = getTimeU() - time_trans;

        time_prep = getTimeU();
        prep();
        time_prep = getTimeU() - time_prep;

        time_draw = getTimeU();
        draw();
        time_draw = getTimeU() - time_draw;

        draw_str(8, 8, modes[mode_index].name);
        draw_int(8, 16, fps);
        draw_int(8, 24, time_trans);
        draw_int(8, 32, time_prep);
        draw_int(8, 40, time_draw);

        // blit
        DisplayScreen(context.sc_ScreenItems[context.sc_CurrentScreen], NULL);
        context.sc_CurrentScreen ^= 1;

        WaitVBL(irqVBL, 1);

        { // fps meter
            sint32 time = GetMSecTime(irqTimer);
            fps_frame++;
            if (time >= fps_time)
            {
                fps = fps_frame;
                fps_frame = 0;
                fps_time += 1000;
            }
        }

        // clear
        clearInfo.ioi_Recv.iob_Buffer = context.sc_Bitmaps[context.sc_CurrentScreen]->bm_Buffer;
        SendIO(irqVRAM, &clearInfo);
    }

    return 0;
}
