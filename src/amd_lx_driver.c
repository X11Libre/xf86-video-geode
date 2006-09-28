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
 * File Contents: This is the main module configures the interfacing 
 *                with the X server. The individual modules will be 
 *                loaded based upon the options selected from the 
 *                XF86Config. This file also has modules for finding 
 *                supported modes, turning on the modes based on options.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Includes that are used by all drivers */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86_libc.h"
#include "xf86Resources.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86cmap.h"

#define RC_MAX_DEPTH 24

#include "amd.h"
#include "cim_defs.h"
#include "cim_regs.h"
#include "cim_dev.h"

/* Frame buffer stuff */
#if CFB
/*
 * If using cfb, cfb.h is required.  Select the others for the bpp values
 * the driver supports.
 */
#define PSZ 8                          /* needed for cfb.h */
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#else
#include "fb.h"
#endif

#include "shadowfb.h"

/* Machine independent stuff */
#include "mipointer.h"
#include "mibank.h"
#include "micmap.h"
#include "mibstore.h"
#include "vgaHW.h"
#include "vbe.h"

/* Check for some extensions */
#ifdef XFreeXDGA
#define _XF86_DGA_SERVER_
#include <X11/extensions/xf86dgastr.h>
#endif /* XFreeXDGA */

#ifdef DPMSExtension
#include "globals.h"
#include "opaque.h"
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#endif /* DPMSExtension */

#include "amd_lx_vga.c"

extern SymTabRec GeodeChipsets[];
extern OptionInfoRec LX_GeodeOptions[];

/* Forward definitions */
static Bool LXPreInit(ScrnInfoPtr, int);
static Bool LXScreenInit(int, ScreenPtr, int, char **);
static Bool LXEnterVT(int, int);
static void LXLeaveVT(int, int);
static void LXFreeScreen(int, int);
void LXAdjustFrame(int, int, int, int);
Bool LXSwitchMode(int, DisplayModePtr, int);
static int LXValidMode(int, DisplayModePtr, Bool, int);
static void LXLoadPalette(ScrnInfoPtr pScrni,
    int numColors, int *indizes, LOCO * colors, VisualPtr pVisual);
static Bool LXMapMem(ScrnInfoPtr);
static Bool LXUnmapMem(ScrnInfoPtr);

extern Bool LXAccelInit(ScreenPtr pScrn);
extern Bool LXHWCursorInit(ScreenPtr pScrn);
extern void LXHideCursor(ScrnInfoPtr pScrni);
extern void LXShowCursor(ScrnInfoPtr pScrni);
extern void LXLoadCursorImage(ScrnInfoPtr pScrni, unsigned char *src);
extern void LXPointerMoved(int index, int x, int y);
extern void LXRotationInit(ScrnInfoPtr pScrni);
extern void LXShadowFBInit(ScreenPtr pScrn, GeodeRec *pGeode, int bytpp);
extern void LXInitVideo(ScreenPtr pScrn);
extern Bool LXDGAInit(ScreenPtr pScrn);

unsigned char *XpressROMPtr;
unsigned long fb;

extern const char *amdVgahwSymbols[];
extern const char *amdVbeSymbols[];
extern const char *amdInt10Symbols[];

#if CFB
extern const char *amdCfbSymbols[];
#else
extern const char *amdFbSymbols[];
#endif
extern const char *amdXaaSymbols[];
extern const char *amdRamdacSymbols[];
extern const char *amdShadowSymbols[];

void LXSetupChipsetFPtr(ScrnInfoPtr pScrni);
GeodeRec *LXGetRec(ScrnInfoPtr pScrni);
void lx_clear_screen(ScrnInfoPtr pScrni, int width, int height, int bpp);
void lx_clear_fb(ScrnInfoPtr pScrni);
void lx_disable_dac_power(ScrnInfoPtr pScrni, int option);
void lx_enable_dac_power(ScrnInfoPtr pScrni, int option);

#if DEBUGLVL>0
FILE *zdfp = NULL;
#endif

/* Reference: Video Graphics Suite Specification:
 * VG Config Register (0x00) page 16
 * VG FP Register (0x02) page 18 */

#define LX_READ_VG(reg)	\
		(outw(0xAC1C,0xFC53), outw(0xAC1C,0x0200|(reg)), inw(0xAC1E))

/* panel resolutiosn decode of FP config reg */

static struct sLXFpResolution
{
    int xres, yres;
} lx_fp_resolution[] = {
    {
    320, 240}, {
    640, 480}, {
    800, 600}, {
    1024, 768}, {
    1152, 864}, {
    1280, 1024}, {
    1600, 1200}, {
    0, 0}
};

/* Get the information from the BIOS regarding the panel */

static int
lx_get_panel_info(int *xres, int *yres)
{
    unsigned short reg = LX_READ_VG(0x02);

    *xres = lx_fp_resolution[(reg >> 3) & 0x07].xres;
    *yres = lx_fp_resolution[(reg >> 3) & 0x07].yres;

    return 0;
}

static int
lx_panel_configured(void)
{
    unsigned short reg = LX_READ_VG(0x00);
    unsigned char ret = (reg >> 8) & 0x7;

    return (ret == 1) || (ret == 5) ? 1 : 0;
}

void
LXSetupChipsetFPtr(ScrnInfoPtr pScrni)
{
#if DEBUGLVL>0
    if (zdfp == NULL) {
        zdfp = fopen("/tmp/xwin.log", "w");
        setbuf(zdfp, NULL);
    }
#endif
    DEBUGMSG(1, (0, X_INFO, "LXSetupChipsetFPtr!\n"));
    pScrni->PreInit = LXPreInit;
    pScrni->ScreenInit = LXScreenInit;
    pScrni->SwitchMode = LXSwitchMode;
    pScrni->AdjustFrame = LXAdjustFrame;
    pScrni->EnterVT = LXEnterVT;
    pScrni->LeaveVT = LXLeaveVT;
    pScrni->FreeScreen = LXFreeScreen;
    pScrni->ValidMode = LXValidMode;
}

#ifdef AMD_V4L2_VIDEO
void
LXInitVideo(ScreenPtr pScrn)
{
    GeodeRec *pGeode;
    int num_adaptors;
    XF86VideoAdaptorPtr *adaptors;
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];

    pGeode = GEODEPTR(pScrni);
    if (!pGeode->NoAccel) {
        num_adaptors = xf86XVListGenericAdaptors(pScrni, &adaptors);
        if (num_adaptors > 0)
            xf86XVScreenInit(pScrn, adaptors, num_adaptors);
    }
}
#else
extern void LXInitVideo(ScreenPtr pScrn);
#endif

/*----------------------------------------------------------------------------
 * LXGetRec.
 *
 * Description	:This function allocate an GeodeRec and hooked into
 * pScrni 	 str driverPrivate member of ScreeenInfo
 * 				 structure.
 * Parameters.
 * pScrni 	:Pointer handle to the screenonfo structure.
 *
 * Returns		:allocated driver structure.
 *
 * Comments     :none
 *
*----------------------------------------------------------------------------
*/
GeodeRec *
LXGetRec(ScrnInfoPtr pScrni)
{
    if (!pScrni->driverPrivate) {
        GeodeRec *pGeode;

        pGeode = pScrni->driverPrivate = xnfcalloc(sizeof(GeodeRec), 1);
#if INT10_SUPPORT
        pGeode->vesa = xcalloc(sizeof(VESARec), 1);
#endif
    }

    return GEODEPTR(pScrni);
}

/*----------------------------------------------------------------------------
 * LXFreeRec.
 *
 * Description	:This function deallocate an GeodeRec and freed from
 *               pScrni str driverPrivate member of ScreeenInfo
 *               structure.
 * Parameters.
 * pScrni	:Pointer handle to the screenonfo structure.
 *
 * Returns		:none
 *
 * Comments     :none
 *
*----------------------------------------------------------------------------
*/
static void
LXFreeRec(ScrnInfoPtr pScrni)
{
    if (pScrni->driverPrivate == NULL) {
        return;
    }
    xfree(pScrni->driverPrivate);
    pScrni->driverPrivate = NULL;
}

/*----------------------------------------------------------------------------
 * LXSaveScreen.
 *
 * Description	:This is to do the screen blanking
 *
 * Parameters.
 *     pScrn	:Handle to ScreenPtr structure.
 *     mode		:mode is used by vgaHWSaveScren to check blnak os on.
 * 												
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments     :none
*----------------------------------------------------------------------------
*/
static Bool
LXSaveScreen(ScreenPtr pScrn, int mode)
{
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];

    DEBUGMSG(0, (0, X_INFO, "LXSaveScreen!\n"));

    if (!pScrni->vtSema)
        return vgaHWSaveScreen(pScrn, mode);

    return TRUE;
}

static xf86MonPtr
LXProbeDDC(ScrnInfoPtr pScrni, int index)
{
    vbeInfoPtr pVbe;
    xf86MonPtr ddc = NULL;

    if (xf86LoadSubModule(pScrni, "vbe")) {
        pVbe = VBEInit(NULL, index);
        ddc = vbeDoEDID(pVbe, NULL);
        vbeFree(pVbe);
    }

    return ddc;
}

static void
LXDecodeDDC(ScrnInfoPtr pScrni, xf86MonPtr ddc)
{
    int i;

    DEBUGMSG(1, (0, X_INFO,
            "Detected monitor DDC (%4s) id %d serno %d week %d year %d "
            "nsects %d\n", &ddc->vendor.name[0], ddc->vendor.prod_id,
            ddc->vendor.serial, ddc->vendor.week, ddc->vendor.year,
            ddc->no_sections));
    for (i = 0; i < DET_TIMINGS; ++i) {
        struct detailed_monitor_section *dp = &ddc->det_mon[i];

        switch (dp->type) {
        case DT:{
                struct detailed_timings *r = &dp->section.d_timings;

                DEBUGMSG(1, (0, X_INFO, "  mon det timing %0.3f  %dx%d\n",
                        r->clock / 1000000.0, r->h_active, r->v_active));
                DEBUGMSG(1, (0, X_INFO,
                        "  mon border %d,%d laced %d stereo %d sync %d, misc %d\n",
                        r->h_border, r->v_border, r->interlaced, r->stereo,
                        r->sync, r->misc));
            }
            break;

        case DS_SERIAL:{
                char *serial_no = (char *)&dp->section.serial[0];

                DEBUGMSG(1, (0, X_INFO, "  mon serial %13s\n", serial_no));
            }
            break;

        case DS_ASCII_STR:{
                char *ascii = (char *)&dp->section.ascii_data[0];

                DEBUGMSG(1, (0, X_INFO, "  mon ascii_str %13s\n", ascii));
            }
            break;

        case DS_NAME:{
                char *name = (char *)&dp->section.name[0];

                DEBUGMSG(1, (0, X_INFO, "  mon name %13s\n", name));
            } break;

        case DS_RANGES:{
                struct monitor_ranges *r = &dp->section.ranges;

                DEBUGMSG(1, (0, X_INFO,
                        "  mon ranges v %d-%d h %d-%d clk %d\n", r->min_v,
                        r->max_v, r->min_h, r->max_h, r->max_clock));
            }
            break;

        case DS_WHITE_P:{
                struct whitePoints *wp = &dp->section.wp[0];

                DEBUGMSG(1, (0, X_INFO,
                        "  mon whitept %f,%f %f,%f idx %d,%d gamma %f,%f\n",
                        wp[1].white_x, wp[1].white_y, wp[2].white_x,
                        wp[2].white_y, wp[1].index, wp[2].index,
                        wp[1].white_gamma, wp[2].white_gamma));
            }
            break;

        case DS_STD_TIMINGS:{
                struct std_timings *t = &dp->section.std_t[0];

                DEBUGMSG(1, (0, X_INFO,
                        "  mon std_timing no   size   @rate, id\n"));
                for (i = 0; i < 5; ++i)
                    DEBUGMSG(1, (0, X_INFO,
                            "                 %d %5dx%-5d @%-4d %d\n", i,
                            t[i].hsize, t[i].vsize, t[i].refresh, t[i].id));
            }
            break;
        }
    }
}

static void
LXFBAlloc(int fd, unsigned int offset, unsigned int size)
{
    cim_mem_req_t req;

    memset(&req, 0, sizeof(req));
    strcpy(&req.owner[0], "XFree86");
    sprintf(&req.name[0], "%08lx", offset);
    req.flags = CIM_F_PUBLIC;
    req.size = size;
    ioctl(fd, CIM_RESERVE_MEM, &req);
}

/* check for "tv-" or "pnl-" in mode name, decode suffix */
/*  return -1 if mode name is not a known tv/pnl mode */
static int
lx_tv_mode(DisplayModePtr pMode)
{
    int tv_mode = -1;
    char *bp, *cp;

    cp = pMode->name;
    if ((*(bp = cp) == 't' && *++bp == 'v') ||
        (*(bp = cp) == 'p' && *++bp == 'n' && *++bp == 'l')) {
        if (*++bp == '-') {
            if (xf86NameCmp("ntsc", ++bp) == 0)
                tv_mode = VG_TVMODE_NTSC;
            else if (xf86NameCmp("pal", bp) == 0)
                tv_mode = VG_TVMODE_PAL;
            else if (xf86NameCmp("6x4_ntsc", bp) == 0)
                tv_mode = VG_TVMODE_6X4_NTSC;
            else if (xf86NameCmp("8x6_ntsc", bp) == 0)
                tv_mode = VG_TVMODE_8X6_NTSC;
            else if (xf86NameCmp("10x7_ntsc", bp) == 0)
                tv_mode = VG_TVMODE_10X7_NTSC;
            else if (xf86NameCmp("6x4_pal", bp) == 0)
                tv_mode = VG_TVMODE_6X4_PAL;
            else if (xf86NameCmp("8x6_pal", bp) == 0)
                tv_mode = VG_TVMODE_8X6_PAL;
            else if (xf86NameCmp("10x7_pal", bp) == 0)
                tv_mode = VG_TVMODE_10X7_PAL;
            else if (xf86NameCmp("480p", bp) == 0)
                tv_mode = VG_TVMODE_480P;
            else if (xf86NameCmp("720p", bp) == 0)
                tv_mode = VG_TVMODE_720P;
            else if (xf86NameCmp("1080i", bp) == 0)
                tv_mode = VG_TVMODE_1080I;
        }
    }

    return tv_mode;
}

