/*
 * Copyright (c) 2006 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

/*
 * File Contents: This file is consists of main Xfree
 *                acceleration supported routines like solid fill used
 *                here.
 * Project:       Geode Xfree Frame buffer device driver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Xfree86 header files */

#include "vgaHW.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xaalocal.h"
#include "xf86fbman.h"
#include "miline.h"
#include "xf86_libc.h"
#include "xaarop.h"
#include "amd.h"

typedef unsigned char uchar;

#if DEBUGLVL>0
extern FILE *zdfp;

#if DEBUGTIM>0
#ifndef USE_RDTSC
#define DBLOG(n,s...) do { if((DEBUGLVL)>=(n)) { long secs,usecs; \
		getsecs(&secs,&usecs); fprintf(zdfp,"%d,%d ",secs,usecs); \
		fprintf(zdfp,s); } } while(0)
#else
#define tsc(n) __asm__ __volatile__ ( \
		" rdtsc" \
		: "=a" (((int*)(&(n)))[0]), "=d" (((int*)(&(n)))[1]) \
		: )
#define DBLOG(n,s...) do { if((DEBUGLVL)>=(n)) { long long t; \
		tsc(t); fprintf(zdfp,"%lld ",t); \
		fprintf(zdfp,s); } } while(0)
#endif
#else
#define DBLOG(n,s...) do { if((DEBUGLVL)>=(n)) fprintf(zdfp,s); } while(0)
#endif
#else
#define DBLOG(n,s...) do {} while(0)
#endif

#define CALC_FBOFFSET(x, y) (((ulong)(y) << gu3_yshift) | \
							((ulong)(x) << gu3_xshift))

#define OS_UDELAY 0
#if OS_UDELAY > 0
#define OS_USLEEP(usec) usleep(usec);
#else
#define OS_USLEEP(usec)
#endif

#define HOOK(fn) localRecPtr->fn = LX##fn

static int lx0 = -1, ly0 = -1;
static int lx1 = -1, ly1 = -1;
static int ROP;

/* #define ENABLE_PREFETCH CIMGP_ENABLE_PREFETCH */
#define ENABLE_PREFETCH 0
#define BLTFLAGS_HAZARD CIMGP_BLTFLAGS_HAZARD

/* specify dst bounding box, upper left/lower right */
static int
lx_flags0(int x0, int y0, int x1, int y1)
{
    int n = ((ROP ^ (ROP >> 1)) & 0x55) == 0 || /* no dst */
        x0 >= lx1 || y0 >= ly1 ||      /* rght/below */
        x1 <= lx0 || y1 <= ly0 ?       /* left/above */
        ENABLE_PREFETCH : BLTFLAGS_HAZARD;

    lx0 = x0;
    ly0 = y0;
    lx1 = x1;
    ly1 = y1;
    return n;
}

/* specify dst bounding box, upper left/WxH */
static int
lx_flags1(int x0, int y0, int w, int h)
{
    return lx_flags0(x0, y0, x0 + w, y0 + h);
}

/* specify dst bounding box, two points */
static int
lx_flags2(int x0, int y0, int x1, int y1)
{
    int n;

    if (x0 >= x1) {
        n = x0;
        x0 = x1 - 1;
        x1 = n + 1;
    }
    if (y0 >= y1) {
        n = y0;
        y0 = y1 - 1;
        y1 = n + 1;
    }

    return lx_flags0(x0, y0, x1, y1);
}

/* specify src/dst bounding box, upper left/WxH */
static int
lx_flags3(int x0, int y0, int x1, int y1, int w, int h)
{
    int x2 = x1 + w, y2 = y1 + h;

    /* dst not hazzard and src not hazzard */
    int n = (((ROP ^ (ROP >> 1)) & 0x55) == 0 ||
        x1 >= lx1 || y1 >= ly1 ||
        x2 <= lx0 || y2 <= ly0) &&
        (((ROP ^ (ROP >> 2)) & 0x33) == 0 ||
        x0 >= lx1 || y0 >= ly1 || x0 + w <= lx0 || y0 + h <= ly0)
        ? ENABLE_PREFETCH : BLTFLAGS_HAZARD;

    lx0 = x1;
    ly0 = y1;
    lx1 = x2;
    ly1 = y2;

    return n;
}

static void
lx_endpt(int x, int y, int len, int mag, int min, int err, int oct, int *px,
    int *py)
{
    int u = len - 1;
    int v = (u * min - err) / mag;

    switch (oct) {
    default:
    case 0:
        x += u;
        y += v;
        break;
    case 1:
        x += v;
        y += u;
        break;
    case 2:
        x += u;
        y -= v;
        break;
    case 3:
        x += v;
        y -= u;
        break;
    case 4:
        x -= u;
        y += v;
        break;
    case 5:
        x -= v;
        y += u;
        break;
    case 6:
        x -= u;
        y -= v;
        break;
    case 7:
        x -= v;
        y -= u;
        break;
    }

    *px = x;
    *py = y;
}

/* static storage declarations */

typedef struct sGBltBox
{
    ulong x, y;
    ulong w, h;
    ulong color;
    int bpp, transparent;
} GBltBox;

static GBltBox giwr;
static GBltBox gc2s;

typedef struct sGDashLine
{
    ulong pat;
    int len;
    int fg;
    int bg;
} GDashLine;

static GDashLine gdln;

static uint gu3_xshift = 1;
static uint gu3_yshift = 1;
static uint gu3_img_fmt = 0;

#if !LX_USE_OFFSCRN_MEM
static uint ImgBufOffset;
#else
static uchar *ImgLnsBuffers;
#endif
static uchar *ClrLnsBuffers;
static XAAInfoRecPtr localRecPtr;

static int lx_src_fmt[4] = {
    CIMGP_SOURCE_FMT_8BPP_INDEXED,
    CIMGP_SOURCE_FMT_0_5_6_5,
    CIMGP_SOURCE_FMT_24BPP,
    CIMGP_SOURCE_FMT_8_8_8_8
};

/* pat  0xF0 */
/* src  0xCC */
/* dst  0xAA */

/* (src FUNC dst) */

static const int SDfn[16] = {
    0x00, 0x88, 0x44, 0xCC, 0x22, 0xAA, 0x66, 0xEE,
    0x11, 0x99, 0x55, 0xDD, 0x33, 0xBB, 0x77, 0xFF
};

/* ((src FUNC dst) AND pat-mask) OR (dst AND (NOT pat-mask)) */

static const int SDfn_PM[16] = {
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA
};