static int
lx_tv_mode_interlaced(int tv_mode)
{
    switch (tv_mode) {
    case VG_TVMODE_NTSC:
    case VG_TVMODE_PAL:
    case VG_TVMODE_1080I:
        return 1;
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * LXPreInit.
 *
 * Description	:This function is called only once ate teh server startup
 *
 * Parameters.
 *  pScrni :Handle to ScreenPtr structure.
 *  options       :may be used to check the probeed one with config.
 * 												
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static Bool
LXPreInit(ScrnInfoPtr pScrni, int options)
{
    ClockRangePtr GeodeClockRange;
    MessageType from;
    int i = 0;
    GeodeRec *pGeode;
    char *mod = NULL;
    xf86MonPtr ddc = NULL;
    Q_WORD msrValue;

#if CFB
    char *reqSymbol = NULL;
#endif /* CFB */
    unsigned long offset, length, fb_top;
    char dev_name[64], dev_owner[64];
    FILE *fp;
    int fd;

#if INT10_SUPPORT
    VESARec *pVesa;
#endif
    unsigned int PitchInc, minPitch, maxPitch;
    unsigned int minHeight, maxHeight;
    unsigned int flags, fmt, high, low;
    int tv_encoder, tv_bus_fmt, tv_601_fmt;
    int tv_conversion, tvox, tvoy, tv_flags;
    int tv_601_flags, tv_vsync_shift, tv_vsync_select;
    int tv_vsync_shift_count;
    const char *s;
    char **modes;
    OptionInfoRec *GeodeOptions = &LX_GeodeOptions[0];

    /*
     * Setup the ClockRanges, which describe what clock ranges are
     * available, and what sort of modes they can be used for.
     */
    GeodeClockRange = (ClockRangePtr) xnfcalloc(sizeof(ClockRange), 1);
    GeodeClockRange->next = NULL;
    GeodeClockRange->minClock = 25175;
    GeodeClockRange->maxClock = 341350;
    GeodeClockRange->clockIndex = -1;  /* programmable */
    GeodeClockRange->interlaceAllowed = TRUE;
    GeodeClockRange->doubleScanAllowed = FALSE; /* XXX check this */

    /* Select valid modes from those available */
    minPitch = 1024;
    maxPitch = 8192;                   /* Can support upto 1920x1440 32Bpp */
    minHeight = 400;
    maxHeight = 2048;                  /* Can support upto 1920x1440 32Bpp */
    /* need height >= maxWidth for rotate */

    DEBUGMSG(1, (0, X_INFO, "LXPreInit!\n"));
    /* Allocate driver private structure */
    if (!(pGeode = LXGetRec(pScrni)))
        return FALSE;

    DEBUGMSG(1, (0, X_NONE, "pGeode = %p\n", (void *)pGeode));

    /* This is the general case */
    for (i = 0; i < pScrni->numEntities; i++) {
        pGeode->pEnt = xf86GetEntityInfo(pScrni->entityList[i]);
        if (pGeode->pEnt->resources)
            return FALSE;
        pGeode->Chipset = pGeode->pEnt->chipset;
        pScrni->chipset = (char *)xf86TokenToString(GeodeChipsets,
            pGeode->pEnt->chipset);
    }

    if ((options & PROBE_DETECT) != 0) {
        ConfiguredMonitor = LXProbeDDC(pScrni, pGeode->pEnt->index);
        return TRUE;
    }
#if INT10_SUPPORT
    if (!xf86LoadSubModule(pScrni, "int10"))
        return FALSE;
    xf86LoaderReqSymLists(amdInt10Symbols, NULL);
#endif

    /* If the vgahw module would be needed it would be loaded here */
    if (!xf86LoadSubModule(pScrni, "vgahw")) {
        return FALSE;
    }
    xf86LoaderReqSymLists(amdVgahwSymbols, NULL);
    /* Do the cimarron hardware detection */
    init_detect_cpu(&pGeode->cpu_version, &pGeode->cpu_revision);

    /* find the base chipset core. Currently there can be only one 
     * chip active at any time.
     */
    if ((pGeode->cpu_version & 0xFF) != CIM_CPU_GEODELX) {
        ErrorF("Chipset not GEODELX !!!\n");
        return FALSE;
    }
    pGeode->DetectedChipSet = LX;
    DEBUGMSG(1, (0, X_INFO, "Detected BaseChip (%d)\n",
            pGeode->DetectedChipSet));

    /* LX : Can have CRT or PANEL/TVO only */
    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msrValue);

    fmt = (msrValue.low >> 3) & 0x07;
    switch (fmt) {
    case 4:
    case 2:
    case 0:
        flags = LX_OT_CRT;
        break;
    case 1:
    case 3:
    case 5:
        flags = LX_OT_FP;
        if ((msrValue.low & 0x8000) != 0)
            flags |= LX_OT_CRT;
        break;
    case 6:
        flags = LX_OT_VOP;
        break;
    case 7:
        flags = LX_OT_DRGB;
        break;
    }

    /* currently supported formats */
    flags &= (LX_OT_FP | LX_OT_CRT | LX_OT_VOP);
    /*  can switch from crt/pnl vop, but not the other way */
    flags |= LX_OT_VOP;
    pGeode->EnabledOutput = flags;

    xf86DrvMsg(0, X_INFO, "AMD LX Output Formats -%sCRT,%sVOP,%sFP,%sDRGB\n",
        ((flags & LX_OT_CRT) ? " " : " No "),
        ((flags & LX_OT_VOP) ? " " : " No "),
        ((flags & LX_OT_FP) ? " " : " No "),
        ((flags & LX_OT_DRGB) ? " " : " No "));

    pGeode->vid_version = CIM_CPU_GEODELX;
    init_read_base_addresses(&(pGeode->InitBaseAddress));

    pGeode->FBLinearAddr = pGeode->InitBaseAddress.framebuffer_base;
    fb_top = pGeode->InitBaseAddress.framebuffer_size;

    if (pGeode->pEnt->device->videoRam == 0) {
        from = X_PROBED;
        pScrni->videoRam = fb_top / 1024;
    } else {
        pScrni->videoRam = pGeode->pEnt->device->videoRam;
        from = X_CONFIG;
        fb_top = pScrni->videoRam << 10;
    }

    DEBUGMSG(1, (pScrni->scrnIndex, from, "VideoRam: %lu kByte\n",
            (unsigned long)pScrni->videoRam));

    pGeode->CmdBfrOffset = 0;
    pGeode->CmdBfrSize = 0;

    /*
     * Force modules to load now, to reserve fb mem for other drivers
     */

    pGeode->cimFd = open("/dev/cimarron", O_RDONLY);
    if (pGeode->cimFd < 0)
        pGeode->cimFd = open("/dev/cim", O_RDONLY);
    DEBUGMSG(1, (pScrni->scrnIndex, X_INFO, "/dev/cimarron: fd=%d\n",
            pGeode->cimFd));
    if ((fd = open("/dev/video", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video0", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video1", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video2", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video3", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video/video0", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video/video1", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video/video2", O_RDONLY)) >= 0)
        close(fd);
    if ((fd = open("/dev/video/video3", O_RDONLY)) >= 0)
        close(fd);

    DEBUGMSG(1, (pScrni->scrnIndex, X_INFO, "open cim map\n"));
    if ((fp = fopen("/proc/driver/cimarron/map", "r")) != NULL) {
        low = 0;
        high = fb_top;

        DEBUGMSG(1, (pScrni->scrnIndex, X_INFO, "scan cim map\n"));
        while (fscanf(fp, "%63s %63s %x %lx %lx\n",
                &dev_owner[0], &dev_name[0], &flags, &offset, &length) == 5) {
            if (offset < pGeode->FBLinearAddr)
                continue;
            offset -= pGeode->FBLinearAddr;
            if ((flags & CIM_F_CMDBUF) != 0) {
                pGeode->CmdBfrOffset = offset;
                pGeode->CmdBfrSize = length;
            }
            if (offset >= fb_top)
                continue;
            if ((flags & CIM_F_PRIVATE) != 0)
                if (offset < high)
                    high = offset;
            if (offset + length >= fb_top)
                length = fb_top - offset;
            if ((flags & CIM_F_PUBLIC) != 0 || (flags & CIM_F_FREE) != 0)
                if (offset + length > low)
                    low = offset + length;
        }
        fclose(fp);
        fb_top = high < low ? high : low;
    }

    DEBUGMSG(1, (0, X_INFO, "fb_top = %08lx\n", fb_top));
    DEBUGMSG(1, (pScrni->scrnIndex, X_INFO,
            "cmd bfr (map) %08lx/%08lx\n", pGeode->CmdBfrOffset,
            pGeode->CmdBfrSize));

    /* if cimarron module not reporting, use default buffer parameters */
    if (pGeode->CmdBfrSize == 0) {
        if (fb_top < CIM_CMD_BFR_SZ) {
            ErrorF("No memory for CMD_BFR !!!\n");
            return FALSE;
        }
        pGeode->CmdBfrSize = CIM_CMD_BFR_SZ;
        fb_top -= pGeode->CmdBfrSize;
        pGeode->CmdBfrOffset = fb_top;
    }

    if (pGeode->CmdBfrSize < CIM_CMD_BFR_MIN) {
        ErrorF("Not enough memory for CMD_BFR !!!\n");
        return FALSE;
    }
    DEBUGMSG(1, (pScrni->scrnIndex, X_INFO,
            "cmd bfr (actual) %08lx/%08lx\n", pGeode->CmdBfrOffset,
            pGeode->CmdBfrSize));

    /* now soak up all of the usable framebuffer memory */
    if (pGeode->cimFd >= 0) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_INFO, "alloc cim map\n"));
        for (;;) {
            if ((fp = fopen("/proc/driver/cimarron/map", "r")) == NULL)
                break;
            low = fb_top;
            high = 0;
            while (fscanf(fp, "%63s %63s %x %lx %lx\n",
                    &dev_owner[0], &dev_name[0], &flags, &offset,
                    &length) == 5) {
                if (offset < pGeode->FBLinearAddr)
                    continue;
                offset -= pGeode->FBLinearAddr;
                if (offset >= fb_top)
                    continue;
                if ((flags & CIM_F_FREE) == 0)
                    continue;
                if (low < offset)
                    continue;
                low = offset;
                if (offset + length > fb_top)
                    length = fb_top - offset;
                high = length;
            }
            fclose(fp);
            if (high == 0)
                break;
            LXFBAlloc(pGeode->cimFd, low, high);
        }
        DEBUGMSG(1, (pScrni->scrnIndex, X_INFO, "check cim map\n"));
        /* only shared memory allowed below fb_top */
        if ((fp = fopen("/proc/driver/cimarron/map", "r")) != NULL) {
            low = 0;
            while (fscanf(fp, "%63s %63s %x %lx %lx\n",
                    &dev_owner[0], &dev_name[0], &flags, &offset,
                    &length) == 5) {
                if (offset < pGeode->FBLinearAddr)
                    continue;
                offset -= pGeode->FBLinearAddr;
                if (offset >= fb_top)
                    continue;
                if ((flags & CIM_F_PUBLIC) == 0) {
                    low = 1;
                    break;
                }
            }
            fclose(fp);
            if (low != 0) {
                ErrorF("Not able to claim free FB memory !!!\n");
                return FALSE;
            }
        }
    }

    pGeode->FBTop = fb_top;
    if (fb_top < GP3_SCRATCH_BUFFER_SIZE) {
        ErrorF("No available FB memory !!!\n");
        return FALSE;
    }

    /* must remove cimarron scratch buffer from FB */
    pGeode->FBAvail = fb_top - GP3_SCRATCH_BUFFER_SIZE;

    DEBUGMSG(1, (pScrni->scrnIndex, X_INFO, "FBAvail = %08lx\n",
            pGeode->FBAvail));
    if (pGeode->DetectedChipSet & LX) {
        pGeode->cpu_reg_size = 0x4000;
        pGeode->gp_reg_size = 0x4000;
        pGeode->vg_reg_size = 0x4000;
        pGeode->vid_reg_size = 0x4000;
        pGeode->vip_reg_size = 0x4000;
    } else {
        pGeode->cpu_reg_size = 0x9000;
        pGeode->vid_reg_size = 0x1000;
    }

    if (!LXMapMem(pScrni))
        return FALSE;

    /* KFB will Knock off VGA */
    /* check if VGA is active */
    pGeode->FBVGAActive = gu3_get_vga_active();

    DEBUGMSG(1, (0, X_PROBED, "VGA = %d\n", pGeode->FBVGAActive));

    /* Fill in the monitor field */
    pScrni->monitor = pScrni->confScreen->monitor;
    /* Determine depth, bpp, etc. */
    if (!xf86SetDepthBpp(pScrni, 8, 8, 8, Support24bppFb | Support32bppFb))
        return FALSE;

    if (!((pScrni->depth == 8) || (pScrni->depth == 16) ||
            (pScrni->depth == 24) || (pScrni->depth == 32))) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                "Given depth (%d bpp) is not supported by this driver\n",
                pScrni->depth));
        return FALSE;
    }

    /*
     * This must happen after pScrni->display has been set
     * because xf86SetWeight references it.
     */
    if (pScrni->depth > 8) {
        /* The defaults are OK for us */
        rgb BitsPerComponent = { 0, 0, 0 };
        rgb BitMask = { 0, 0, 0 };

        if (pScrni->depth > 16) {
            /* we are operating in 24 bpp, Redcloud */
            BitsPerComponent.red = 8;
            BitsPerComponent.green = 8;
            BitsPerComponent.blue = 8;

            BitMask.red = 0xFF0000;
            BitMask.green = 0x00FF00;
            BitMask.blue = 0x0000FF;
        }
        if (xf86SetWeight(pScrni, BitsPerComponent, BitMask) == 0)
            return FALSE;
    }

    xf86PrintDepthBpp(pScrni);

    if (!xf86SetDefaultVisual(pScrni, -1))
        return FALSE;

    /* The new cmap layer needs this to be initialized */
    if (pScrni->depth > 1) {
        Gamma zeros = { 0.0, 0.0, 0.0 };

        if (!xf86SetGamma(pScrni, zeros)) {
            return FALSE;
        }
    }

    /* We use a programmable clock */
    pScrni->progClock = TRUE;

    /*
     * Collect all of the relevant option flags
     * (fill in pScrni->options)
     */
    xf86CollectOptions(pScrni, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrni->scrnIndex, pScrni->options, GeodeOptions);

#if INT10_SUPPORT
    pVesa = pGeode->vesa;
    /* Initialize Vesa record */

    if ((pVesa->pInt = xf86InitInt10(pGeode->pEnt->index)) == NULL) {
        xf86DrvMsg(0, X_ERROR, "Int10 initialization failed.\n");
        return (FALSE);
    }
#endif

    if (pScrni->depth == 8) {
        /* Default to 8 */
        pScrni->rgbBits = 8;
    }
    from = X_DEFAULT;

    pGeode->FPGeomDstSet = 0;
    if ((s = xf86GetOptValString(GeodeOptions,
                LX_OPTION_FP_DEST_GEOM)) != NULL) {
        char *sp;
        int xres = strtoul(s, &sp, 0);

        if (sp != NULL && *sp == 'x') {
            int yres = strtoul(sp + 1, &sp, 0);

            if (sp != NULL && *sp == 0) {
                if (xres > 0 && xres <= maxPitch &&
                    yres >= minHeight && yres <= maxHeight) {
                    pGeode->FPGeomDstSet = 1;
                    pGeode->FPGeomDstX = xres;
                    pGeode->FPGeomDstY = yres;
                } else
                    DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                            "FP_DEST_GEOM \"%dx%d\" out of range\n", xres,
                            yres));
            }
        }

        if (pGeode->FPGeomDstSet == 0) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "FP_DEST_GEOM \"%s\" not recognized\n", s));
            return FALSE;
        }
        pGeode->EnabledOutput |= LX_OT_FP;
    }

    pGeode->FPGeomActSet = 0;
    if ((s = xf86GetOptValString(GeodeOptions,
                LX_OPTION_FP_ACTIVE_GEOM)) != NULL) {
        char *sp;
        int xres = strtoul(s, &sp, 0);

        if (sp != NULL && *sp == 'x') {
            int yres = strtoul(sp + 1, &sp, 0);

            if (sp != NULL && *sp == 0) {
                if (xres > 0 && xres <= maxPitch &&
                    yres >= minHeight && yres <= maxHeight) {
                    pGeode->FPGeomActSet = 1;
                    pGeode->FPGeomActX = xres;
                    pGeode->FPGeomActY = yres;
                } else
                    DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                            "FP_ACTIVE_GEOM \"%s\" out of range\n", s));
            }
        }
        if (pGeode->FPGeomActSet == 0) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "FP_ACTIVE_GEOM \"%s\" not recognized\n", s));
            return FALSE;
        }
        pGeode->EnabledOutput |= LX_OT_FP;
    }

    if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
        if (pGeode->FPGeomDstSet == 0) {
            if (lx_panel_configured() == 0) {
                ErrorF("Panel configured and enabled but not configured "
                    "in BIOS !!!\n");
                return FALSE;
            }
            lx_get_panel_info(&pGeode->FPBiosResX, &pGeode->FPBiosResY);
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "FP Bios panel configuation used\n"));
            pGeode->FPGeomDstX = pGeode->FPBiosResX;
            pGeode->FPGeomDstY = pGeode->FPBiosResY;
        }
        if (pGeode->FPGeomActSet == 0) {
            pGeode->FPGeomActX = pGeode->FPGeomDstX;
            pGeode->FPGeomActY = pGeode->FPGeomDstY;
        }
        if (pGeode->FPGeomActX > pGeode->FPGeomDstX ||
            pGeode->FPGeomActY > pGeode->FPGeomDstY) {
            ErrorF("FP Geom params Active %dx%d bigger than Dest %dx%d\n",
                pGeode->FPGeomActX, pGeode->FPGeomActY,
                pGeode->FPGeomDstX, pGeode->FPGeomDstY);
            return FALSE;
        }
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                "FP Geom params Dest %dx%d, Active %dx%d\n",
                pGeode->FPGeomDstX, pGeode->FPGeomDstY,
                pGeode->FPGeomActX, pGeode->FPGeomActY));
    }

    if (xf86IsOptionSet(GeodeOptions, LX_OPTION_FLATPANEL)) {
        if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_FLATPANEL, TRUE)) {
            if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "FlatPanel Selected\n"));
            } else
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "FlatPanel Selected, but not available - ignored\n"));
        } else {
            if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "FlatPanel configured, but not enabled\n"));
                pGeode->EnabledOutput &= ~LX_OT_FP;
            }
        }
    }

    /*
     * The preferred method is to use the "hw cursor" option as a tri-state
     * option, with the default set above.
     */
    pGeode->HWCursor = TRUE;
    if (xf86GetOptValBool(GeodeOptions, LX_OPTION_HW_CURSOR,
            &pGeode->HWCursor)) {
        from = X_CONFIG;
    }
    /* For compatibility, accept this too (as an override) */
    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_SW_CURSOR, FALSE)) {
        from = X_CONFIG;
        pGeode->HWCursor = FALSE;
    }
    DEBUGMSG(1, (pScrni->scrnIndex, from, "Using %s cursor\n",
            pGeode->HWCursor ? "HW" : "SW"));

    pGeode->Compression = TRUE;
    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_NOCOMPRESSION, FALSE)) {
        pGeode->Compression = FALSE;
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG, "NoCompression\n"));
    }

    pGeode->NoAccel = FALSE;
    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_NOACCEL, FALSE)) {
        pGeode->NoAccel = TRUE;
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG, "Acceleration disabled\n"));
    }

    if (!xf86GetOptValInteger(GeodeOptions, LX_OPTION_OSM_IMG_BUFS,
            &(pGeode->NoOfImgBuffers)))
        pGeode->NoOfImgBuffers = DEFAULT_IMG_LINE_BUFS;
    DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
            "NoOfImgBuffers = %d\n", pGeode->NoOfImgBuffers));
    if (!xf86GetOptValInteger(GeodeOptions, LX_OPTION_OSM_CLR_BUFS,
            &(pGeode->NoOfColorExpandLines)))
        pGeode->NoOfColorExpandLines = DEFAULT_CLR_LINE_BUFS;
    if (pGeode->NoOfColorExpandLines <= 0)
        pGeode->NoOfColorExpandLines = 0;
    DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
            "NoOfColorExpandLines = %d\n", pGeode->NoOfColorExpandLines));

    pGeode->CustomMode = FALSE;
    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_CUSTOM_MODE, FALSE)) {
        pGeode->CustomMode = TRUE;
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG, "Custom mode enabled\n"));
    }

    if (xf86IsOptionSet(GeodeOptions, LX_OPTION_CRTENABLE)) {
        if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_CRTENABLE, TRUE)) {
            if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "CRT Output Selected\n"));
            } else
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "CRT Output Selected, but not available "
                        "- ignored\n"));
        } else {
            if ((pGeode->EnabledOutput & LX_OT_CRT) != 0) {
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "CRT output configured, but not enabled\n"));
                pGeode->EnabledOutput &= ~LX_OT_CRT;
            }
        }
    }

    pGeode->TVSupport = FALSE;
    if ((s = xf86GetOptValString(GeodeOptions, LX_OPTION_TV_ENCODER))
        != NULL) {
        tv_encoder = -1;
        if (xf86NameCmp(s, "ADV7171") == 0)
            tv_encoder = VG_ENCODER_ADV7171;
        else if (xf86NameCmp(s, "SAA7127") == 0)
            tv_encoder = VG_ENCODER_SAA7127;
        else if (xf86NameCmp(s, "FS454") == 0)
            tv_encoder = VG_ENCODER_FS454;
        else if (xf86NameCmp(s, "ADV7300") == 0)
            tv_encoder = VG_ENCODER_ADV7300;
        if (tv_encoder < 0) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "VOP output configured, but no encoder specified, "
                    "VOP diabled\n"));
            pGeode->EnabledOutput &= ~LX_OT_VOP;
        } else
            pGeode->TVSupport = TRUE;
        pGeode->tv_encoder = tv_encoder;
    }

    /* If TV Supported then check for TVO support */
    if (pGeode->TVSupport != FALSE) {
        tv_bus_fmt = -1;
        tv_601_fmt = -1;
        if ((s = xf86GetOptValString(GeodeOptions,
                    LX_OPTION_TV_BUS_FMT)) != NULL) {
            if (xf86NameCmp(s, "disabled") == 0)
                tv_bus_fmt = VOP_MODE_DISABLED;
            else if (xf86NameCmp(s, "vip11") == 0)
                tv_bus_fmt = VOP_MODE_VIP11;
            else if (xf86NameCmp(s, "ccir656") == 0)
                tv_bus_fmt = VOP_MODE_CCIR656;
            else if (xf86NameCmp(s, "vip20_8bit") == 0)
                tv_bus_fmt = VOP_MODE_VIP20_8BIT;
            else if (xf86NameCmp(s, "vip20_16bit") == 0)
                tv_bus_fmt = VOP_MODE_VIP20_16BIT;
            else if (xf86NameCmp(s, "601_yuv_8bit") == 0) {
                tv_601_fmt = VOP_601_YUV_8BIT;
                tv_bus_fmt = VOP_MODE_601;
            } else if (xf86NameCmp(s, "601_yuv_16bit") == 0) {
                tv_601_fmt = VOP_601_YUV_16BIT;
                tv_bus_fmt = VOP_MODE_601;
            } else if (xf86NameCmp(s, "601_rgb_8_8_8") == 0) {
                tv_601_fmt = VOP_601_RGB_8_8_8;
                tv_bus_fmt = VOP_MODE_601;
            } else if (xf86NameCmp(s, "601_yuv_4_4_4") == 0) {
                tv_601_fmt = VOP_601_YUV_4_4_4;
                tv_bus_fmt = VOP_MODE_601;
            }
        }
        if (tv_bus_fmt < 0) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "VOP output configured, but no bus format specified,\n"
                    "VOP bus format will depend on SD/HD mode\n"));
        }
        pGeode->tv_bus_fmt = tv_bus_fmt;
        pGeode->tv_601_fmt = tv_601_fmt;
        tv_flags = 0;
        if ((s = xf86GetOptValString(GeodeOptions,
                    LX_OPTION_TV_FLAGS)) != NULL) {
            char *opt, *sp = strdup(s);

            if (sp != NULL) {
                for (opt = strtok(sp, ":"); opt != NULL;
                    opt = strtok(NULL, ":")) {
                    if (xf86NameCmp(opt, "singlechipcompat") == 0)
                        tv_flags |= VOP_FLAG_SINGLECHIPCOMPAT;
                    else if (xf86NameCmp(opt, "extendedsav") == 0)
                        tv_flags |= VOP_FLAG_EXTENDEDSAV;
                    else if (xf86NameCmp(opt, "vbi") == 0)
                        tv_flags |= VOP_FLAG_VBI;
                    else if (xf86NameCmp(opt, "task") == 0)
                        tv_flags |= VOP_FLAG_TASK;
                    else if (xf86NameCmp(opt, "swap_uv") == 0)
                        tv_flags |= VOP_FLAG_SWAP_UV;
                    else if (xf86NameCmp(opt, "swap_vbi") == 0)
                        tv_flags |= VOP_FLAG_SWAP_VBI;
                    else
                        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                                "VOP flag \"%s\" not recognized\n", opt));
                }
                free(sp);
            }
        }
        tv_vsync_shift_count = 0;
        tv_601_flags = 0;
        tv_vsync_shift = VOP_VSYNC_NOSHIFT;
        if ((s = xf86GetOptValString(GeodeOptions,
                    LX_OPTION_TV_601_FLAGS)) != NULL) {
            char *opt, *sp = strdup(s);

            if (sp != NULL) {
                for (opt = strtok(sp, ":"); opt != NULL;
                    opt = strtok(NULL, ":")) {
                    if (xf86NameCmp(opt, "inv_de_pol") == 0)
                        tv_601_flags |= VOP_601_INVERT_DISPE;
                    else if (xf86NameCmp(opt, "inv_hs_pol") == 0)
                        tv_601_flags |= VOP_601_INVERT_HSYNC;
                    else if (xf86NameCmp(opt, "inv_vs_pol") == 0)
                        tv_601_flags |= VOP_601_INVERT_VSYNC;
                    else if (xf86NameCmp(opt, "vsync-4") == 0)
                        tv_vsync_shift = VOP_VSYNC_EARLIER_BY4;
                    else if (xf86NameCmp(opt, "vsync-2") == 0)
                        tv_vsync_shift = VOP_VSYNC_EARLIER_BY2;
                    else if (xf86NameCmp(opt, "vsync+0") == 0)
                        tv_vsync_shift = VOP_VSYNC_NOSHIFT;
                    else if (xf86NameCmp(opt, "vsync+2") == 0) {
                        tv_vsync_shift = VOP_VSYNC_LATER_BY_X;
                        tv_vsync_shift_count = 2;
                    } else if (xf86NameCmp(opt, "vsync+4") == 0) {
                        tv_vsync_shift = VOP_VSYNC_LATER_BY_X;
                        tv_vsync_shift_count = 4;
                    } else
                        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                                "VOP 601_flag \"%s\" not recognized\n", opt));
                }
                free(sp);
            }
        }
        tv_vsync_select = VOP_MB_SYNCSEL_DISABLED;
        if ((s = xf86GetOptValString(GeodeOptions,
                    LX_OPTION_TV_VSYNC_SELECT)) != NULL) {
            char *opt, *sp = strdup(s);

            if (sp != NULL) {
                for (opt = strtok(sp, ":"); opt != NULL;
                    opt = strtok(NULL, ":")) {
                    if (xf86NameCmp(opt, "disabled") == 0)
                        tv_vsync_select = VOP_MB_SYNCSEL_DISABLED;
                    else if (xf86NameCmp(opt, "vg") == 0)
                        tv_vsync_select = VOP_MB_SYNCSEL_VG;
                    else if (xf86NameCmp(opt, "vg_inv") == 0)
                        tv_vsync_select = VOP_MB_SYNCSEL_VG_INV;
                    else if (xf86NameCmp(opt, "statreg17") == 0)
                        tv_vsync_select = VOP_MB_SYNCSEL_STATREG17;
                    else if (xf86NameCmp(opt, "statreg17_inv") == 0)
                        tv_vsync_select = VOP_MB_SYNCSEL_STATREG17_INV;
                    else
                        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                                "VOP vsync_select \"%s\" not "
                                "recognized\n", opt));
                }
                free(sp);
            }
        }
        pGeode->tv_flags = tv_flags;
        pGeode->tv_601_flags = tv_601_flags;
        pGeode->tv_vsync_shift = tv_vsync_shift;
        pGeode->tv_vsync_shift_count = tv_vsync_shift_count;
        pGeode->tv_vsync_select = tv_vsync_select;
        tv_conversion = -1;
        if ((s = xf86GetOptValString(GeodeOptions,
                    LX_OPTION_TV_CONVERSION)) != NULL) {
            if (xf86NameCmp(s, "cosited") == 0)
                tv_conversion = VOP_422MODE_COSITED;
            else if (xf86NameCmp(s, "interspersed") == 0)
                tv_conversion = VOP_422MODE_INTERSPERSED;
            else if (xf86NameCmp(s, "alternating") == 0)
                tv_conversion = VOP_422MODE_ALTERNATING;
            else {
                DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                        "VOP conversion \"%s\" not recognized\n", s));
            }
        }
        if (tv_conversion < 0) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "VOP output configured, but no conversion specified,\n"
                    "VOP conversion will defaults to \"cosited\"\n"));
            tv_conversion = VOP_422MODE_COSITED;
        }
        pGeode->tv_conversion = tv_conversion;
        tvox = tvoy = 0;
        if ((s = xf86GetOptValString(GeodeOptions,
                    LX_OPTION_TV_OVERSCAN)) != NULL) {
            char *opt, *sp = strdup(s);

            if (sp != NULL) {
                if ((opt = strtok(sp, ":")) != NULL)
                    tvox = strtol(opt, NULL, 0);
                if ((opt = strtok(NULL, ":")) != NULL)
                    tvoy = strtol(opt, NULL, 0);
                free(sp);
            }
            DEBUGMSG(1, (0, X_CONFIG, "TVO %d %d\n", tvox, tvoy));
        }
        pGeode->tvox = tvox;
        pGeode->tvoy = tvoy;
    } else if ((pGeode->EnabledOutput & LX_OT_VOP) != 0) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                "VOP output enabled, but not configured, VOP diabled\n"));
        pGeode->EnabledOutput &= ~LX_OT_VOP;
    }

    if ((pGeode->EnabledOutput & LX_OT_CRT) != 0) {
        ddc = LXProbeDDC(pScrni, pGeode->pEnt->index);
    }

    flags = pGeode->EnabledOutput;
    xf86DrvMsg(0, X_INFO, "AMD LX Active Formats -%sCRT,%sVOP,%sFP,%sDRGB\n",
        ((flags & LX_OT_CRT) ? " " : " No "),
        ((flags & LX_OT_VOP) ? " " : " No "),
        ((flags & LX_OT_FP) ? " " : " No "),
        ((flags & LX_OT_DRGB) ? " " : " No "));

    if ((pGeode->EnabledOutput & (LX_OT_CRT | LX_OT_FP | LX_OT_VOP)) == 0) {
        ErrorF("No output enabled !!!\n");
        return FALSE;
    }

    pGeode->ShadowFB = FALSE;
    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_SHADOW_FB, FALSE)) {
        pGeode->ShadowFB = TRUE;
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                "Using \"Shadow Framebuffer\"\n"));
    }

    pGeode->Rotate = 0;
    if ((s = xf86GetOptValString(GeodeOptions, LX_OPTION_ROTATE))) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG, "Rotating - %s\n", s));
        if (!xf86NameCmp(s, "CW")) {
            pGeode->Rotate = 1;
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "Rotating screen clockwise\n"));
        } else if (!xf86NameCmp(s, "INVERT")) {
            pGeode->Rotate = 2;
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "Rotating screen inverted\n"));
        } else if (!xf86NameCmp(s, "CCW")) {
            pGeode->Rotate = 3;
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "Rotating screen counter clockwise\n"));
        }
        if (pGeode->Rotate != 0) {
            pGeode->ShadowFB = TRUE;
        } else {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "\"%s\" is not a valid value for Option \"Rotate\"\n",
                    s));
            DEBUGMSG(1, (pScrni->scrnIndex, X_INFO,
                    "Valid options are \"CW\", \"INVERT\", or \"CCW\"\n"));
        }
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * GeodeProbe(), but check it just in case.
     */
    if (pScrni->chipset == NULL) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                "ChipID 0x%04X is not recognised\n", pGeode->Chipset));
        return FALSE;
    }
    if (pGeode->Chipset < 0) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                "Chipset \"%s\" is not recognised\n", pScrni->chipset));
        return FALSE;
    }

    /*
     * Init the screen with some values
     */
    DEBUGMSG(1, (pScrni->scrnIndex, from,
            "Video I/O registers at 0x%08lX\n",
            (unsigned long)VGAHW_GET_IOBASE()));

    if (pScrni->memPhysBase == 0) {
        from = X_PROBED;
        pScrni->memPhysBase = pGeode->FBLinearAddr;
    }
    pScrni->fbOffset = 0;

    DEBUGMSG(1, (pScrni->scrnIndex, from,
            "Linear framebuffer at 0x%08lX\n",
            (unsigned long)pScrni->memPhysBase));

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrni->maxHValue adn
     * pScrni->maxVValue are set. Since our LXValidMode()
     * already takes care of this, we don't worry about setting them here.
     */
    if (pScrni->depth > 16) {
        PitchInc = 4096;
    } else if (pScrni->depth == 16) {
        PitchInc = 2048;
    } else {
        PitchInc = 1024;
    }
    PitchInc <<= 3;                    /* in bits */

    /* by default use what user sets in the XF86Config file */
    modes = pScrni->display->modes;

    if (ddc != NULL && pScrni->monitor != NULL
        && pScrni->monitor->DDC == NULL) {
        pScrni->monitor->DDC = ddc;
        LXDecodeDDC(pScrni, ddc);
    }

    i = xf86ValidateModes(pScrni, pScrni->monitor->Modes, modes,
        GeodeClockRange, NULL, minPitch, maxPitch,
        PitchInc, minHeight, maxHeight,
        pScrni->display->virtualX,
        pScrni->display->virtualY, pGeode->FBAvail, LOOKUP_BEST_REFRESH);

    DEBUGMSG(1, (pScrni->scrnIndex, from, "xf86ValidateModes:%d %d %d %d\n",
            i, pScrni->virtualX, pScrni->virtualY, pScrni->displayWidth));
    if (i == -1) {
        LXFreeRec(pScrni);
        return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrni);

    if (i == 0 || pScrni->modes == NULL) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR, "No valid modes found\n"));
        LXFreeRec(pScrni);
        return FALSE;
    }

    xf86SetCrtcForModes(pScrni, 0);

    /* Set the current mode to the first in the list */
    pScrni->currentMode = pScrni->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrni);

    /* Set the display resolution */
    xf86SetDpi(pScrni, 0, 0);

    /* Load bpp-specific modules */
    mod = NULL;