/* (pat FUNC dst) */

static int PDfn[16] = {
    0x00, 0xA0, 0x50, 0xF0, 0x0A, 0xAA, 0x5A, 0xFA,
    0x05, 0xA5, 0x55, 0xF5, 0x0F, 0xAF, 0x5F, 0xFF
};

/* ((pat FUNC dst) AND src-mask) OR (dst AND (NOT src-mask)) */

static int PDfn_SM[16] = {
    0x22, 0xA2, 0x62, 0xE2, 0x2A, 0xAA, 0x6A, 0xEA,
    0x26, 0xA6, 0x66, 0xE6, 0x2E, 0xAE, 0x6E, 0xEE
};

/*----------------------------------------------------------------------------
 * LXAccelSync.
 *
 * Description  :This function is called to synchronize with the graphics
 *               engine and it waits until the graphic engine is idle.
 *				 This is required before allowing direct access to the
 *				 framebuffer.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *
 * Returns              :none
 *---------------------------------------------------------------------------*/
void
LXAccelSync(ScrnInfoPtr pScrni)
{
    DBLOG(3, "LXAccelSync()\n");
    while (gp_test_blt_busy() != 0) {
        OS_USLEEP(OS_UDELAY)
    }
}

#if LX_FILL_RECT_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForSolidFill.
 *
 * Description  :The SetupFor and Subsequent SolidFill(Rect) provide
 *               filling rectangular areas of the screen with a
 *               foreground color.
 *
 * Parameters.
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *   color        int     foreground fill color
 *    rop         int     unmapped raster op
 * planemask     uint     -1 (fill) or pattern data
 *
 * Returns              :none
 *--------------------------------------------------------------------------*/
static void
LXSetupForSolidFill(ScrnInfoPtr pScrni, int color, int rop, uint planemask)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2, "LXSetupForSolidFill(%#x,%#x,%#x)\n", color, rop, planemask);
    rop &= 0x0F;
    gp_declare_blt(0);
    if (planemask == ~0U) {            /* fill with color */
        gp_set_raster_operation(ROP = SDfn[rop]);
    } else {                           /* select rop that uses source data for planemask */
        gp_set_raster_operation(ROP = SDfn_PM[rop]);
        gp_set_solid_pattern(planemask);
    }
    gp_set_solid_source(color);
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
}

 /*----------------------------------------------------------------------------
 * LXSubsequentSolidFillRect.
 *
 * Description  :see LXSetupForSolidFill.
 *
 * Parameters.
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x          int     destination x offset
 *     y          int     destination y offset
 *     w          int     fill area width (pixels)
 *     h          int     fill area height (pixels)
 *
 * Returns      :none
 *
 * Sample application uses:
 *   - Window backgrounds. 
 *   - pull down highlighting.
 *   - x11perf: rectangle tests (-rect500).
 *   - x11perf: fill trapezoid tests (-trap100).
 *   - x11perf: horizontal line segments (-hseg500).
 *---------------------------------------------------------------------------*/
static void
LXSubsequentSolidFillRect(ScrnInfoPtr pScrni, int x, int y, int w, int h)
{
    int flags;

    DBLOG(2, "LXSubsequentSolidFillRect() at %d,%d %dx%d\n", x, y, w, h);
    flags = lx_flags1(x, y, w, h);
    gp_declare_blt(flags);
    gp_pattern_fill(CALC_FBOFFSET(x, y), w, h);
}

/* LX_FILL_RECT_SUPPORT */
#endif

#if LX_CLREXP_8X8_PAT_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForColor8x8PatternFill
 *
 * Description  :8x8 color pattern data is 64 pixels of full color data
 *               stored linearly in offscreen video memory.  These patterns
 *               are useful as a substitute for 8x8 mono patterns when tiling,
 *               doing opaque stipples, or regular stipples. 
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *    patx        int     x offset to pattern data
 *    paty        int     y offset to pattern data
 *    rop         int     unmapped raster operation
 * planemask     uint     -1 (copy) or pattern data
 * trans_color    int     -1 (copy) or transparent color (not enabled)
 *                         trans color only supported on source channel
 *                         or in monochrome pattern channel
 * 
 * Returns      :none.
 *
 *---------------------------------------------------------------------------*/

static void
LXSetupForColor8x8PatternFill(ScrnInfoPtr pScrni, int patx, int paty, int rop,
    uint planemask, int trans_color)
{
    unsigned long *pat_8x8;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2, "LXSetupForColor8x8PatternFill() pat %#x,%#x rop %#x %#x %#x\n",
        patx, paty, rop, planemask, trans_color);
    pat_8x8 = (unsigned long *)(pGeode->FBBase + CALC_FBOFFSET(patx, paty));
    /* since the cache may be loaded by blt, we must wait here */
    LXAccelSync(pScrni);
    gp_set_color_pattern(pat_8x8, gu3_img_fmt, 0, 0);
    rop &= 0x0F;
    gp_declare_blt(0);
    if (planemask == ~0U) {            /* fill with pattern */
        gp_set_raster_operation(ROP = PDfn[rop]);
    } else {                           /* select rop that uses source data for planemask */
        gp_set_raster_operation(ROP = PDfn_SM[rop]);
        gp_set_solid_source((ulong) planemask);
    }
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
}

/*----------------------------------------------------------------------------
 * LXSubsequentColor8x8PatternFillRect
 *
 * Description  :see LXSetupForColor8x8PatternFill.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *   patx         int     pattern phase x offset
 *   paty         int     pattern phase y offset
 *      x         int     destination x offset
 *      y         int     destination y offset
 *      w         int     fill area width (pixels)
 *      h         int     fill area height (pixels)
 *              
 * Returns      :none
 *
 * Sample application uses:
 *   - Patterned desktops
 *   - x11perf: stippled rectangle tests (-srect500).
 *   - x11perf: opaque stippled rectangle tests (-osrect500).
 *--------------------------------------------------------------------------*/
static void
LXSubsequentColor8x8PatternFillRect(ScrnInfoPtr pScrni, int patx, int paty,
    int x, int y, int w, int h)
{
    int flags;

    DBLOG(2,
        "LXSubsequentColor8x8PatternFillRect() patxy %d,%d at %d,%d %dsx%d\n",
        patx, paty, x, y, w, h);
    flags = lx_flags1(x, y, w, h);
    gp_declare_blt(flags);
    gp_set_pattern_origin(patx, paty);
    gp_pattern_fill(CALC_FBOFFSET(x, y), w, h);
}

/* LX_CLREXP_8X8_PAT_SUPPORT */
#endif