#if CFB
    /* Load bpp-specific modules */
    switch (pScrni->bitsPerPixel) {
    case 8:
        mod = "cfb";
        reqSymbol = "cfbScreenInit";
        break;
    case 16:
        mod = "cfb16";
        reqSymbol = "cfb16ScreenInit";
        break;
    case 24:
        mod = "cfb24";
        reqSymbol = "cfb24ScreenInit";
        break;
    case 32:
        mod = "cfb32";
        reqSymbol = "cfb32ScreenInit";
        break;
    default:
        return FALSE;
    }
    if (mod && xf86LoadSubModule(pScrni, mod) == NULL) {
        LXFreeRec(pScrni);
        return FALSE;
    }

    xf86LoaderReqSymbols(reqSymbol, NULL);
#else
    if (xf86LoadSubModule(pScrni, "fb") == NULL) {
        LXFreeRec(pScrni);
        return FALSE;
    }

    xf86LoaderReqSymLists(amdFbSymbols, NULL);
#endif
    if (pGeode->NoAccel == FALSE) {
        if (!xf86LoadSubModule(pScrni, "xaa")) {
            LXFreeRec(pScrni);
            return FALSE;
        }
        xf86LoaderReqSymLists(amdXaaSymbols, NULL);
    }
    if (pGeode->HWCursor == TRUE) {
        if (!xf86LoadSubModule(pScrni, "ramdac")) {
            LXFreeRec(pScrni);
            return FALSE;
        }
        xf86LoaderReqSymLists(amdRamdacSymbols, NULL);
    }
    /* Load shadowfb if needed */
    if (pGeode->ShadowFB) {
        if (!xf86LoadSubModule(pScrni, "shadowfb")) {
            LXFreeRec(pScrni);
            return FALSE;
        }
        xf86LoaderReqSymLists(amdShadowSymbols, NULL);
    }
    if (xf86RegisterResources(pGeode->pEnt->index, NULL, ResExclusive)) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                "xf86RegisterResources() found resource conflicts\n"));
        LXFreeRec(pScrni);
        return FALSE;
    }
    LXUnmapMem(pScrni);

    return TRUE;
}

/*----------------------------------------------------------------------------
 * LXRestore.
 *
 * Description	:This function restores the mode that was saved on server
                 entry
 * Parameters.
 * pScrni 	:Handle to ScreenPtr structure.
 *  Pmode       :poits to screen mode
 * 												
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static void
LXRestore(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(0, (0, X_INFO, "LXRestore\n"));

    if (pGeode->FBVGAActive) {
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        vgaHWProtect(pScrni, TRUE);
        vgaHWRestore(pScrni, &pvgaHW->SavedReg, VGA_SR_ALL);
        vgaHWProtect(pScrni, FALSE);
    }
}

/*----------------------------------------------------------------------------
 * LXCalculatePitchBytes.
 *
 * Description	:This function restores the mode that was saved on server
 *
 * Parameters.
 * pScrni 	:Handle to ScreenPtr structure.
 *    Pmode     :Points to screenmode
 * 									
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static int
LXCalculatePitchBytes(unsigned int width, unsigned int bpp)
{
    int lineDelta = width * (bpp >> 3);

    if (width < 640) {
        /* low resolutions have both pixel and line doubling */
        DEBUGMSG(1, (0, X_PROBED, "lower resolution %d %d\n",
                width, lineDelta));
        lineDelta <<= 1;
    }
    /* needed in Rotate mode when in accel is turned off */

    if (lineDelta > 4096)
        lineDelta = 8192;
    else if (lineDelta > 2048)
        lineDelta = 4096;
    else if (lineDelta > 1024)
        lineDelta = 2048;
    else
        lineDelta = 1024;

    DEBUGMSG(0, (0, X_INFO, "pitch %d %d\n", width, lineDelta));

    return lineDelta;
}

/*----------------------------------------------------------------------------
 * LXGetRefreshRate.
 *
 * Description	:This function restores the mode that saved on server
 *
 * Parameters.
 *     Pmode    :Pointer to the screen modes
 * 												
 * Returns		:It returns the selected refresh rate.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static int
LXGetRefreshRate(DisplayModePtr pMode)
{
#define THRESHOLD 2
    unsigned int i;
    static int validRates[] = { 56, 60, 70, 72, 75, 85, 90, 100 };      /* Hz */
    unsigned long dotClock;
    int refreshRate;
    int selectedRate;

    dotClock = pMode->SynthClock * 1000;
    refreshRate = dotClock / (pMode->CrtcHTotal * pMode->CrtcVTotal);

    if ((pMode->CrtcHTotal < 640) && (pMode->CrtcVTotal < 480))
        refreshRate >>= 2;             /* double pixel and double scan */

    DEBUGMSG(0, (0, X_INFO, "dotclock %lu %d\n", dotClock, refreshRate));

    selectedRate = validRates[0];

    for (i = 0; i < (sizeof(validRates) / sizeof(validRates[0])); i++) {
        if (validRates[i] < (refreshRate + THRESHOLD)) {
            selectedRate = validRates[i];
        }
    }

    return selectedRate;
}

void
lx_clear_screen(ScrnInfoPtr pScrni, int width, int height, int bpp)
{
    /* no accels, mode is not yet set */
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long offset = vg_get_display_offset();
    unsigned long pitch = vg_get_display_pitch();
    unsigned long n = width * ((bpp + 7) >> 3);

    DEBUGMSG(0, (0, X_INFO, "clear screen %lx %d %d %d %lu %lu\n", offset,
            width, height, bpp, pitch, n));
    while (height > 0) {
        memset(pGeode->FBBase + offset, 0, n);
        offset += pitch;
        --height;
    }
}

void
lx_clear_fb(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned char *fb = pGeode->FBBase + pGeode->FBOffset;

    memset(fb, 0, pGeode->FBSize);
    if (pGeode->ShadowPtr != NULL && pGeode->ShadowPtr != fb)
        memset(pGeode->ShadowPtr, 0, pGeode->ShadowSize);
}

static int
lx_set_tv_mode(ScrnInfoPtr pScrni, int tv_mode)
{
    int ret, bpp, flags;
    int tv_conversion, tv_bus_fmt, tv_flags;
    int tv_601_fmt, tv_601_flags;
    int tv_vsync_shift, tv_vsync_shift_count, tv_vsync_select;
    unsigned long src_width, src_height;
    char *bp, *cp, *dp;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    VOPCONFIGURATIONBUFFER vopc;

    bpp = pScrni->bitsPerPixel;
    if (bpp == 32)
        bpp = 24;
    flags = lx_tv_mode_interlaced(tv_mode) != 0 ? VG_MODEFLAG_INTERLACED : 0;
    src_width = src_height = 0;
    ret = vg_set_tv_mode(&src_width, &src_height,
        pGeode->tv_encoder, tv_mode, bpp, flags, 0, 0);
    DEBUGMSG(1, (0, X_INFO,
            "Setting TV mode %lux%lu encoder=%d,bpp=%d,flags=%d,overscan "
            "%d,%d\n", src_width, src_height, pGeode->tv_encoder, bpp,
            flags, pGeode->tvox, pGeode->tvoy));
    ret = vg_set_tv_mode(&src_width, &src_height,
        pGeode->tv_encoder, tv_mode, bpp, flags, pGeode->tvox, pGeode->tvoy);

    DEBUGMSG(1, (0, X_INFO, "Set TV mode ret=%d\n", ret));
    if (ret == 0) {
        memset(&vopc, 0, sizeof(vopc));
        tv_flags = pGeode->tv_flags;
        tv_bus_fmt = pGeode->tv_bus_fmt;
        tv_601_fmt = pGeode->tv_601_fmt;
        tv_601_flags = pGeode->tv_601_flags;
        tv_vsync_shift = pGeode->tv_vsync_shift;
        tv_vsync_shift_count = pGeode->tv_vsync_shift_count;
        tv_vsync_select = pGeode->tv_vsync_select;
        tv_conversion = pGeode->tv_conversion;
        if (tv_bus_fmt < 0) {
            dp = "defaults";
            switch (tv_mode) {
            case VG_TVMODE_NTSC:
            case VG_TVMODE_6X4_NTSC:
            case VG_TVMODE_8X6_NTSC:
            case VG_TVMODE_10X7_NTSC:
            case VG_TVMODE_PAL:
            case VG_TVMODE_6X4_PAL:
            case VG_TVMODE_8X6_PAL:
            case VG_TVMODE_10X7_PAL:
                tv_bus_fmt = VOP_MODE_VIP11;
                break;
            default:
                tv_bus_fmt = VOP_MODE_VIP20_16BIT;
                break;
            }
        } else
            dp = "set";
        switch (tv_bus_fmt) {
        case VOP_MODE_VIP11:
            bp = "vop11";
            break;
        case VOP_MODE_CCIR656:
            bp = "ccir656";
            break;
        case VOP_MODE_VIP20_8BIT:
            bp = "vip20_8bit";
            break;
        case VOP_MODE_VIP20_16BIT:
            bp = "vip20_16bit";
            break;
        case VOP_MODE_601:
            switch (tv_601_fmt) {
            default:
                tv_601_fmt = VOP_601_YUV_8BIT;
            case VOP_601_YUV_8BIT:
                bp = "601_yuv_8bit";
                break;
            case VOP_601_YUV_16BIT:
                bp = "601_yuv_16bit";
                break;
            case VOP_601_RGB_8_8_8:
                bp = "601_rgb_8_8_8";
                break;
            case VOP_601_YUV_4_4_4:
                bp = "601_yuv_4_4_4";
                break;
            }
            break;
        default:
            tv_bus_fmt = VOP_MODE_DISABLED;
        case VOP_MODE_DISABLED:
            bp = "disabled";
            break;
        }
        switch (tv_conversion) {
        default:
            tv_conversion = VOP_422MODE_COSITED;
        case VOP_422MODE_COSITED:
            cp = "cosited";
            break;
        case VOP_422MODE_INTERSPERSED:
            cp = "interspersed";
            break;
        case VOP_422MODE_ALTERNATING:
            cp = "alternating";
            break;
        }
        vopc.flags = tv_flags;
        vopc.mode = tv_bus_fmt;
        vopc.conversion_mode = tv_conversion;
        vopc.vsync_out = tv_vsync_select;
        vopc.vop601.flags = tv_601_flags;
        vopc.vop601.vsync_shift = tv_vsync_shift;
        vopc.vop601.vsync_shift_count = tv_vsync_shift_count;
        vopc.vop601.output_mode = tv_601_fmt;
        DEBUGMSG(1, (0, X_INFO,
                "Set TV mode %s to %s, conv %s, flags %x\n",
                dp, bp, cp, tv_flags));
        DEBUGMSG(1, (0, X_INFO,
                "Set TV 601 mode %x flags %x vsync shift %x/%x\n",
                tv_601_fmt, tv_601_flags, tv_vsync_shift,
                tv_vsync_shift_count));
        vop_set_configuration(&vopc);
    }

    return ret;
}

static int
lx_set_custom_mode(unsigned long bpp, unsigned long flags,
    unsigned long hactive, unsigned long hblankstart,
    unsigned long hsyncstart, unsigned long hsyncend,
    unsigned long hblankend, unsigned long htotal,
    unsigned long vactive, unsigned long vblankstart,
    unsigned long vsyncstart, unsigned long vsyncend,
    unsigned long vblankend, unsigned long vtotal, unsigned long frequency)
{
    VG_DISPLAY_MODE mode;

    memset(&mode, 0, sizeof(mode));
    mode.flags = flags;
    mode.src_width = hactive;
    mode.src_height = vactive;
    mode.mode_width = hactive;
    mode.mode_height = vactive;
    mode.hactive = hactive;
    mode.hblankstart = hblankstart;
    mode.hsyncstart = hsyncstart;
    mode.hsyncend = hsyncend;
    mode.hblankend = hblankend;
    mode.htotal = htotal;
    mode.vactive = vactive;
    mode.vblankstart = vblankstart;
    mode.vsyncstart = vsyncstart;
    mode.vsyncend = vsyncend;
    mode.vblankend = vblankend;
    mode.vtotal = vtotal;
    mode.vactive_even = vactive;
    mode.vblankstart_even = vblankstart;
    mode.vsyncstart_even = vsyncstart;
    mode.vsyncend_even = vsyncend;
    mode.vblankend_even = vblankend;
    mode.vtotal_even = vtotal;
    mode.frequency = frequency;

    return vg_set_custom_mode(&mode, bpp);
}

/*----------------------------------------------------------------------------
 * LXSetMode.
 *
 * Description	:This function sets parametrs for screen mode
 *
 * Parameters.
 * pScrni 	:Pointer to the screenInfo structure.
 *	 Pmode      :Pointer to the screen modes
 * 												
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/

static Bool
LXSetMode(ScrnInfoPtr pScrni, DisplayModePtr pMode)
{
    int bpp, bppx, rate, video_enable, tv_mode, opath;
    unsigned long flags, srcw, srch, actw, acth, dstw, dsth, video_flags;

    GeodeRec *pGeode = GEODEPTR(pScrni);
    DF_VIDEO_SOURCE_PARAMS vs_odd, vs_even;

    gp_wait_until_idle();
    /* disable video */
    df_get_video_enable(&video_enable, &video_flags);
    if (video_enable != 0)
        df_set_video_enable(0, 0);
    df_get_video_source_configuration(&vs_odd, &vs_even);
    lx_disable_dac_power(pScrni, DF_CRT_DISABLE);

    DEBUGMSG(1, (0, X_NONE, "LXSetMode! %p %p %p\n",
            cim_gp_ptr, cim_vid_ptr, cim_fb_ptr));

    /* Set the VT semaphore */
    pScrni->vtSema = TRUE;

    srcw = pMode->CrtcHDisplay;
    srch = pMode->CrtcVDisplay;
    bpp = pScrni->bitsPerPixel;
    rate = LXGetRefreshRate(pMode);
    /* otherwise color/chroma keying doesnt work */
    bppx = bpp == 32 ? 24 : bpp;

    /* The timing will be adjusted later */
    DEBUGMSG(1, (0, X_PROBED,
            "Setting mode %dx%d %0.3f  %d %d %d %d  %d %d %d %d\n",
            pMode->CrtcHDisplay, pMode->CrtcVDisplay,
            pMode->SynthClock / 1000.0, pMode->CrtcHDisplay,
            pMode->CrtcHSyncStart, pMode->CrtcHSyncEnd, pMode->CrtcHTotal,
            pMode->CrtcVDisplay, pMode->CrtcVSyncStart, pMode->CrtcVSyncEnd,
            pMode->CrtcVTotal));
    DEBUGMSG(1, (0, X_INFO,
            "Set display mode: %lux%lu-%d (%dHz) Pitch %d/%d\n", srcw, srch,
            bpp, rate, pGeode->Pitch, pGeode->AccelPitch));

    opath = DF_DISPLAY_CRT;
    if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
        if ((pGeode->EnabledOutput & LX_OT_CRT) != 0)
            opath = DF_DISPLAY_CRT_FP;
        else
            opath = DF_DISPLAY_FP;
    }

    if (pGeode->TVSupport && (tv_mode = lx_tv_mode(pMode)) >= 0) {
        DEBUGMSG(1, (0, X_INFO, "Set TV mode %d\n", tv_mode));
        lx_set_tv_mode(pScrni, tv_mode);
        opath = DF_DISPLAY_VOP;
    } else if (pGeode->CustomMode != 0) {
        DEBUGMSG(1, (0, X_PROBED, "Setting Custom mode\n"));
        flags = 0;
        if ((pMode->Flags & V_NHSYNC) != 0)
            flags |= VG_MODEFLAG_NEG_HSYNC;
        if ((pMode->Flags & V_NVSYNC) != 0)
            flags |= VG_MODEFLAG_NEG_VSYNC;
        lx_set_custom_mode(bppx, flags, pMode->CrtcHDisplay,
            pMode->CrtcHBlankStart, pMode->CrtcHSyncStart,
            pMode->CrtcHSyncEnd, pMode->CrtcHBlankEnd,
            pMode->CrtcHTotal, pMode->CrtcVDisplay,
            pMode->CrtcVBlankStart, pMode->CrtcVSyncStart,
            pMode->CrtcVSyncEnd, pMode->CrtcVBlankEnd,
            pMode->CrtcVTotal, (int)((pMode->SynthClock / 1000.0) * 0x10000));
    } else if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
        /* display is fp */
        actw = pGeode->FPGeomActX;
        dstw = pGeode->FPGeomDstX;
        acth = pGeode->FPGeomActY;
        dsth = pGeode->FPGeomDstY;
        flags = (pGeode->EnabledOutput & LX_OT_CRT) !=
            0 ? VG_MODEFLAG_CRT_AND_FP : 0;
        /* cant do scaling if width > 1024 (hw bfr size limitation) */
        if (srcw > 1024) {
            if (srcw != actw)
                DEBUGMSG(1, (0, X_PROBED,
                        "FPGeomSrcX > 1024, scaling disabled\n"));
            actw = srcw;
            acth = srch;
            vg_set_border_color(0);
        }
        DEBUGMSG(1, (0, X_PROBED,
                "Setting Display for TFT %lux%lu %lux%lu %lux%lu %d\n", srcw,
                srch, actw, acth, dstw, dsth, pScrni->bitsPerPixel));
        vg_set_panel_mode(srcw, srch, actw, acth, dstw, dsth, bppx, flags);
    } else {
        /* display is crt */
        DEBUGMSG(1, (0, X_PROBED, "Setting Display for CRT %lux%lu-%d@%d\n",
                srcw, srch, bppx, LXGetRefreshRate(pMode)));
        vg_set_display_mode(srcw, srch, srcw, srch, bppx,
            LXGetRefreshRate(pMode), 0);
    }

    df_set_output_path(opath);
    vg_set_display_pitch(pGeode->Pitch);
    gp_set_bpp(pScrni->bitsPerPixel);

    vg_set_display_offset(0L);
    vg_wait_vertical_blank();

    DEBUGMSG(1, (0, X_PROBED, "Display mode set\n"));
    /* enable compression if option selected */
    if (pGeode->Compression != 0) {
        DEBUGMSG(1, (0, X_PROBED, "Compression mode set %d\n",
                pGeode->Compression));
        /* set the compression parameters,and it will be turned on later. */
        vg_configure_compression(&(pGeode->CBData));

        /* set the compression buffer, all parameters already set */
        vg_set_compression_enable(1);
    }

    if (pGeode->HWCursor != 0) {
        VG_PANNING_COORDINATES panning;

        /* Load blank cursor */
        LXLoadCursorImage(pScrni, NULL);
        vg_set_cursor_position(0, 0, &panning);
        LXShowCursor(pScrni);
    }

    DEBUGMSG(1, (0, X_INFO, "setting mode done.\n"));

    vg_set_display_offset(pGeode->PrevDisplayOffset);

    /* Restore the contents in the screen info */
    DEBUGMSG(1, (0, X_INFO, "After setting the mode\n"));
    switch (pGeode->Rotate) {
    case 1:
    case 3:
        pGeode->HDisplay = pMode->VDisplay;
        pGeode->VDisplay = pMode->HDisplay;
        break;
    default:
        pGeode->HDisplay = pMode->HDisplay;
        pGeode->VDisplay = pMode->VDisplay;
        break;
    }

    df_configure_video_source(&vs_odd, &vs_even);
    if (video_enable != 0)
        df_set_video_enable(video_enable, video_flags);
    lx_enable_dac_power(pScrni, 1);

    return TRUE;
}