#if LX_MONO_8X8_PAT_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForMono8x8PatternFill
 *
 * Description  :8x8 mono pattern data is 64 bits of color expansion data
 *               with ones indicating the foreground color and zeros
 *               indicating the background color.  These patterns are
 *               useful when tiling, doing opaque stipples, or regular
 *               stipples. 
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *    patx        int     x offset to pattern data
 *    paty        int     y offset to pattern data
 *     fg         int     foreground color
 *     bg         int     -1 (transparent) or background color
 *    rop         int     unmapped raster operation
 * planemask     uint     -1 (copy) or pattern data
 * 
 * Returns      :none.
 *
 * Comments     :none.
 *
 *--------------------------------------------------------------------------*/
static void
LXSetupForMono8x8PatternFill(ScrnInfoPtr pScrni, int patx, int paty,
    int fg, int bg, int rop, uint planemask)
{
    int trans;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2,
        "LXSetupForMono8x8PatternFill() pat %#x,%#x fg %#x bg %#x %#x %#x\n",
        patx, paty, fg, bg, rop, planemask);
    rop &= 0x0F;
    gp_declare_blt(0);
    if (planemask == ~0U) {            /* fill with pattern */
        gp_set_raster_operation(ROP = PDfn[rop]);
    } else {                           /* select rop that uses source data for planemask */
        gp_set_raster_operation(ROP = PDfn_SM[rop]);
        gp_set_solid_source((ulong) planemask);
    }
    trans = bg == -1 ? 1 : 0;
    gp_set_mono_pattern(bg, fg, patx, paty, trans, 0, 0);
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
}

/*----------------------------------------------------------------------------
 * LXSubsequentMono8x8PatternFillRect
 *
 * Description  :see LXSetupForMono8x8PatternFill
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *   patx         int     pattern phase x offset
 *   paty         int     pattern phase y offset
 *      x         int     destination x offset
 *      y         int     destination y offset
 *      w         int     fill area width (pixels)
 *      h         int     fill area height (pixels)

 * Returns      :none
 *
 * Sample application uses:
 *   - Patterned desktops
 *   - x11perf: stippled rectangle tests (-srect500).
 *   - x11perf: opaque stippled rectangle tests (-osrect500).
 *--------------------------------------------------------------------------*/
static void
LXSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrni, int patx, int paty,
    int x, int y, int w, int h)
{
    int flags;

    DBLOG(2,
        "LXSubsequentMono8x8PatternFillRect() pat %#x,%#x at %d,%d %dx%d\n",
        patx, paty, x, y, w, h);
    flags = lx_flags1(x, y, w, h);
    gp_declare_blt(flags);
    gp_set_pattern_origin(patx, paty);
    gp_pattern_fill(CALC_FBOFFSET(x, y), w, h);
}

/* LX_MONO_8X8_PAT_SUPPORT */
#endif

#if LX_SCR2SCRCPY_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForScreenToScreenCopy
 *
 * Description  :SetupFor and Subsequent ScreenToScreenCopy functions
 *               provide an interface for copying rectangular areas from
 *               video memory to video memory.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *   xdir         int     x copy direction (up/dn)
 *   ydir         int     y copy direction (up/dn)
 *    rop         int     unmapped raster operation
 * planemask     uint     -1 (copy) or pattern data
 * trans_color    int     -1 (copy) or transparent color
 *
 * Returns      :none
 *---------------------------------------------------------------------------*/
static void
LXSetupForScreenToScreenCopy(ScrnInfoPtr pScrni, int xdir, int ydir, int rop,
    uint planemask, int trans_color)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2, "LXSetupForScreenToScreenCopy() xd%d yd%d rop %#x %#x %#x\n",
        xdir, ydir, rop, planemask, trans_color);
    rop &= 0x0F;
    gp_declare_blt(0);
    if (planemask == ~0U) {
        gp_set_raster_operation(ROP = SDfn[rop]);
    } else {
        gp_set_raster_operation(ROP = SDfn_PM[rop]);
        gp_set_solid_pattern((ulong) planemask);
    }
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
    gc2s.transparent = (trans_color == -1) ? 0 : 1;
    gc2s.color = trans_color;
}

/*----------------------------------------------------------------------------
 * LXSubsquentScreenToScreenCopy
 *
 * Description  :see LXSetupForScreenToScreenCopy.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x1         int     source x offset
 *     y1         int     source y offset
 *     x2         int     destination x offset
 *     y2         int     destination y offset
 *      w         int     copy area width (pixels)
 *      h         int     copy area height (pixels)
 * 
 * Returns      :none
 *
 * Sample application uses (non-transparent):
 *   - Moving windows.
 *   - x11perf: scroll tests (-scroll500).
 *   - x11perf: copy from window to window (-copywinwin500).
 *---------------------------------------------------------------------------*/
static void
LXSubsequentScreenToScreenCopy(ScrnInfoPtr pScrni,
    int x1, int y1, int x2, int y2, int w, int h)
{
    int flags;

    DBLOG(2, "LXSubsequentScreenToScreenCopy() from %d,%d to %d,%d %dx%d\n",
        x1, y1, x2, y2, w, h);

    flags = lx_flags3(x1, y1, x2, y2, w, h);
    gp_declare_blt(flags);

    if (gc2s.transparent) {
        gp_set_source_transparency(gc2s.color, ~0);
    }

    flags = 0;

    if (x2 > x1)
        flags |= 1;
    if (y2 > y1)
        flags |= 2;

    gp_screen_to_screen_blt(CALC_FBOFFSET(x2, y2), CALC_FBOFFSET(x1, y1), w,
        h, flags);
}

/* LX_SCR2SCRCPY_SUPPORT */
#endif

#if LX_SCANLINE_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForScanlineImageWrite
 *
 * Description  :SetupFor/Subsequent ScanlineImageWrite and ImageWriteScanline
 *               transfer full color pixel data from system memory to video
 *               memory.  This is useful for dealing with alignment issues and
 *               performing raster ops on the data.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *    rop         int     unmapped raster operation
 * planemask     uint     -1 (copy) or pattern data
 *    bpp         int     bits per pixel (unused)
 *  depth         int     color depth (unused)
 *
 * Returns      :none
 *
 *  x11perf -putimage10  
 *  x11perf -putimage100 
 *  x11perf -putimage500 