/*----------------------------------------------------------------------------
 * LXEnterGraphics.
 *
 * Description	:This function will intiallize the displaytiming
				 structure for nextmode and switch to VGA mode.
 *
 * Parameters.
 *    pScrn   :Screen information will be stored in this structure.
 * 	pScrni :Pointer to the screenInfo structure.
 *													
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments     :gfx_vga_mode_switch() will start and end the
 *				switching based on the arguments 0 or 1.soft_vga
 *				is disabled in this function.
*----------------------------------------------------------------------------
*/
static Bool
LXEnterGraphics(ScreenPtr pScrn, ScrnInfoPtr pScrni)
{
    int bpp;
    unsigned long cmd_bfr_phys;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

    DEBUGMSG(1, (0, X_INFO, "LXEnterGraphics.\n"));

    gp_wait_until_idle();
    cmd_bfr_phys =
        pGeode->InitBaseAddress.framebuffer_base + pGeode->CmdBfrOffset;
    cim_cmd_base_ptr = cim_fb_ptr + pGeode->CmdBfrOffset;
    gp_set_frame_buffer_base(pGeode->InitBaseAddress.framebuffer_base,
        pGeode->FBTop);
    gp_set_command_buffer_base(cmd_bfr_phys, 0, pGeode->CmdBfrSize);

    lx_disable_dac_power(pScrni, DF_CRT_DISABLE);
    /* Save CRT State */
    vg_get_current_display_mode(&pGeode->FBcimdisplaytiming.vgDisplayMode,
        &bpp);

    pGeode->FBcimdisplaytiming.wBpp = bpp;
    pGeode->FBcimdisplaytiming.wPitch = vg_get_display_pitch();

    /* Save Display offset */
    pGeode->FBDisplayOffset = vg_get_display_offset();
    pGeode->FBBIOSMode = pvgaHW->readCrtc(pvgaHW, 0x040);
    DEBUGMSG(1, (0, X_INFO, "FBBIOSMode %d\n", pGeode->FBBIOSMode));

    /* Save the current Compression state */
    pGeode->FBCompressionEnable = vg_get_compression_enable();

    vg_get_compression_info(&(pGeode->FBCBData));

    /* Save Cursor offset */
    vg_get_cursor_info(&pGeode->FBCursor);

    /* only if comming from VGA */
    if (pGeode->FBVGAActive) {
        unsigned short sequencer;
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        /* Map VGA aperture */
        if (!vgaHWMapMem(pScrni))
            return FALSE;

        /* Unlock VGA registers */
        vgaHWUnlock(pvgaHW);

        /* Save the current state and setup the current mode */
        vgaHWSave(pScrni, &VGAHWPTR(pScrni)->SavedReg, VGA_SR_ALL);

        /* DISABLE VGA SEQUENCER */
        /* This allows the VGA state machine to terminate. We must delay */
        /* such that there are no pending MBUS requests.  */

        cim_outb(DC3_SEQUENCER_INDEX, DC3_SEQUENCER_CLK_MODE);
        sequencer = cim_inb(DC3_SEQUENCER_DATA);
        sequencer |= DC3_CLK_MODE_SCREEN_OFF;
        cim_outb(DC3_SEQUENCER_DATA, sequencer);

        vg_delay_milliseconds(1);

        /* BLANK THE VGA DISPLAY */
        cim_outw(DC3_SEQUENCER_INDEX, DC3_SEQUENCER_RESET);
        sequencer = cim_inb(DC3_SEQUENCER_DATA);
        sequencer &= ~DC3_RESET_VGA_DISP_ENABLE;
        cim_outb(DC3_SEQUENCER_DATA, sequencer);

        vg_delay_milliseconds(1);
    }

    lx_clear_fb(pScrni);

    if (!LXSetMode(pScrni, pScrni->currentMode)) {
        return FALSE;
    }

    lx_enable_dac_power(pScrni, 1);
    return TRUE;
}

void
lx_disable_dac_power(ScrnInfoPtr pScrni, int option)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if ((pGeode->EnabledOutput & LX_OT_FP) != 0)
        df_set_panel_enable(0);
    if ((pGeode->EnabledOutput & LX_OT_CRT) != 0) {
        if ((pGeode->EnabledOutput & LX_OT_FP) != 0)
            /* wait for the panel to be fully powered off */
            while ((READ_VID32(DF_POWER_MANAGEMENT) & 2) == 0) ;
        df_set_crt_enable(option);
    }
}

void
lx_enable_dac_power(ScrnInfoPtr pScrni, int option)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    df_set_crt_enable(DF_CRT_ENABLE);
    if (option != 0 && (pGeode->EnabledOutput & LX_OT_CRT) == 0) {
        unsigned int misc = READ_VID32(DF_VID_MISC);

        misc |= DF_DAC_POWER_DOWN;
        WRITE_VID32(DF_VID_MISC, misc);
    }
    if ((pGeode->EnabledOutput & LX_OT_FP) != 0)
        df_set_panel_enable(1);
}

/*----------------------------------------------------------------------------
 * LXLeaveGraphics:
 *
 * Description	:This function will restore the displaymode parameters
 * 				 and switches the VGA mode
 *
 * Parameters.
 *    pScrni   :Pointer to the screenInfo structure.
 * 												
 * Returns		:none.
*----------------------------------------------------------------------------
*/
static void
LXLeaveGraphics(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    VG_PANNING_COORDINATES panning;

    gp_wait_until_idle();

    /* Restore VG registers */
    lx_disable_dac_power(pScrni, DF_CRT_DISABLE);
    vg_set_custom_mode(&(pGeode->FBcimdisplaytiming.vgDisplayMode),
        pGeode->FBcimdisplaytiming.wBpp);

    vg_set_compression_enable(0);

    /* Restore the previous Compression state */
    if (pGeode->FBCompressionEnable) {
        vg_configure_compression(&(pGeode->FBCBData));
        vg_set_compression_enable(1);
    }

    vg_set_display_pitch(pGeode->FBcimdisplaytiming.wPitch);
    vg_set_display_offset(pGeode->FBDisplayOffset);

    /* Restore Cursor */
    vg_set_cursor_position(pGeode->FBCursor.cursor_x,
        pGeode->FBCursor.cursor_y, &panning);

    /* For the moment, always do an int 10 */

#if INT10_SUPPORT
    pGeode->vesa->pInt->num = 0x10;
    pGeode->vesa->pInt->ax = 0x0 | pGeode->FBBIOSMode;
    pGeode->vesa->pInt->bx = 0;
    xf86ExecX86int10(pGeode->vesa->pInt);
#endif
    vg_delay_milliseconds(3);
    LXRestore(pScrni);

    lx_enable_dac_power(pScrni, 0);
}

/*----------------------------------------------------------------------------
 * LXCloseScreen.
 *
 * Description	:This function will restore the original mode
 *				 and also it unmap video memory
 *
 * Parameters.
 *    ScrnIndex	:Screen index value of the screen will be closed.
 * 	pScrn    	:Pointer to the screen structure.
 *	
 * 												
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments		:none.
*----------------------------------------------------------------------------
*/
static Bool
LXCloseScreen(int scrnIndex, ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->ShadowPtr && !pGeode->ShadowInFBMem)
        xfree(pGeode->ShadowPtr);

    DEBUGMSG(1, (scrnIndex, X_PROBED, "LXCloseScreen %d\n", pScrni->vtSema));

    if (pScrni->vtSema)
        LXLeaveGraphics(pScrni);

    if (pGeode->AccelInfoRec)
        XAADestroyInfoRec(pGeode->AccelInfoRec);

    if (pGeode->AccelImageWriteBuffers) {
#if LX_USE_OFFSCRN_MEM
        xfree(pGeode->AccelImageWriteBuffers[0]);
#endif
        xfree(pGeode->AccelImageWriteBuffers);
        pGeode->AccelImageWriteBuffers = NULL;
    }

    if (pGeode->AccelColorExpandBuffers) {
        xfree(pGeode->AccelColorExpandBuffers[0]);
        xfree(pGeode->AccelColorExpandBuffers);
        pGeode->AccelColorExpandBuffers = NULL;
    }
    pScrni->vtSema = FALSE;

    LXUnmapMem(pScrni);

    if (pGeode && (pScrn->CloseScreen = pGeode->CloseScreen)) {
        pGeode->CloseScreen = NULL;
        return ((*pScrn->CloseScreen) (scrnIndex, pScrn));
    }
    return TRUE;
}

#ifdef DPMSExtension
/*----------------------------------------------------------------------------
 * LXDPMSSet.
 *
 * Description	:This function sets geode into Power Management
 *               Signalling mode.				
 *
 * Parameters.
 * 	pScrni	 :Pointer to screen info strucrure.
 * 	mode         :Specifies the power management mode.
 *	 												
 * Returns		 :none.
 *
 * Comments      :none.
*----------------------------------------------------------------------------
*/
static void
LXDPMSSet(ScrnInfoPtr pScrni, int mode, int flags)
{
    GeodeRec *pGeode;

    pGeode = GEODEPTR(pScrni);

    DEBUGMSG(1, (0, X_INFO, "LXDPMSSet!\n"));

    /* Check if we are actively controlling the display */
    if (!pScrni->vtSema) {
        ErrorF("LXDPMSSet called when we not controlling the VT!\n");
        return;
    }
    switch (mode) {
    case DPMSModeOn:                  /* Screen: On; HSync: On; VSync: On */
        lx_enable_dac_power(pScrni, 1);
        break;

    case DPMSModeStandby:             /* Screen: Off; HSync: Off; VSync: On */
        lx_disable_dac_power(pScrni, DF_CRT_STANDBY);
        break;

    case DPMSModeSuspend:             /* Screen: Off; HSync: On; VSync: Off */
        lx_disable_dac_power(pScrni, DF_CRT_SUSPEND);
        break;

    case DPMSModeOff:                 /* Screen: Off; HSync: Off; VSync: Off */
        lx_disable_dac_power(pScrni, DF_CRT_DISABLE);
        break;
    }
}
#endif