*----------------------------------------------------------------------------
*/
static void
LXSetupForScanlineImageWrite(ScrnInfoPtr pScrni, int rop, uint planemask,
    int trans_color, int bpp, int depth)
{
    int Bpp = (bpp + 7) >> 3;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2, "LXSetupForScanlineImageWrite() rop %#x %#x %#x %d %d\n",
        rop, planemask, trans_color, bpp, depth);
    rop &= 0x0F;
    gp_set_source_format(lx_src_fmt[Bpp - 1]);
    gp_declare_blt(0);
    if (planemask == ~0U) {
        gp_set_raster_operation(ROP = SDfn[rop]);
    } else {
        gp_set_raster_operation(ROP = SDfn_PM[rop]);
        gp_set_solid_pattern(planemask);
    }
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
    giwr.transparent = (trans_color == -1) ? 0 : 1;
    giwr.color = trans_color;
}

/*----------------------------------------------------------------------------
 * LXSubsequentScanlineImageWriteRect
 *
 * Description  : see LXSetupForScanlineImageWrite.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *      x         int     destination x offset
 *      y         int     destination y offset
 *      w         int     copy area width (pixels)
 *      h         int     copy area height (pixels)
 * skipleft       int     x margin (pixels) to skip (not enabled)
 *      
 * Returns      :none
 *---------------------------------------------------------------------------*/
static void
LXSubsequentScanlineImageWriteRect(ScrnInfoPtr pScrni,
    int x, int y, int w, int h, int skipleft)
{
    DBLOG(2, "LXSubsequentScanlineImageWriteRect() rop %d,%d %dx%d %d\n", x,
        y, w, h, skipleft);
    giwr.x = x;
    giwr.y = y;
    giwr.w = w;
    giwr.h = h;
    /* since the image buffer must be not busy (it may be busy from
     * a previous ScanlineWriteImage), we must add a Sync here */
#if !LX_USE_OFFSCRN_MEM
    LXAccelSync(pScrni);
#endif
}

/*----------------------------------------------------------------------------
 * LXSubsquentImageWriteScanline
 *
 * Description  : see LXSetupForScanlineImageWrite.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *  bufno         int     scanline number in write group
 *
 * Returns      :none
 *
 * Sample application uses (non-transparent):
 *   - Moving windows.
 *   - x11perf: scroll tests (-scroll500).
 *   - x11perf: copy from window to window (-copywinwin500).
 *
 *---------------------------------------------------------------------------*/
static void
LXSubsequentImageWriteScanline(ScrnInfoPtr pScrni, int bufno)
{
    GeodeRec *pGeode;
    int blt_height = 0;

    DBLOG(3, "LXSubsequentImageWriteScanline() %d\n", bufno);
    pGeode = GEODEPTR(pScrni);

    if ((blt_height = pGeode->NoOfImgBuffers) > giwr.h)
        blt_height = giwr.h;
    if (++bufno < blt_height)
        return;

    gp_declare_blt(ENABLE_PREFETCH);
    if (giwr.transparent) {
        gp_set_source_transparency(giwr.color, ~0);
    }
#if !LX_USE_OFFSCRN_MEM
    gp_screen_to_screen_blt(CALC_FBOFFSET(giwr.x, giwr.y), ImgBufOffset,
        giwr.w, blt_height, 0);
    LXAccelSync(pScrni);
#else
    gp_color_bitmap_to_screen_blt(CALC_FBOFFSET(giwr.x, giwr.y), 0,
        giwr.w, blt_height, ImgLnsBuffers, pGeode->AccelPitch);
#endif
    giwr.h -= blt_height;
    giwr.y += blt_height;
}

/* LX_SCANLINE_SUPPORT */
#endif

#if LX_CPU2SCREXP_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForScanlineCPUToScreenColorExpandFill
 *
 * Description  :SetupFor/Subsequent CPUToScreenColorExpandFill and
 *               ColorExpandScanline routines provide an interface for
 *               doing expansion blits from source patterns stored in
 *               system memory.
 *                              
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     fg         int     foreground color
 *     bg         int     -1 (transparent) or background color
 *    rop         int     unmapped raster operation
 * planemask     uint     -1 (copy) or pattern data
 *
 * Returns      :none.
 *---------------------------------------------------------------------------*/

static void
LXSetupForScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrni,
    int fg, int bg, int rop, uint planemask)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2,
        "LXSetupForScanlineCPUToScreenColorExpandFill() "
        "fg %#x bg %#x rop %#x %#x\n", fg, bg, rop, planemask);
    rop &= 0x0F;
    gp_declare_blt(0);
    if (planemask == ~0U) {
        gp_set_raster_operation(ROP = SDfn[rop]);
    } else {
        gp_set_raster_operation(ROP = SDfn_PM[rop]);
        gp_set_solid_pattern(planemask);
    }
    gp_set_mono_source(bg, fg, (bg == -1));
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
    gc2s.bpp = 1;
    gc2s.transparent = 0;
    gc2s.color = 0;
}

/*----------------------------------------------------------------------------
 * LXSubsequentScanlineCPUToScreenColorExpandFill
 *
 * Description  :see LXSetupForScanlineCPUToScreenColorExpandFill
 *
 * Parameters:
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x          int     destination x offset
 *     y          int     destination y offset
 *     w          int     fill area width (pixels)
 *     h          int     fill area height (pixels)
 *      
 * Returns      :none
 *
 *---------------------------------------------------------------------------*/
static void
LXSubsequentScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrni,
    int x, int y, int w, int h, int skipleft)
{
    DBLOG(2,
        "LXSubsequentScanlineCPUToScreenColorExpandFill() %d,%d %dx%d %d\n",
        x, y, w, h, skipleft);
    gc2s.x = x;
    gc2s.y = y;
    gc2s.w = w;
    gc2s.h = h;
}

/*----------------------------------------------------------------------------
 * LXSubsequentColorExpandScanline
 *
 * Description  :see LXSetupForScanlineCPUToScreenColorExpandFill
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *  bufno         int     scanline number in write group
 *
 * Returns      :none
*----------------------------------------------------------------------------
*/
static void
LXSubsequentColorExpandScanline(ScrnInfoPtr pScrni, int bufno)
{
    GeodeRec *pGeode;
    ulong srcpitch;
    int blt_height = 0;

    DBLOG(3, "LXSubsequentColorExpandScanline() %d\n", bufno);
    pGeode = GEODEPTR(pScrni);

    if ((blt_height = pGeode->NoOfImgBuffers) > gc2s.h)
        blt_height = gc2s.h;
    if (++bufno < blt_height)
        return;

    gp_declare_blt(ENABLE_PREFETCH);
    /* convert from bits to dwords */
    srcpitch = ((pGeode->AccelPitch + 31) >> 5) << 2;
    gp_mono_bitmap_to_screen_blt(CALC_FBOFFSET(gc2s.x, gc2s.y), 0, gc2s.w,
        blt_height, ClrLnsBuffers, srcpitch);
    gc2s.h -= blt_height;
    gc2s.y += blt_height;
}

/* LX_CPU2SCREXP_SUPPORT */
#endif

#if LX_SCR2SCREXP_SUPPORT

/*----------------------------------------------------------------------------
 * LXSetupForScreenToScreenColorExpandFill
 *
 * Description  :SetupFor/Subsequent ScreenToScreenColorExpandFill and
 *               ColorExpandScanline routines provide an interface for
 *               doing expansion blits from source patterns stored in
 *               video memory.
 *                              
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     fg         int     foreground color
 *     bg         int     -1 (transparent) or background color
 *    rop         int     unmapped raster operation
 * planemask     uint     -1 (copy) or pattern data
 *
 * Returns      :none.
 *---------------------------------------------------------------------------*/

static void
LXSetupForScreenToScreenColorExpandFill(ScrnInfoPtr pScrni, int fg, int bg,
    int rop, uint planemask)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2,
        "LXSetupForScreenToScreenColorExpandFill() "
        "fg %#x bg %#x rop %#x %#x\n", fg, bg, rop, planemask);
    rop &= 0x0F;
    gp_declare_blt(0);
    if (planemask == ~0U) {
        gp_set_raster_operation(ROP = SDfn[rop]);
    } else {
        gp_set_raster_operation(ROP = SDfn_PM[rop]);
        gp_set_solid_pattern(planemask);
    }
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_set_mono_source(bg, fg, (bg == -1));
    gp_write_parameters();
}

/*----------------------------------------------------------------------------
 * LXSubsequentScreenToScreenColorExpandFill
 *
 * Description  :see LXSetupForScreenToScreenColorExpandFill
 *
 * Parameters:
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x          int     destination x offset
 *     y          int     destination y offset
 *     w          int     fill area width (pixels)
 *     h          int     fill area height (pixels)
 * offset         int     initial x offset
 *      
 * Returns      :none
 *
 *---------------------------------------------------------------------------*/
static void
LXSubsequentScreenToScreenColorExpandFill(ScrnInfoPtr pScrni,
    int x, int y, int w, int h, int srcx, int srcy, int offset)
{
    int flags;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2,
        "LXSubsequentScreenToScreenColorExpandFill() %d,%d %dx%d %d,%d %d\n",
        x, y, w, h, srcx, srcy, offset);
    flags = lx_flags3(srcx, srcy, x, y, w, h);
    gp_declare_blt(flags);
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_mono_expand_blt(CALC_FBOFFSET(x, y), CALC_FBOFFSET(srcx, srcy), offset,
        w, h, 0);
}

/* LX_SCR2SCREXP_SUPPORT */
#endif

#define VM_X_MAJOR 0
#define VM_Y_MAJOR 1
#define VM_MAJOR_INC 2
#define VM_MAJOR_DEC 0
#define VM_MINOR_INC 4
#define VM_MINOR_DEC 0

static ushort vmode[] = {
    VM_X_MAJOR | VM_MAJOR_INC | VM_MINOR_INC,   /* !XDECR !YDECR !YMAJOR */
    VM_Y_MAJOR | VM_MAJOR_INC | VM_MINOR_INC,   /* !XDECR !YDECR  YMAJOR */
    VM_X_MAJOR | VM_MAJOR_INC | VM_MINOR_DEC,   /* !XDECR  YDECR !YMAJOR */
    VM_Y_MAJOR | VM_MAJOR_DEC | VM_MINOR_INC,   /* !XDECR  YDECR  YMAJOR */
    VM_X_MAJOR | VM_MAJOR_DEC | VM_MINOR_INC,   /*  XDECR !YDECR !YMAJOR */
    VM_Y_MAJOR | VM_MAJOR_INC | VM_MINOR_DEC,   /*  XDECR !YDECR  YMAJOR */
    VM_X_MAJOR | VM_MAJOR_DEC | VM_MINOR_DEC,   /*  XDECR  YDECR !YMAJOR */
    VM_Y_MAJOR | VM_MAJOR_DEC | VM_MINOR_DEC,   /*  XDECR  YDECR  YMAJOR */
};

#define ABS(_val1, _val2)	(((_val1) > (_val2)) ? \
							((_val1)-(_val2)) : ((_val2) - (_val1)))

#if LX_BRES_LINE_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForSolidLine
 *
 * Description  :SetupForSolidLine and Subsequent HorVertLine TwoPointLine
 *               BresenhamLine provides an interface for drawing thin
 *               solid lines.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *   color        int     foreground fill color
 *    rop         int     unmapped raster op
 * planemask     uint     -1 (fill) or pattern data (not enabled)
 * 
 * Returns              :none
*---------------------------------------------------------------------------*/
static void
LXSetupForSolidLine(ScrnInfoPtr pScrni, int color, int rop, uint planemask)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2, "LXSetupForSolidLine() %#x %#x %#x\n", color, rop, planemask);
    rop &= 0x0F;
    gp_declare_vector(CIMGP_BLTFLAGS_HAZARD);
    if (planemask == ~0U) {
        gp_set_raster_operation(ROP = PDfn[rop]);
    } else {
        gp_set_raster_operation(ROP = PDfn_SM[rop]);
        gp_set_solid_source(planemask);
    }
    gp_set_solid_pattern((ulong) color);
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
}

/*---------------------------------------------------------------------------
 * LXSubsequentSolidBresenhamLine
 *
 * Description  :see LXSetupForSolidLine
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x1         int     destination x offset
 *     y1         int     destination y offset
 * absmaj         int     Bresenman absolute major
 * absmin         int     Bresenman absolute minor
 *    err         int     Bresenman initial error term
 *    len         int     length of the vector (pixels)
 * octant         int     specifies sign and magnitude relationships
 *                         used to determine axis of magor rendering
 *                         and direction of vector progress.
 *
 * Returns      :none
 *
 *   - Window outlines on window move.
 *   - x11perf: line segments (-line500).
 *   - x11perf: line segments (-seg500).
 *---------------------------------------------------------------------------*/