/*----------------------------------------------------------------------------
 * LXScreenInit.
 *
 * Description	:This function will be called at the each ofserver
 *   			 generation.				
 *
 * Parameters.
 *   scrnIndex   :Specfies the screenindex value during generation.
 *    pScrn	 :Pointer to screen strucrure.
 * 	argc         :parameters for command line arguments count
 *	argv         :command line arguments if any it is not used.  												
 *
 * Returns		 :none.
 *
 * Comments      :none.
*----------------------------------------------------------------------------
*/
static Bool
LXScreenInit(int scrnIndex, ScreenPtr pScrn, int argc, char **argv)
{
    int i, bytpp, size, fbsize, fboffset, fbavail;
    int pitch, displayWidth, virtualX, virtualY;
    int HDisplay, VDisplay, maxHDisplay, maxVDisplay, maxX, maxY;
    unsigned char *FBStart, **ap, *bp;
    DisplayModePtr p;
    GeodeRec *pGeode;
    VisualPtr visual;
    BoxRec AvailBox;
    RegionRec OffscreenRegion;
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
    Bool Inited = FALSE;

    DEBUGMSG(1, (0, X_INFO, "LXScreenInit!\n"));
    /* Get driver private */
    pGeode = LXGetRec(pScrni);
    DEBUGMSG(1, (0, X_INFO, "LXScreenInit(0)!\n"));

    /*
     * Allocate a vgaHWRec
     */

    if (!vgaHWGetHWRec(pScrni))
        return FALSE;
    if (!vgaHWMapMem(pScrni))
        return FALSE;

    vgaHWGetIOBase(VGAHWPTR(pScrni));

    if (!LXMapMem(pScrni))
        return FALSE;

    pGeode->Pitch = LXCalculatePitchBytes(pScrni->virtualX,
        pScrni->bitsPerPixel);
    pGeode->AccelPitch = pGeode->Pitch;
    bytpp = (pScrni->bitsPerPixel + 7) / 8;

    /* start of framebuffer for accels */
    fboffset = 0;
    fbavail = pGeode->FBAvail;

    /* allocate display frame buffer at zero offset */
    fbsize = pScrni->virtualY * pGeode->Pitch;
    pGeode->FBSize = fbsize;

    pGeode->CursorSize = (HW_CURSOR_W * HW_CURSOR_H) / 8 * 2;
    pGeode->CursorStartOffset = 0;

    DEBUGMSG(1, (scrnIndex, X_PROBED, "%d %d %d\n",
            pScrni->virtualX, pScrni->bitsPerPixel, pGeode->Pitch));

    HDisplay = pScrni->currentMode->HDisplay;
    VDisplay = pScrni->currentMode->VDisplay;
    pGeode->orig_virtX = pScrni->virtualX;
    pGeode->orig_virtY = pScrni->virtualY;

    p = pScrni->modes;
    maxHDisplay = p->HDisplay;
    maxVDisplay = p->VDisplay;
    while ((p = p->next) != pScrni->modes) {
        if (maxHDisplay < p->HDisplay)
            maxHDisplay = p->HDisplay;
        if (maxVDisplay < p->VDisplay)
            maxVDisplay = p->VDisplay;
    }
    DEBUGMSG(1, (scrnIndex, X_PROBED, "maxHDisplay %d maxVDisplay %d\n",
            maxHDisplay, maxVDisplay));

    switch (pGeode->Rotate) {
    case 1:
    case 3:
        pGeode->HDisplay = VDisplay;
        pGeode->VDisplay = HDisplay;
        virtualX = pScrni->virtualY;
        virtualY = pScrni->virtualX;
        maxX = maxVDisplay;
        maxY = maxHDisplay;
        break;
    default:
        pGeode->HDisplay = HDisplay;
        pGeode->VDisplay = VDisplay;
        virtualX = pScrni->virtualX;
        virtualY = pScrni->virtualY;
        maxX = maxHDisplay;
        maxY = maxVDisplay;
        break;
    }

    /* shadow may be first in FB, since accels render there */

    pGeode->ShadowPtr = NULL;
    if (pGeode->ShadowFB) {
        if (!pGeode->PointerMoved) {
            pGeode->PointerMoved = pScrni->PointerMoved;
            pScrni->PointerMoved = LXPointerMoved;
        }
        if (!pGeode->NoAccel) {
            pGeode->ShadowPitch =
                LXCalculatePitchBytes(virtualX, pScrni->bitsPerPixel);
            size = pGeode->ShadowPitch * virtualY;
            if (size <= fbavail - fbsize) {
                pGeode->ShadowPtr =
                    (unsigned char *)pGeode->FBBase + fboffset;
                pGeode->AccelPitch = pGeode->ShadowPitch;
                pGeode->ShadowSize = size;
                pGeode->ShadowInFBMem = TRUE;
                fboffset += size;
                fbavail -= size;
            } else {
                xf86DrvMsg(scrnIndex, X_ERROR,
                    "Shadow FB, No FB Memory, trying offscreen\n");
            }
        }
        if (pGeode->ShadowPtr == NULL) {
            pGeode->ShadowPitch =
                BitmapBytePad(pScrni->bitsPerPixel * virtualX);
            size = pGeode->ShadowPitch * virtualY;
            pGeode->ShadowPtr = xalloc(size);
            if (pGeode->ShadowPtr != NULL) {
                pGeode->ShadowSize = size;
                pGeode->ShadowInFBMem = FALSE;
                if (!pGeode->NoAccel) {
                    pGeode->NoAccel = TRUE;
                    pGeode->HWCursor = FALSE;
                    xf86DrvMsg(scrnIndex, X_ERROR,
                        "Shadow FB offscreen, All Accels disabled\n");
                }
            } else {
                xf86DrvMsg(scrnIndex, X_ERROR,
                    "Shadow FB, No offscreen Memory, disabled\n");
                pGeode->ShadowFB = FALSE;
                pGeode->Rotate = 0;
                pGeode->HDisplay = HDisplay;
                pGeode->VDisplay = VDisplay;
                virtualX = pScrni->virtualX;
                virtualY = pScrni->virtualY;
            }
        }
    }

    if (pGeode->ShadowPtr != NULL) {
        displayWidth = pGeode->ShadowPitch / bytpp;
        FBStart = pGeode->ShadowPtr;
        DEBUGMSG(1, (0, X_PROBED, "Shadow %p \n", FBStart));
    } else {
        displayWidth = pGeode->Pitch / bytpp;
        FBStart = pGeode->FBBase;
        DEBUGMSG(1, (0, X_PROBED, "FBStart %p \n", FBStart));
    }

    DEBUGMSG(1, (0, X_PROBED, "FB display %X size %X \n", fboffset, fbsize));
    pGeode->FBOffset = fboffset;       /* offset of display framebuffer */
    pScrni->fbOffset = fboffset;
    fboffset += fbsize;
    fbavail -= fbsize;

    if (pGeode->Compression) {         /* Compression enabled */
        pGeode->CBData.size = 512 + 32;
        pGeode->CBData.pitch = 512 + 32;
        size = maxY * pGeode->CBData.pitch;
        DEBUGMSG(1, (0, X_PROBED, "CB %#x size %#x (%d*%lu)\n", fboffset,
                size, maxY, pGeode->CBData.pitch));
        if (size <= fbavail) {
            pGeode->CBData.compression_offset = fboffset;
            fboffset += size;
            fbavail -= size;
        } else {
            xf86DrvMsg(scrnIndex, X_ERROR,
                "Compression, No FB Memory, disabled\n");
            pGeode->Compression = FALSE;
        }
    }

    if (pGeode->HWCursor) {            /* HWCursor enabled */
        size = pGeode->CursorSize;
        if (size <= fbavail) {
            pGeode->CursorStartOffset = fboffset;
            fboffset += size;
            fbavail -= size;
        } else {
            xf86DrvMsg(scrnIndex, X_ERROR,
                "HWCursor, No FB Memory, disabled\n");
            pGeode->HWCursor = FALSE;
        }
    }

    if (!pGeode->NoAccel) {            /* Acceleration enabled */
        if (pGeode->NoOfImgBuffers > 0) {
            pGeode->AccelImageWriteBuffers = NULL;
            pitch = pGeode->AccelPitch;
            size = pitch * pGeode->NoOfImgBuffers;
#if !LX_USE_OFFSCRN_MEM
            if (size <= fbavail) {
                bp = (unsigned char *)pGeode->FBBase + fboffset;
                ap = xalloc(sizeof(pGeode->AccelImageWriteBuffers[0]) *
                    pGeode->NoOfImgBuffers);
                if (ap != NULL) {
                    for (i = 0; i < pGeode->NoOfImgBuffers; ++i) {
                        ap[i] = bp;
                        bp += pitch;
                    }
                    pGeode->AccelImageWriteBuffers = ap;
                    fboffset += size;
                    fbavail -= size;
                } else {
                    xf86DrvMsg(scrnIndex, X_ERROR,
                        "Image Write, No Memory\n");
                }
            } else {
                xf86DrvMsg(scrnIndex, X_ERROR, "Image Write, No FB Memory\n");
            }
#else
            if ((bp = (unsigned char *)xalloc(size)) != NULL) {
                ap = xalloc(sizeof(pGeode->AccelImageWriteBuffers[0]) *
                    pGeode->NoOfImgBuffers);
                if (ap != NULL) {
                    for (i = 0; i < pGeode->NoOfImgBuffers; ++i) {
                        ap[i] = bp;
                        bp += pitch;
                    }
                    pGeode->AccelImageWriteBuffers = ap;
                } else {
                    xf86DrvMsg(scrnIndex, X_ERROR,
                        "Image Write, No Memory\n");
                }
            } else {
                xf86DrvMsg(scrnIndex, X_ERROR,
                    "Image Write, No offscreen Memory\n");
            }
#endif
            if (pGeode->AccelImageWriteBuffers == NULL) {
                xf86DrvMsg(scrnIndex, X_ERROR,
                    "Accel Image Write disabled\n");
                pGeode->NoOfImgBuffers = 0;
            }
        }

        if (pGeode->NoOfColorExpandLines > 0) {
            pGeode->AccelColorExpandBuffers = NULL;
            pitch = ((pGeode->AccelPitch + 31) >> 5) << 2;
            size = pitch * pGeode->NoOfColorExpandLines;
            if ((bp = (unsigned char *)xalloc(size)) != NULL) {
                ap = xalloc(sizeof(pGeode->AccelColorExpandBuffers[0]) *
                    pGeode->NoOfColorExpandLines);
                if (ap != NULL) {
                    for (i = 0; i < pGeode->NoOfColorExpandLines; ++i) {
                        ap[i] = bp;
                        bp += pitch;
                    }
                    pGeode->AccelColorExpandBuffers = ap;
                } else {
                    xf86DrvMsg(scrnIndex, X_ERROR,
                        "Color Expansion, No Memory\n");
                }
            } else {
                xf86DrvMsg(scrnIndex, X_ERROR,
                    "Color Expansion, No offscreen Memory\n");
            }
            if (pGeode->AccelColorExpandBuffers == NULL) {
                xf86DrvMsg(scrnIndex, X_ERROR,
                    "Accel Color Expansion disabled\n");
                pGeode->NoOfColorExpandLines = 0;
            }
        }
    } else {
        pGeode->NoOfImgBuffers = 0;
        pGeode->AccelImageWriteBuffers = NULL;
        pGeode->NoOfColorExpandLines = 0;
        pGeode->AccelColorExpandBuffers = NULL;
    }

    /* Initialise graphics mode */
    if (!LXEnterGraphics(pScrn, pScrni))
        return FALSE;

    pScrni->virtualX = virtualX;
    pScrni->virtualY = virtualY;

    /* Reset visual list */
    miClearVisualTypes();

    /* Setup the visual we support */
    if (pScrni->bitsPerPixel > 8) {
        DEBUGMSG(1, (scrnIndex, X_PROBED,
                "miSetVisualTypes %d %X %X %X\n",
                pScrni->depth,
                TrueColorMask, pScrni->rgbBits, pScrni->defaultVisual));

        if (!miSetVisualTypes(pScrni->depth,
                TrueColorMask, pScrni->rgbBits, pScrni->defaultVisual)) {
            return FALSE;
        }
    } else {
        if (!miSetVisualTypes(pScrni->depth,
                miGetDefaultVisualMask(pScrni->depth),
                pScrni->rgbBits, pScrni->defaultVisual)) {
            return FALSE;
        }
    }

    /* Set for RENDER extensions */
    miSetPixmapDepths();

    /* Call the framebuffer layer's ScreenInit function, and fill in other
     * pScrn fields.
     */
    switch (pScrni->bitsPerPixel) {
#if CFB
    case 8:
        Inited = cfbScreenInit(pScrn, FBStart, virtualX, virtualY,
            pScrni->xDpi, pScrni->yDpi, displayWidth);
        break;
    case 16:
        Inited = cfb16ScreenInit(pScrn, FBStart, virtualX, virtualY,
            pScrni->xDpi, pScrni->yDpi, displayWidth);
        break;
    case 24:
    case 32:
        Inited = cfb32ScreenInit(pScrn, FBStart, virtualX, virtualY,
            pScrni->xDpi, pScrni->yDpi, displayWidth);
        break;
#else
    case 8:
    case 16:
    case 24:
    case 32:
        Inited = fbScreenInit(pScrn, FBStart, virtualX, virtualY,
            pScrni->xDpi, pScrni->yDpi, displayWidth, pScrni->bitsPerPixel);
        break;
#endif
    default:
        xf86DrvMsg(scrnIndex, X_ERROR,
            "Internal error: invalid bpp (%d) in ScreenInit\n",
            pScrni->bitsPerPixel);
        Inited = FALSE;
        break;
    }

    if (!Inited)
        return FALSE;

    LXRotationInit(pScrni);
    LXAdjustFrame(scrnIndex, pScrni->frameX0, pScrni->frameY0, 0);

    /* SET UP GRAPHICS MEMORY AVAILABLE FOR PIXMAP CACHE */
    AvailBox.x1 = 0;
    AvailBox.y1 = (fboffset + pGeode->AccelPitch - 1) / pGeode->AccelPitch;
    AvailBox.x2 = displayWidth;
    AvailBox.y2 = (pGeode->FBAvail - pGeode->AccelPitch + 1) /
        pGeode->AccelPitch;

    if (AvailBox.y1 < AvailBox.y2) {
        xf86DrvMsg(scrnIndex, X_INFO,
            "Initializing Memory manager to (%d,%d) (%d,%d)\n",
            AvailBox.x1, AvailBox.y1, AvailBox.x2, AvailBox.y2);
        REGION_INIT(pScrn, &OffscreenRegion, &AvailBox, 2);
        if (!xf86InitFBManagerRegion(pScrn, &OffscreenRegion)) {
            xf86DrvMsg(scrnIndex, X_ERROR,
                "Memory manager initialization failed, Cache Diabled\n");
        }
        REGION_UNINIT(pScrn, &OffscreenRegion);
    } else {
        xf86DrvMsg(scrnIndex, X_INFO,
            "No Off Screen Memory, Cache Disabled (%d,%d) (%d,%d)\n",
            AvailBox.x1, AvailBox.y1, AvailBox.x2, AvailBox.y2);
    }

    xf86SetBlackWhitePixels(pScrn);

    if (!pGeode->ShadowFB) {
        LXDGAInit(pScrn);
    }

    if (pScrni->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScrn->visuals + pScrn->numVisuals;
        while (--visual >= pScrn->visuals) {
            if ((visual->class | DynamicClass) == DirectColor) {
                visual->offsetRed = pScrni->offset.red;
                visual->offsetGreen = pScrni->offset.green;
                visual->offsetBlue = pScrni->offset.blue;
                visual->redMask = pScrni->mask.red;
                visual->greenMask = pScrni->mask.green;
                visual->blueMask = pScrni->mask.blue;
            }
        }
    }
#if CFB
#else
    /* must be after RGB ordering fixed */
    fbPictureInit(pScrn, 0, 0);
#endif

    if (!pGeode->NoAccel) {
        LXAccelInit(pScrn);
    }
    miInitializeBackingStore(pScrn);
    xf86SetBackingStore(pScrn);
    /* Initialise software cursor */
    miDCInitialize(pScrn, xf86GetPointerScreenFuncs());
    /* Initialize HW cursor layer.
     * Must follow software cursor initialization
     */
    if (pGeode->HWCursor) {
        if (!LXHWCursorInit(pScrn))
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                "Hardware cursor initialization failed\n");
    }
    /* Setup default colourmap */
    if (!miCreateDefColormap(pScrn)) {
        return FALSE;
    }
    if (pScrni->bitsPerPixel == 8) {
        /* Initialize colormap layer.
         * Must follow initialization of the default colormap
         */
        if (!xf86HandleColormaps(pScrn, 256, 8, LXLoadPalette, NULL,
                CMAP_PALETTED_TRUECOLOR | CMAP_RELOAD_ON_MODE_SWITCH)) {
            return FALSE;
        }
    }

    if (pGeode->ShadowFB) {
        DEBUGMSG(1, (0, X_INFO, "Shadowed, Rotate=%d, NoAccel=%d\n",
                pGeode->Rotate, pGeode->NoAccel));
        LXShadowFBInit(pScrn, pGeode, bytpp);
    }