static void
LXSubsequentSolidBresenhamLine(ScrnInfoPtr pScrni, int x1, int y1,
    int absmaj, int absmin, int err, int len, int octant)
{
    int x2, y2, flags;
    long axial, diagn;

    DBLOG(2, "LXSubsequentSolidBresenhamLine() %d,%d %d %d, %d %d, %d\n",
        x1, y1, absmaj, absmin, err, len, octant);
    if (len <= 0)
        return;
    lx_endpt(x1, y1, len, absmaj, absmin, err, octant, &x2, &y2);
    flags = lx_flags2(x1, y1, x2 + 1, y2 + 1);
    gp_declare_vector(flags);
    axial = absmin;
    err += axial;
    diagn = absmin - absmaj;
    gp_bresenham_line(CALC_FBOFFSET(x1, y1), len, err, axial, diagn,
        vmode[octant]);
}

/*---------------------------------------------------------------------------
 * LXSubsequentSolidTwoPointLine
 *
 * Description  :see LXSetupForSolidLine
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x0         int     destination x start offset
 *     y0         int     destination y start offset
 *     x1         int     destination x end offset
 *     y1         int     destination y end offset
 *  flags         int     OMIT_LAST, dont draw last pixel (not used)
 *
 * Returns      :none
 *---------------------------------------------------------------------------*/
static void
LXSubsequentSolidTwoPointLine(ScrnInfoPtr pScrni, int x0, int y0,
    int x1, int y1, int flags)
{
    long dx, dy, dmaj, dmin, octant, bias;
    long axial, diagn, err, len;

    DBLOG(2, "LXSubsequentSolidTwoPointLine() %d,%d %d,%d, %#x\n",
        x0, y0, x1, y1, flags);

    if ((dx = x1 - x0) < 0)
        dx = -dx;
    if ((dy = y1 - y0) < 0)
        dy = -dy;

    if (dy >= dx) {
        dmaj = dy;
        dmin = dx;
        octant = YMAJOR;
    } else {
        dmaj = dx;
        dmin = dy;
        octant = 0;
    }
    len = dmaj;
    if ((flags & OMIT_LAST) == 0)
        ++len;
    if (len <= 0)
        return;
    if (x1 < x0)
        octant |= XDECREASING;
    if (y1 < y0)
        octant |= YDECREASING;

    flags = lx_flags2(x0, y0, x1 + 1, y1 + 1);
    gp_declare_vector(flags);
    axial = dmin << 1;
    bias = miGetZeroLineBias(pScrni->pScreen);
    err = axial - dmaj - ((bias >> octant) & 1);
    diagn = (dmin - dmaj) << 1;
    gp_bresenham_line(CALC_FBOFFSET(x0, y0), len, err, axial, diagn,
        vmode[octant]);
}

/*---------------------------------------------------------------------------
 * LXSubsequentSolidHorVertLine
 *
 * Description  :see LXSetupForSolidLine
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x          int     destination x offset
 *     y          int     destination y offset
 *    len         int     length of the vector (pixels)
 *    dir         int     DEGREES_270 or DEGREES_0 line direction
 *
 * Sample application uses:
 *   - Window outlines on window move.
 *   - x11perf: line segments (-hseg500).
 *   - x11perf: line segments (-vseg500).
 *---------------------------------------------------------------------------
 */
static void
LXSubsequentSolidHorVertLine(ScrnInfoPtr pScrni,
    int x, int y, int len, int dir)
{
    int flags, w, h;

    DBLOG(2, "LXSubsequentHorVertLine() %d,%d %d %d\n", x, y, len, dir);
    gp_declare_blt(0);
    if (dir == DEGREES_0) {
        w = len;
        h = 1;
    } else {
        w = 1;
        h = len;
    }
    flags = lx_flags1(x, y, w, h);
    gp_declare_blt(flags);
    gp_pattern_fill(CALC_FBOFFSET(x, y), ((dir == DEGREES_0) ? len : 1),
        ((dir == DEGREES_0) ? 1 : len));
}

/* LX_BRES_LINE_SUPPORT */
#endif

#if LX_DASH_LINE_SUPPORT
/*----------------------------------------------------------------------------
 * LXSetupForDashedLine
 *
 * Description  :SetupForDashedLine and Subsequent TwoPointLine
 *               BresenhamLine provides an interface for drawing thin
 *               dashed lines.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     fg         int     foreground fill color
 *     bg         int     -1 (transp) or background fill color
 *    rop         int     unmapped raster op
 * planemask     uint     -1 (fill) or pattern data (not enabled)
 *  length        int     pattern length (bits)
 * pattern     uchar*     dash pattern mask
 * 
 * Returns              :none
*---------------------------------------------------------------------------*/
static void
LXSetupForDashedLine(ScrnInfoPtr pScrni,
    int fg, int bg, int rop, uint planemask, int length, uchar * pattern)
{
    int n;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2,
        "LXSetupForDashedLine() "
        "fg %#x bg %#x rop %#x pm %#x len %d, pat %#x\n",
        fg, bg, rop, planemask, length, *(ulong *) pattern);
    gdln.fg = fg;
    gdln.bg = bg;
    gdln.len = length;
    n = (length + 7) / 8;
    if (n > sizeof(gdln.pat))
        n = sizeof(gdln.pat);
    memcpy(&gdln.pat, pattern, n);
    rop &= 0x0F;
    gp_declare_vector(CIMGP_BLTFLAGS_HAZARD);
    if (planemask == ~0U) {
        gp_set_raster_operation(ROP = PDfn[rop]);
    } else {
        gp_set_raster_operation(ROP = PDfn_SM[rop]);
        gp_set_solid_source(planemask);
    }
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    gp_write_parameters();
}

/*---------------------------------------------------------------------------
 * LXSubsequentDashedBresenhamLine
 *
 * Description  :see LXSetupForDashedLine
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x1         int     destination x offset
 *     y1         int     destination y offset
 * absmaj         int     Bresenman absolute major
 * absmin         int     Bresenman absolute minor
 *    err         int     Bresenman initial error term
 *    len         int     length of the vector (pixels)
 * octant         int     specifies sign and magnitude relationships
 *                         used to determine axis of magor rendering
 *                         and direction of vector progress.
 *  phase         int     initial pattern offset at x1,y1
 *
 * Returns      :none
 *---------------------------------------------------------------------------*/
static void
LXSubsequentDashedBresenhamLine(ScrnInfoPtr pScrni, int x1, int y1,
    int absmaj, int absmin, int err, int len, int octant, int phase)
{
    int x2, y2, flags;
    long axial, diagn;
    ulong pattern;

    DBLOG(2, "LXSubsequentDashedBresenhamLine() %d,%d %d %d, %d %d, %d %d\n",
        x1, y1, absmaj, absmin, err, len, octant, phase);
    if (len <= 0)
        return;
    pattern = gdln.pat;
    if (phase > 0) {
        int n = gdln.len - phase;

        pattern = ((pattern >> phase) & ((1UL << n) - 1)) | (pattern << n);
    }
    gp_set_vector_pattern(pattern, gdln.fg, gdln.len);
    lx_endpt(x1, y1, len, absmaj, absmin, err, octant, &x2, &y2);
    flags = lx_flags2(x1, y1, x2 + 1, y2 + 1);
    gp_declare_vector(flags);
    axial = absmin;
    err += axial;
    diagn = absmin - absmaj;
    gp_bresenham_line(CALC_FBOFFSET(x1, y1), len, err, axial, diagn,
        vmode[octant]);
}

/*---------------------------------------------------------------------------
 * LXSubsequentDashedTwoPointLine
 *
 * Description  :see LXSetupForDashedLine
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *     x0         int     destination x start offset
 *     y0         int     destination y start offset
 *     x1         int     destination x end offset
 *     y1         int     destination y end offset
 *  flags         int     OMIT_LAST, dont draw last pixel (not used)
 *  phase         int     initial pattern offset at x1,y1
 *
 * Returns      :none
 *---------------------------------------------------------------------------*/
static void
LXSubsequentDashedTwoPointLine(ScrnInfoPtr pScrni, int x0, int y0,
    int x1, int y1, int flags, int phase)
{
    ulong pattern;
    long dx, dy, dmaj, dmin, octant, bias;
    long axial, diagn, err, len;

    DBLOG(2, "LXSubsequentDashedTwoPointLine() %d,%d %d,%d, %#x %d\n",
        x0, y0, x1, y1, flags, phase);

    if ((dx = x1 - x0) < 0)
        dx = -dx;
    if ((dy = y1 - y0) < 0)
        dy = -dy;
    if (dy >= dx) {
        dmaj = dy;
        dmin = dx;
        octant = YMAJOR;
    } else {
        dmaj = dx;
        dmin = dy;
        octant = 0;
    }
    len = dmaj;
    if ((flags & OMIT_LAST) == 0)
        ++len;
    if (len <= 0)
        return;
    if (x1 < x0)
        octant |= XDECREASING;
    if (y1 < y0)
        octant |= YDECREASING;

    pattern = gdln.pat;
    if (phase > 0) {
        int n = gdln.len - phase;

        pattern = ((pattern >> phase) & ((1UL << n) - 1)) | (pattern << n);
    }
    gp_set_vector_pattern(pattern, gdln.fg, gdln.len);
    flags = lx_flags2(x0, y0, x1 + 1, y1 + 1);
    gp_declare_vector(flags);

    axial = dmin << 1;
    bias = miGetZeroLineBias(pScrni->pScreen);
    err = axial - dmaj - ((bias >> octant) & 1);
    diagn = (dmin - dmaj) << 1;
    gp_bresenham_line(CALC_FBOFFSET(x0, y0), len, err, axial, diagn,
        vmode[octant]);
}

/* LX_DASH_LINE_SUPPORT */
#endif

#if LX_WRITE_PIXMAP_SUPPORT
void
LXWritePixmap(ScrnInfoPtr pScrni, int x, int y, int w, int h,
    unsigned char *src, int srcwidth, int rop,
    unsigned int planemask, int trans, int bpp, int depth)
{
    int flags, dx, dy;
    int Bpp = (bpp + 7) >> 3;
    unsigned long offset;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(2, "LXWritePixmap() %d,%d %dx%d, s%#x sp%d %#x %#x %#x %d %d\n",
        x, y, w, h, src, srcwidth, rop, planemask, trans, bpp, depth);

    rop &= 0x0F;
    gp_set_source_format(lx_src_fmt[Bpp - 1]);
    /* must assign before lx_flags */
    ROP = planemask == ~0U ? SDfn[rop] : SDfn_PM[rop];

    if (src >= pGeode->FBBase && src < pGeode->FBBase + pGeode->FBSize) {
        offset = src - pGeode->FBBase;
        dx = (offset & ((1 << gu3_yshift) - 1)) >> Bpp;
        dy = offset >> gu3_yshift;
        flags = lx_flags3(x, y, dx, dy, w, h);
    } else
        flags = ENABLE_PREFETCH;

    gp_declare_blt(flags);
    gp_set_raster_operation(ROP);
    if (planemask != ~0U)
        gp_set_solid_pattern(planemask);
    gp_set_strides(pGeode->AccelPitch, pGeode->AccelPitch);
    if (trans != -1) {
        gp_set_source_transparency(trans, ~0);
    }

    gp_color_bitmap_to_screen_blt(CALC_FBOFFSET(x, y), 0, w, h, src,
        srcwidth);
    SET_SYNC_FLAG(pGeode->AccelInfoRec);
}
#endif

/*----------------------------------------------------------------------------
 * LXAccelInit.
 *
 * Description  :Create and initialize XAAInfoRec structure.
 *               The XAAInfoRec structure contains many fields, most of
 *               which are primitive function pointers and flags.  Each
 *               primitive will have two or more functions and a set of
 *               associated associated flags.  These functions can be
 *               classified into two principle classes, the "SetupFor"
 *               and "Subsequent" functions.  The "SetupFor" function tells
 *               the driver that the hardware should be initialized for
 *               a particular type of graphics operation.  After the
 *               "SetupFor" function, one or more calls to the "Subsequent"
 *               function will be made to indicate that an instance of the
 *               particular primitive should be rendered by the hardware.
 *
 *    Arg        Type     Comment
 *  pScrn      ScreenPtr  pointer to active Screen data
 *
 * Returns              :TRUE on success and FALSE on Failure
 *
 * Comments             :This function is called in LXScreenInit in
 *                       amd_lx_driver.c to initialize acceleration.
 *---------------------------------------------------------------------------*/