#ifdef DPMSExtension
    xf86DPMSInit(pScrn, LXDPMSSet, 0);
#endif

    LXInitVideo(pScrn);                /* needed for video */
    /* Wrap the screen's CloseScreen vector and set its
     * SaveScreen vector 
     */
    pGeode->CloseScreen = pScrn->CloseScreen;
    pScrn->CloseScreen = LXCloseScreen;

    pScrn->SaveScreen = LXSaveScreen;

    /* Report any unused options */
    if (serverGeneration == 1) {
        xf86ShowUnusedOptions(pScrni->scrnIndex, pScrni->options);
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
 * LXSwitchMode.
 *
 * Description	:This function will switches the screen mode
 *   			    				
 * Parameters:
 *    scrnIndex	:Specfies the screen index value.
 *    pMode		:pointer to the mode structure.
 * 	  flags     :may be used for status check?.
 *	  												
 * Returns		:Returns TRUE on success and FALSE on failure.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
Bool
LXSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
    DEBUGMSG(1, (0, X_INFO, "LXSwitchMode\n"));
    return LXSetMode(xf86Screens[scrnIndex], pMode);
}

/*----------------------------------------------------------------------------
 * LXAdjustFrame.
 *
 * Description	:This function is used to intiallize the start
 *				 address of the memory.
 * Parameters.
 *    scrnIndex	:Specfies the screen index value.
 *     x     	:x co-ordinate value interms of pixels.
 * 	   y        :y co-ordinate value interms of pixels.
 *	  												
 * Returns		:none.
 *
 * Comments    	:none.
*----------------------------------------------------------------------------
*/
void
LXAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int newX, newY;
    unsigned long offset;

    if (x + pGeode->HDisplay >= pScrni->virtualX)
        x = pScrni->virtualX - pGeode->HDisplay;
    if (x < 0)
        x = 0;
    if (y + pGeode->VDisplay >= pScrni->virtualY)
        y = pScrni->virtualY - pGeode->VDisplay;
    if (y < 0)
        y = 0;
    pScrni->frameX0 = x;
    pScrni->frameY0 = y;
    pScrni->frameX1 = x + pGeode->HDisplay - 1;
    pScrni->frameY1 = y + pGeode->VDisplay - 1;
    (*pGeode->Rotation) (x, y, pScrni->virtualX, pScrni->virtualY, &newX,
        &newY);
    (*pGeode->RBltXlat) (newX, newY, pGeode->HDisplay, pGeode->VDisplay,
        &newX, &newY);
    offset = pGeode->FBOffset + newY * pGeode->Pitch +
        newX * (pScrni->bitsPerPixel >> 3);
    vg_set_display_offset(offset);
}

/*----------------------------------------------------------------------------
 * LXEnterVT.
 *
 * Description	:This is called when VT switching back to the X server
 *			
 * Parameters.
 *    scrnIndex	:Specfies the screen index value.
 *     flags   	:Not used inside the function.
 * 	 						
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static Bool
LXEnterVT(int scrnIndex, int flags)
{
    DEBUGMSG(1, (0, X_INFO, "LXEnterVT\n"));
    return LXEnterGraphics(NULL, xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * LXLeaveVT.
 *
 * Description	:This is called when VT switching  X server text mode.
 *			
 * Parameters.
 *    scrnIndex	:Specfies the screen index value.
 *     flags    :Not used inside the function.
 * 	 						
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static void
LXLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->PrevDisplayOffset = vg_get_display_offset();
    DEBUGMSG(1, (0, X_INFO, "LXLeaveVT\n"));
    LXLeaveGraphics(xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * LXFreeScreen.
 *
 * Description	:This is called to free any persistent data structures.
 *			
 * Parameters.
 *    scrnIndex :Specfies the screen index value.
 *     flags   	:Not used inside the function.
 * 	 						
 * Returns		:none.
 *
 * Comments     :This will be called only when screen being deleted..
*----------------------------------------------------------------------------
*/
static void
LXFreeScreen(int scrnIndex, int flags)
{
    DEBUGMSG(1, (0, X_INFO, "LXFreeScreen\n"));
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
        vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    LXFreeRec(xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * LXValidMode.
 *
 * Description	:This function checks if a mode is suitable for selected
 *                   		chipset.
 * Parameters.
 *    scrnIndex :Specfies the screen index value.
 *     pMode	:Pointer to the screen mode structure..
 * 	 verbose    :not used for implementation.						
 *     flags    :not used for implementation
 *
 * Returns		:MODE_OK if the specified mode is supported or
 *                    		MODE_NO_INTERLACE.
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static int
LXValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
    unsigned int total_memory_required;
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    int ret = -1;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(0, (0, X_NONE, "GeodeValidateMode: %dx%d %d %d\n",
            pMode->CrtcHDisplay, pMode->CrtcVDisplay,
            pScrni->bitsPerPixel, LXGetRefreshRate(pMode)));
    if (pGeode->CustomMode == 0) {
        int tv_mode;
        VG_QUERY_MODE vgQueryMode;
        unsigned long flags;

        if (pMode->Flags & V_INTERLACE)
            return MODE_NO_INTERLACE;

        flags = VG_QUERYFLAG_REFRESH | VG_QUERYFLAG_BPP |
            VG_QUERYFLAG_ACTIVEWIDTH | VG_QUERYFLAG_ACTIVEHEIGHT;

        if ((pGeode->EnabledOutput & LX_OT_FP) != 0) {
            /* scaling required, but too big to scale */
            if (pGeode->FPGeomDstX != pMode->CrtcHDisplay
                && pMode->CrtcHDisplay > 1024)
                return MODE_NOMODE;
            flags = VG_QUERYFLAG_PANELWIDTH | VG_QUERYFLAG_PANELHEIGHT |
                VG_QUERYFLAG_PANEL;
            vgQueryMode.panel_width = pGeode->FPGeomDstX;
            vgQueryMode.panel_height = pGeode->FPGeomDstY;
        }

        vgQueryMode.active_width = pMode->CrtcHDisplay;
        vgQueryMode.active_height = pMode->CrtcVDisplay;
        vgQueryMode.bpp = pScrni->bitsPerPixel;
        vgQueryMode.hz = LXGetRefreshRate(pMode);
        vgQueryMode.query_flags = VG_QUERYFLAG_REFRESH | VG_QUERYFLAG_BPP |
            VG_QUERYFLAG_ACTIVEWIDTH | VG_QUERYFLAG_ACTIVEHEIGHT;
        if ((tv_mode = lx_tv_mode(pMode)) >= 0) {
            vgQueryMode.encoder = pGeode->tv_encoder;
            vgQueryMode.tvmode = tv_mode;
            vgQueryMode.query_flags |=
                VG_QUERYFLAG_TVMODE | VG_QUERYFLAG_ENCODER;
            vgQueryMode.query_flags &= ~VG_QUERYFLAG_REFRESH;
            if (lx_tv_mode_interlaced(tv_mode) != 0) {
                vgQueryMode.query_flags |= VG_QUERYFLAG_INTERLACED;
                vgQueryMode.active_height /= 2;
            }
        }
        ret = vg_get_display_mode_index(&vgQueryMode);
        if (ret < 0)
            return MODE_NOMODE;
    }

    total_memory_required = LXCalculatePitchBytes(pMode->CrtcHDisplay,
        pScrni->bitsPerPixel) * pMode->CrtcVDisplay;

    DEBUGMSG(0, (0, X_NONE, "Total Mem %x %lx\n",
            total_memory_required, pGeode->FBAvail));

    if (total_memory_required > pGeode->FBAvail)
        return MODE_MEM;

    return MODE_OK;
}

/*----------------------------------------------------------------------------
 * LXLoadPalette.
 *
 * Description	:This function sets the  palette entry used for graphics data
 *
 * Parameters.
 *   pScrni:Points the screeninfo structure.
 *     numColors:Specifies the no of colors it supported.
 * 	 indizes    :This is used get index value .						
 *     LOCO     :to be added.
 *     pVisual  :to be added.
 *
 * Returns		:MODE_OK if the specified mode is supported or
 *          	 MODE_NO_INTERLACE.
 * Comments     :none.
*----------------------------------------------------------------------------
*/

static void
LXLoadPalette(ScrnInfoPtr pScrni,
    int numColors, int *indizes, LOCO * colors, VisualPtr pVisual)
{
    int i, index, color;

    for (i = 0; i < numColors; i++) {
        index = indizes[i] & 0xFF;
        color = (((unsigned long)(colors[index].red & 0xFF)) << 16) |
            (((unsigned long)(colors[index].green & 0xFF)) << 8) |
            ((unsigned long)(colors[index].blue & 0xFF));
        vg_set_display_palette_entry(index, color);
    }
}

static Bool
LXMapMem(ScrnInfoPtr pScrni)
{
    unsigned long cmd_bfr_phys;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(1, (0, X_NONE, "LXMapMem\n"));

    cim_gp_ptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_MMIO,
        pGeode->InitBaseAddress.gp_register_base, pGeode->gp_reg_size);
    cim_vg_ptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_MMIO,
        pGeode->InitBaseAddress.vg_register_base, pGeode->vg_reg_size);
    cim_vid_ptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_MMIO,
        pGeode->InitBaseAddress.df_register_base, pGeode->vid_reg_size);
    cim_vip_ptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_MMIO,
        pGeode->InitBaseAddress.vip_register_base, pGeode->vip_reg_size);

    cim_fb_ptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_FRAMEBUFFER,
        pGeode->InitBaseAddress.framebuffer_base,
        pGeode->InitBaseAddress.framebuffer_size);

    pGeode->FBBase = cim_fb_ptr;

    DEBUGMSG(1, (0, X_NONE, "cim ptrs %p %p %p %p %p\n",
            cim_gp_ptr, cim_vg_ptr, cim_vid_ptr, cim_vip_ptr, cim_fb_ptr));

    /* CHECK IF REGISTERS WERE MAPPED SUCCESSFULLY */
    if ((!cim_gp_ptr) || (!cim_vid_ptr) || (!cim_fb_ptr)) {
        DEBUGMSG(1, (0, X_NONE, "Could not map hardware registers.\n"));
        return (FALSE);
    }

    cmd_bfr_phys =
        pGeode->InitBaseAddress.framebuffer_base + pGeode->CmdBfrOffset;
    cim_cmd_base_ptr = cim_fb_ptr + pGeode->CmdBfrOffset;

    /*
     * Map the top of the frame buffer as the scratch buffer
     * (GP3_SCRATCH_BUFFER_SIZE)
     */
    gp_set_frame_buffer_base(pGeode->InitBaseAddress.framebuffer_base,
        pGeode->FBTop);
    gp_set_command_buffer_base(cmd_bfr_phys, 0, pGeode->CmdBfrSize);
    DEBUGMSG(1, (0, X_NONE, "cim cmd  %p %lx %lx %lx\n",
            cim_cmd_base_ptr, pGeode->CmdBfrSize, cmd_bfr_phys,
            pGeode->FBTop));

    /* Map the XpressROM ptr to read what platform are we on */
    XpressROMPtr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_FRAMEBUFFER, 0xF0000, 0x10000);

    DEBUGMSG(1, (0, X_NONE, "adapter info %lx %lx %lx %lx %p, %p\n",
            pGeode->cpu_version,
            pGeode->vid_version, pGeode->FBLinearAddr,
            pGeode->FBAvail, pGeode->FBBase, XpressROMPtr));

    return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
LXUnmapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(1, (0, X_NONE, "LXUnMapMem\n"));

    /* unmap all the memory map's */
    xf86UnMapVidMem(pScrni->scrnIndex, cim_gp_ptr, pGeode->gp_reg_size);
    xf86UnMapVidMem(pScrni->scrnIndex, cim_vg_ptr, pGeode->vg_reg_size);
    xf86UnMapVidMem(pScrni->scrnIndex, cim_vid_ptr, pGeode->vid_reg_size);
    xf86UnMapVidMem(pScrni->scrnIndex, cim_vip_ptr, pGeode->vip_reg_size);
    xf86UnMapVidMem(pScrni->scrnIndex,
        cim_fb_ptr, pGeode->InitBaseAddress.framebuffer_size);
    xf86UnMapVidMem(pScrni->scrnIndex, XpressROMPtr, 0x10000);

    return TRUE;
}