Bool
LXAccelInit(ScreenPtr pScrn)
{
    GeodeRec *pGeode;
    ScrnInfoPtr pScrni;

#if DEBUGLVL>0
    if (zdfp == NULL) {
        zdfp = fopen("/tmp/xwin.log", "w");
        setbuf(zdfp, NULL);
    }
#endif
    DBLOG(2, "LXAccelInit()\n");

    pScrni = xf86Screens[pScrn->myNum];
    pGeode = GEODEPTR(pScrni);

    switch (pScrni->bitsPerPixel) {
    case 8:
        gu3_img_fmt = CIMGP_SOURCE_FMT_3_3_2;
        break;
    case 16:
        gu3_img_fmt = CIMGP_SOURCE_FMT_0_5_6_5;
        break;
    case 32:
        gu3_img_fmt = CIMGP_SOURCE_FMT_8_8_8_8;
        break;
    }

    gu3_xshift = pScrni->bitsPerPixel >> 4;

    switch (pGeode->AccelPitch) {
    case 1024:
        gu3_yshift = 10;
        break;
    case 2048:
        gu3_yshift = 11;
        break;
    case 4096:
        gu3_yshift = 12;
        break;
    case 8192:
        gu3_yshift = 13;
        break;
    }

    /* Getting the pointer for acceleration Inforecord */
    pGeode->AccelInfoRec = localRecPtr = XAACreateInfoRec();

    /* SET ACCELERATION FLAGS */
    localRecPtr->Flags = PIXMAP_CACHE | OFFSCREEN_PIXMAPS |
        LINEAR_FRAMEBUFFER;

    /* HOOK SYNCRONIZARION ROUTINE */
    localRecPtr->Sync = LXAccelSync;

#if LX_FILL_RECT_SUPPORT
    /* HOOK FILLED RECTANGLES */
    HOOK(SetupForSolidFill);
    HOOK(SubsequentSolidFillRect);
    localRecPtr->SolidFillFlags = 0;
#endif

#if LX_MONO_8X8_PAT_SUPPORT
    /* HOOK 8x8 Mono EXPAND PATTERNS */
    HOOK(SetupForMono8x8PatternFill);
    HOOK(SubsequentMono8x8PatternFillRect);
    localRecPtr->Mono8x8PatternFillFlags =
        BIT_ORDER_IN_BYTE_MSBFIRST | SCANLINE_PAD_DWORD |
        HARDWARE_PATTERN_PROGRAMMED_BITS | HARDWARE_PATTERN_PROGRAMMED_ORIGIN;
#endif

#if LX_CLREXP_8X8_PAT_SUPPORT
    /* Color expansion */
    HOOK(SetupForColor8x8PatternFill);
    HOOK(SubsequentColor8x8PatternFillRect);
    localRecPtr->Color8x8PatternFillFlags =
        BIT_ORDER_IN_BYTE_MSBFIRST | SCANLINE_PAD_DWORD |
        NO_TRANSPARENCY | HARDWARE_PATTERN_PROGRAMMED_ORIGIN;
#endif

#if LX_SCR2SCRCPY_SUPPORT
    /* HOOK SCREEN TO SCREEN COPIES
     * Set flag to only allow copy if transparency is enabled.
     */
    HOOK(SetupForScreenToScreenCopy);
    HOOK(SubsequentScreenToScreenCopy);
    localRecPtr->ScreenToScreenCopyFlags =
        BIT_ORDER_IN_BYTE_MSBFIRST | SCANLINE_PAD_DWORD;
#endif

#if LX_BRES_LINE_SUPPORT
    /* HOOK BRESENHAM SOLID LINES */
    localRecPtr->SolidLineFlags = NO_PLANEMASK;
    HOOK(SetupForSolidLine);
    HOOK(SubsequentSolidBresenhamLine);
    HOOK(SubsequentSolidHorVertLine);
    HOOK(SubsequentSolidTwoPointLine);
    localRecPtr->SolidBresenhamLineErrorTermBits = 15;
#endif

#if LX_DASH_LINE_SUPPORT
    /* HOOK BRESENHAM DASHED LINES */
    localRecPtr->DashedLineFlags = NO_PLANEMASK | TRANSPARENCY_ONLY |
        LINE_PATTERN_LSBFIRST_LSBJUSTIFIED | SCANLINE_PAD_DWORD;
    HOOK(SetupForDashedLine);
    HOOK(SubsequentDashedBresenhamLine);
    HOOK(SubsequentDashedTwoPointLine);
    localRecPtr->DashedBresenhamLineErrorTermBits = 15;
    localRecPtr->DashPatternMaxLength = 32;
#endif

#if LX_SCR2SCREXP_SUPPORT
    /* Color expansion */
    HOOK(SetupForScreenToScreenColorExpandFill);
    HOOK(SubsequentScreenToScreenColorExpandFill);
    localRecPtr->ScreenToScreenColorExpandFillFlags =
        BIT_ORDER_IN_BYTE_MSBFIRST | SCANLINE_PAD_DWORD;
#endif

    if (pGeode->AccelImageWriteBuffers) {
#if LX_SCANLINE_SUPPORT
        localRecPtr->ScanlineImageWriteBuffers =
            pGeode->AccelImageWriteBuffers;
        localRecPtr->NumScanlineImageWriteBuffers = pGeode->NoOfImgBuffers;
        HOOK(SetupForScanlineImageWrite);
        HOOK(SubsequentScanlineImageWriteRect);
        HOOK(SubsequentImageWriteScanline);
        localRecPtr->ScanlineImageWriteFlags =
            BIT_ORDER_IN_BYTE_MSBFIRST | SCANLINE_PAD_DWORD;
#endif
#if !LX_USE_OFFSCRN_MEM
        ImgBufOffset = pGeode->AccelImageWriteBuffers[0] - pGeode->FBBase;
#else
        ImgLnsBuffers = (uchar *) pGeode->AccelImageWriteBuffers[0];
#endif
    } else {
        localRecPtr->PixmapCacheFlags = DO_NOT_BLIT_STIPPLES;
    }

    if (pGeode->AccelColorExpandBuffers) {
#if LX_CPU2SCREXP_SUPPORT
        /* Color expansion */
        localRecPtr->ScanlineColorExpandBuffers =
            pGeode->AccelColorExpandBuffers;
        localRecPtr->NumScanlineColorExpandBuffers =
            pGeode->NoOfColorExpandLines;
        HOOK(SetupForScanlineCPUToScreenColorExpandFill);
        HOOK(SubsequentScanlineCPUToScreenColorExpandFill);
        HOOK(SubsequentColorExpandScanline);
        localRecPtr->ScanlineCPUToScreenColorExpandFillFlags =
            BIT_ORDER_IN_BYTE_MSBFIRST | SCANLINE_PAD_DWORD;
#endif
        ClrLnsBuffers = (uchar *) pGeode->AccelColorExpandBuffers[0];
    }
#if LX_WRITE_PIXMAP_SUPPORT
    HOOK(WritePixmap);
#endif

    return (XAAInit(pScrn, localRecPtr));
}

/* END OF FILE */
