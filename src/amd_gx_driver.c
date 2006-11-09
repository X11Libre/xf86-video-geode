/* Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/*
 * File Contents:   This is the main module configures the interfacing with
 *                  the X server. The individual modules will be loaded based
 *                  upon the options selected from the XF86Config. This file
 *                  also has modules for finding supported modes, turning on
 *                  the modes based on options.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 * */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Includes that are used by all drivers */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86_libc.h"
#include "xf86Resources.h"

/* We may want inb() and outb() */
#include "compiler.h"

/* We may want to access the PCI config space */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

/* Colormap handling stuff */
#include "xf86cmap.h"

#define RC_MAX_DEPTH 24

#include "amd.h"
#include "gfx_defs.h"
#include "gfx_regs.h"
#include "panel.h"

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
/* All drivers implementing backing store need this */
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

#include "amd_gx_vga.c"

extern SymTabRec GeodeChipsets[];
extern OptionInfoRec GX_GeodeOptions[];

/* Forward definitions */
static Bool GXPreInit(ScrnInfoPtr, int);
static Bool GXScreenInit(int, ScreenPtr, int, char **);
static Bool GXEnterVT(int, int);
static void GXLeaveVT(int, int);
static void GXFreeScreen(int, int);
void GXAdjustFrame(int, int, int, int);
Bool GXSwitchMode(int, DisplayModePtr, int);
static int GXValidMode(int, DisplayModePtr, Bool, int);
static void GXLoadPalette(ScrnInfoPtr pScrni,
    int numColors, int *indizes, LOCO * colors, VisualPtr pVisual);
static Bool GXMapMem(ScrnInfoPtr);
static Bool GXUnmapMem(ScrnInfoPtr);
static void gx_set_DvLineSize(unsigned int pitch);

extern Bool GXAccelInit(ScreenPtr pScrn);
extern Bool GXHWCursorInit(ScreenPtr pScrn);
extern void GXHideCursor(ScrnInfoPtr pScrni);
extern void GXShowCursor(ScrnInfoPtr pScrni);
extern void GXPointerMoved(int index, int x, int y);
extern void GXRotationInit(ScrnInfoPtr pScrni);
extern void GXShadowFBInit(ScreenPtr pScrn, GeodeRec *pGeode, int bytpp);
extern void GXInitVideo(ScreenPtr pScrn);
extern Bool GXDGAInit(ScreenPtr pScrn);
extern void GXLoadCursorImage(ScrnInfoPtr pScrni, unsigned char *src);

unsigned char *XpressROMPtr;
unsigned long fb;

/* Existing Processor Models */
#define GX1 0x1
#define GX 0x2
#define GX_CRT 0x6
#define GX_TFT 0xA

/* List of symbols from other modules that this module references.The purpose
 * is that to avoid unresolved symbol warnings
 */
extern const char *amdVgahwSymbols[];
extern const char *amdVbeSymbols[];
extern const char *amdInt10Symbols[];

#if CFB
extern const char *amdCfbSymbols[];
#else
extern const char *amdFbSymbols[];
#endif
extern const char *amdXaaSymbols[];
extern const char *amdExaSymbols[];
extern const char *amdRamdacSymbols[];
extern const char *amdShadowSymbols[];

void GXSetupChipsetFPtr(ScrnInfoPtr pScrn);
GeodeRec *GXGetRec(ScrnInfoPtr pScrni);
void get_flatpanel_info(const char *options, int *W, int *H,
    int *D, int *C, int *T);
void gx_clear_screen(ScrnInfoPtr pScrni, int width, int height, int bpp);
void gx_enable_dac_power(ScrnInfoPtr pScrni);
void gx_disable_dac_power(ScrnInfoPtr pScrni);

#if DEBUGLVL>0
FILE *zdfp = NULL;
#endif

void
GXSetupChipsetFPtr(ScrnInfoPtr pScrn)
{
#if DEBUGLVL>0
    if (zdfp == NULL) {
        zdfp = fopen("/tmp/xwin.log", "w");
#if DEBUGTIM==0
        setbuf(zdfp, NULL);
#endif
    }
#endif
    DEBUGMSG(1, (0, X_INFO, "GXSetupChipsetFPtr!\n"));
    pScrn->PreInit = GXPreInit;
    pScrn->ScreenInit = GXScreenInit;
    pScrn->SwitchMode = GXSwitchMode;
    pScrn->AdjustFrame = GXAdjustFrame;
    pScrn->EnterVT = GXEnterVT;
    pScrn->LeaveVT = GXLeaveVT;
    pScrn->FreeScreen = GXFreeScreen;
    pScrn->ValidMode = GXValidMode;
}

/*----------------------------------------------------------------------------
 * GXGetRec.
 *
 * Description	:This function allocates a GeodeRec and hooks into pScrni
 *				struct's driverPrivate member of ScreeenInfo structure.
 *
 * Parameters.
 *		pScrni	:Pointer handle to the screenonfo structure.
 *
 * Returns		:allocated pScrninfo structure.
 *
 * Comments     :none
 *
 *----------------------------------------------------------------------------
 */
GeodeRec *
GXGetRec(ScrnInfoPtr pScrni)
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
 * GXFreeRec.
 *
 * Description	:This function deallocates and disconnect the GeodeRec from
 *               the pScrni struct's driverPrivate member.
 *               
 * Parameters.
 *		pScrni	:Pointer handle to the screenonfo structure.
 *
 * Returns		:none
 *
 * Comments     :none
 *
 *----------------------------------------------------------------------------
 */
static void
GXFreeRec(ScrnInfoPtr pScrni)
{
    if (pScrni->driverPrivate == NULL) {
        return;
    }
    xfree(pScrni->driverPrivate);
    pScrni->driverPrivate = NULL;
}

/*----------------------------------------------------------------------------
 * GXSaveScreen.
 *
 * Description	:This is todo the screen blanking
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
GXSaveScreen(ScreenPtr pScrn, int mode)
{
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
   GeodePtr pGeode = GEODEPTR(pScrni);

   if (pGeode->useVGA && !pScrni->vtSema)
        return vgaHWSaveScreen(pScrn, mode);
    return TRUE;
}

/*----------------------------------------------------------------------------
 * get_flatpanel_info.
 *
 * Description:		This gets the parameter values of the flatpanel attached.
 *
 * Parameters:
 *		options:	Pointer to the display options.
 *		W:			Pointer to the width of the panel
 *		H:			Pointer to the height of the panel
 * 	    D: 			Pointer to the depth of the panel.
 *      C: 			Pointer to the color of the panel. 
 *      T: 			Pointer to the type of the panel. 
 *
 * Returns: 		none.
 *
 * Comments:		none
 *------------------------------------------------------------------------
 */
void
get_flatpanel_info(const char *options, int *W, int *H,
    int *D, int *C, int *T)
{
    char *pnl_opt;

    pnl_opt = strtok((char *)options, ":");
    *W = strtoul(pnl_opt, NULL, 0);
    pnl_opt = strtok(NULL, ":");
    *H = strtoul(pnl_opt, NULL, 0);
    pnl_opt = strtok(NULL, ":");
    *D = strtoul(pnl_opt, NULL, 0);
    pnl_opt = strtok(NULL, ":");
    *C = strtoul(pnl_opt, NULL, 0);
    pnl_opt = strtok(NULL, ":");
    *T = strtoul(pnl_opt, NULL, 0);

    *C = (*C) ? PNL_COLOR_PANEL : PNL_MONO_PANEL;

    switch (*T) {
    case 0:
        *T = PNL_SSTN;
        break;
    case 1:
        *T = PNL_DSTN;
        break;
    case 2:
    default:
        *T = PNL_TFT;
        break;
    }

    if ((*W != 640) && (*W != 800) && (*W != 1024))
        *W = 640;

    if ((*H != 480) && (*H != 600) && (*H != 768))
        *H = 480;
}

static xf86MonPtr
GXProbeDDC(ScrnInfoPtr pScrni, int index)
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
GXDecodeDDC(ScrnInfoPtr pScrni, xf86MonPtr ddc)
{
    int i;

    DEBUGMSG(1, (0, X_INFO,
            "Detected monitor DDC (%4s) id %d serno %d week %d year %d "
            "nsects %d\n",
            &ddc->vendor.name[0], ddc->vendor.prod_id, ddc->vendor.serial,
            ddc->vendor.week, ddc->vendor.year, ddc->no_sections));
    for (i = 0; i < DET_TIMINGS; ++i) {
        struct detailed_monitor_section *dp = &ddc->det_mon[i];

        switch (dp->type) {
        case DT:
            {
                struct detailed_timings *r = &dp->section.d_timings;

                DEBUGMSG(1, (0, X_INFO, "  mon det timing %0.3f  %dx%d\n",
                        r->clock / 1000000.0, r->h_active, r->v_active));
                DEBUGMSG(1, (0, X_INFO,
                        "  mon border %d,%d laced %d stereo %d sync %d, misc"
                        " %d\n",
                        r->h_border, r->v_border, r->interlaced, r->stereo,
                        r->sync, r->misc));
            }
            break;
        case DS_SERIAL:
            {
                char *serial_no = (char *)&dp->section.serial[0];

                DEBUGMSG(1, (0, X_INFO, "  mon serial %13s\n", serial_no));
            }
            break;
        case DS_ASCII_STR:
            {
                char *ascii = (char *)&dp->section.ascii_data[0];

                DEBUGMSG(1, (0, X_INFO, "  mon ascii_str %13s\n", ascii));
            }
            break;
        case DS_NAME:
            {
                char *name = (char *)&dp->section.name[0];

                DEBUGMSG(1, (0, X_INFO, "  mon name %13s\n", name));
            }
            break;
        case DS_RANGES:
            {
                struct monitor_ranges *r = &dp->section.ranges;

                DEBUGMSG(1, (0, X_INFO,
                        "  mon ranges v %d-%d h %d-%d clk %d\n", r->min_v,
                        r->max_v, r->min_h, r->max_h, r->max_clock));
            }
            break;
        case DS_WHITE_P:
            {
                struct whitePoints *wp = &dp->section.wp[0];

                DEBUGMSG(1, (0, X_INFO,
                        "  mon whitept %f,%f %f,%f idx %d,%d gamma %f,%f\n",
                        wp[1].white_x, wp[1].white_y, wp[2].white_x,
                        wp[2].white_y, wp[1].index, wp[2].index,
                        wp[1].white_gamma, wp[2].white_gamma));
            }
            break;
        case DS_STD_TIMINGS:
            {
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

/*----------------------------------------------------------------------------
 * GXPreInit.
 *
 * Description	:This function is called only once ate teh server startup
 *
 * Parameters.
 *  	pScrni 	:Handle to ScreenPtr structure.
 *  flags       :flags may be used to check the probeed one with config.
 * 												
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static Bool
GXPreInit(ScrnInfoPtr pScrni, int flags)
{
    ClockRangePtr GeodeClockRange;
    MessageType from;
    int i = 0;
    GeodeRec *pGeode;
    char *mod = NULL;
    xf86MonPtr ddc = NULL;
    OptionInfoRec *GeodeOptions = &GX_GeodeOptions[0];

#if CFB
    char *reqSymbol = NULL;
#endif /* CFB */
#if INT10_SUPPORT
    VESARec *pVesa;
#endif
    unsigned int PitchInc = 0, minPitch = 0, maxPitch = 0;
    unsigned int minHeight = 0, maxHeight = 0, maxWidth = 0;
    unsigned int SupportFlags;
    const char *s;
    char **modes;

    /*
     * Setup the ClockRanges, which describe what clock ranges are
     * available, and what sort of modes they can be used for.
     */
    GeodeClockRange = (ClockRangePtr) xnfcalloc(sizeof(ClockRange), 1);
    GeodeClockRange->next = NULL;
    GeodeClockRange->minClock = 25175;
    GeodeClockRange->maxClock = 229500;
    GeodeClockRange->clockIndex = -1;  /* programmable */
    GeodeClockRange->interlaceAllowed = TRUE;
    GeodeClockRange->doubleScanAllowed = FALSE; /* XXX check this */

    DEBUGMSG(1, (0, X_INFO, "GXPreInit!\n"));
    /* Allocate driver private structure */
    if (!(pGeode = GXGetRec(pScrni)))
        return FALSE;

   if (pScrni->numEntities != 1) 
     return FALSE;

        pGeode->pEnt = xf86GetEntityInfo(pScrni->entityList[i]);
        if (pGeode->pEnt->resources)
            return FALSE;
        pGeode->Chipset = pGeode->pEnt->chipset;
        pScrni->chipset = (char *)xf86TokenToString(GeodeChipsets,
            pGeode->pEnt->chipset);


   /* DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR, "PROBEDDC\n")); */

   /* Note that we can't do this without VGA */

   if (flags & PROBE_DETECT != 0) {
	ConfiguredMonitor = GXProbeDDC(pScrni, pGeode->pEnt->index);
        return TRUE;
    }

   /* Hardware detection */

    pGeode->cpu_version = gfx_detect_cpu();

   if ((pGeode->cpu_version & 0xFF) == GFX_CPU_REDCLOUD) {
	   int ret;
        Q_WORD msrValue;
	   pGeode->DetectedChipSet = GX;

	   /* See if this a CRT or TFT part */

        ret = gfx_msr_read(RC_ID_DF, MBD_MSR_CONFIG, &msrValue);

	/* We depend heavily on the MSR working, so if it doesn't, there
	 * isn't much we can do but complain and move on
	 */

	if (ret != 0) {
		DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
					"MSR read failed (ret=%d)\n", ret));
	}

        pGeode->DetectedChipSet =
            ((msrValue.low & RCDF_CONFIG_FMT_MASK) ==
            RCDF_CONFIG_FMT_FP) ? GX_TFT : GX_CRT;

    }

    pGeode->vid_version = gfx_detect_video();
    pGeode->FBLinearAddr = gfx_get_frame_buffer_base();

    /* update the max clock from the one system suports  */
    GeodeClockRange->maxClock = gfx_get_max_supported_pixel_clock();


   /* Set Durango register pointers */

    if (pGeode->DetectedChipSet & GX) {
        pGeode->cpu_reg_size = 0x4000;
        pGeode->gp_reg_size = 0x4000;
        pGeode->vid_reg_size = 0x4000;
    } else {
        pGeode->cpu_reg_size = 0x9000;
        pGeode->vid_reg_size = 0x1000;
    }


   pGeode->FBVGAActive = 0;

    /* Fill in the monitor field */
    pScrni->monitor = pScrni->confScreen->monitor;

    SupportFlags = Support24bppFb | Support32bppFb;

    if (!xf86SetDepthBpp(pScrni, 8, 8, 8, SupportFlags)) {
        return FALSE;
    } else {
        if (!((pScrni->depth == 8) ||
                (pScrni->depth == 16) ||
                (pScrni->depth == 24) || (pScrni->depth == 32))) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                    "Given depth (%d bpp) is not supported by this driver\n",
                    pScrni->depth));
            return FALSE;
        }
    }

    /*This must happen after pScrni->display has been set
     * * because xf86SetWeight references it.
     */
    if (pScrni->depth > 8) {

        rgb BitsPerComponent = { 0, 0, 0 };
        rgb BitMask = { 0, 0, 0 };

        if (pScrni->depth > 16) {

            BitsPerComponent.red = 8;
            BitsPerComponent.green = 8;
            BitsPerComponent.blue = 8;

            BitMask.red = 0xFF0000;
            BitMask.green = 0x00FF00;
            BitMask.blue = 0x0000FF;
        }
        if (!xf86SetWeight(pScrni, BitsPerComponent, BitMask)) {
            return FALSE;
        } else {
            /* XXX Check if the returned weight is supported */
        }
    }

    xf86PrintDepthBpp(pScrni);

    if (!xf86SetDefaultVisual(pScrni, -1))
        return FALSE;

    if (pScrni->depth > 1) {
        Gamma zeros = { 0.0, 0.0, 0.0 };

        if (!xf86SetGamma(pScrni, zeros)) {
            return FALSE;
        }
    }

    /* We use a programmable clock */
    pScrni->progClock = TRUE;

    xf86CollectOptions(pScrni, NULL);

   xf86ProcessOptions(pScrni->scrnIndex, pScrni->options,
		      GeodeOptions);

   pGeode->useVGA = TRUE;

   if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOVGA, FALSE))
	pGeode->useVGA = FALSE;

    xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT,
        "useVGA=%d\n", pGeode->useVGA);


   if (pGeode->useVGA) {
#if INT10_SUPPORT
     if (!xf86LoadSubModule(pScrni, "int10"))
       return FALSE;
     xf86LoaderReqSymLists(amdInt10Symbols, NULL);

    pVesa = pGeode->vesa;

     if ((pVesa->pInt = xf86InitInt10(pGeode->pEnt->index)) == NULL) {
         DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                  "Unable to initialize INT10 support\n"));
         return FALSE;
     }
#endif

     if (!xf86LoadSubModule(pScrni, "vgahw"))
         return FALSE;

     xf86LoaderReqSymLists(amdVgahwSymbols, NULL);

     pGeode->FBAvail = gfx_get_frame_buffer_size();
   }
   else {
     if (!xf86GetOptValInteger(GeodeOptions, GX_OPTION_FBSIZE,
			       &(pGeode->FBAvail)))
       pGeode->FBAvail = 0x800000;
   }

   if (!GXMapMem(pScrni))
      return FALSE;

   pGeode->FBVGAActive = FALSE;

   if (pGeode->useVGA)
       pGeode->FBVGAActive = gu2_get_vga_active();

    /*Set the bits per RGB for 8bpp mode */

   if (pScrni->depth == 8)
        pScrni->rgbBits = 8;

    from = X_DEFAULT;

    /*
     * *The preferred method is to use the "hw cursor" option as a tri-state
     * *option, with the default set above.
     */
    pGeode->HWCursor = TRUE;

   if (xf86GetOptValBool(GeodeOptions, GX_OPTION_HW_CURSOR, &pGeode->HWCursor)) {
        from = X_CONFIG;
    }
    /* For compatibility, accept this too (as an override) */
    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_SW_CURSOR, FALSE)) {
        from = X_CONFIG;
        pGeode->HWCursor = FALSE;
    }

    pGeode->Compression = TRUE;
    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOCOMPRESSION, FALSE)) {
        pGeode->Compression = FALSE;
    }

    pGeode->NoAccel = FALSE;
    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOACCEL, FALSE)) {
        pGeode->NoAccel = TRUE;
    }

    if (!xf86GetOptValInteger(GeodeOptions, GX_OPTION_OSM_IMG_BUFS,
            &(pGeode->NoOfImgBuffers)))
        pGeode->NoOfImgBuffers = DEFAULT_IMG_LINE_BUFS;

    if (pGeode->NoOfImgBuffers <= 0)
        pGeode->NoOfImgBuffers = 0;

    if (!xf86GetOptValInteger(GeodeOptions, GX_OPTION_OSM_CLR_BUFS,
            &(pGeode->NoOfColorExpandLines)))
        pGeode->NoOfColorExpandLines = DEFAULT_CLR_LINE_BUFS;

    if (pGeode->NoOfColorExpandLines <= 0)
        pGeode->NoOfColorExpandLines = 0;


#ifdef XF86EXA
    if (!xf86GetOptValInteger(GeodeOptions, GX_OPTION_EXA_SCRATCH_BFRSZ,
            &(pGeode->exaBfrSz)))
        pGeode->exaBfrSz = DEFAULT_EXA_SCRATCH_BFRSZ;

    if (pGeode->exaBfrSz <= 0)
        pGeode->exaBfrSz = 0;
#endif

  

    pGeode->CustomMode = FALSE;
    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_CUSTOM_MODE, FALSE)) {
        pGeode->CustomMode = TRUE;
    }

   pGeode->Panel = FALSE;

   if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_FLATPANEL, FALSE)) {
     pGeode->Panel = TRUE;
   }

    if (pGeode->DetectedChipSet == GX_TFT) {
        pGeode->Panel = TRUE;
    }

   pGeode->dconPanel = FALSE;

   if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_DCONPANEL, FALSE)) {
	   pGeode->dconPanel = TRUE;
	   gx_dcon_init();
   }

   /* Force the Panel on if on a GX TFT part, no CRT support anyway */

   if (pGeode->DetectedChipSet == GX_TFT) {
        pGeode->Panel = TRUE;
   }

   /* If on a CRT and Panel flag set, disable Panel */
   if ((pGeode->DetectedChipSet == GX_CRT) && (pGeode->Panel)) {
       pGeode->dconPanel = FALSE;
       pGeode->Panel = FALSE;
    }

#if defined(PNL_SUP)
    /* If dconPanel is selected - then hard wire the settings */
    /* Otherwise, suck the timings out of the BIOS */

    if (pGeode->dconPanel) {
	    pGeode->FPBX = DCON_DEFAULT_XRES;
	    pGeode->FPBY = DCON_DEFAULT_YRES;
	    pGeode->FPBB = DCON_DEFAULT_BPP;
	    pGeode->FPBF = DCON_DEFAULT_REFRESH;
            DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR, "DCON!\n"));
    } else if (pGeode->Panel) {
	if ((pGeode->Panel = Pnl_IsPanelEnabledInBIOS()))
		Pnl_GetPanelInfoFromBIOS(&pGeode->FPBX, &pGeode->FPBY,
		&pGeode->FPBB, &pGeode->FPBF);
    }

    if (pGeode->Panel)
       Pnl_PowerUp();
     else
       Pnl_PowerDown();
#endif

    pGeode->useEXA = FALSE;
    from = X_DEFAULT;
    if ((s = xf86GetOptValString(GeodeOptions, GX_OPTION_ACCEL_METHOD))) {
        if (!xf86NameCmp(s, "XAA")) {
            from = X_CONFIG;
            pGeode->useEXA = FALSE;
#if XF86EXA
        } else if (!xf86NameCmp(s, "EXA")) {
            from = X_CONFIG;
            pGeode->useEXA = TRUE;
#endif
        } else
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                "Unknown acceleration architecture - %s\n", s);
    }

    xf86DrvMsg(pScrni->scrnIndex, from,
        "Using %s acceleration architecture\n",
        pGeode->useEXA ? "EXA" : "XAA");

    pGeode->ShadowFB = FALSE;
    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_SHADOW_FB, FALSE)) {
        pGeode->ShadowFB = TRUE;
        pGeode->NoAccel = TRUE;
    }

    pGeode->Rotate = 0;
    if ((s = xf86GetOptValString(GeodeOptions, GX_OPTION_ROTATE))) {

     if (!xf86NameCmp(s, "CW")) 
            pGeode->Rotate = 1;
      else if (!xf86NameCmp(s, "INVERT")) 
            pGeode->Rotate = 2;
      else if (!xf86NameCmp(s, "CCW")) 
            pGeode->Rotate = 3;


        if (pGeode->Rotate == 0) {
            DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG,
                    "\"%s\" is not a valid value for Option \"Rotate\"\n",
                    s));
            DEBUGMSG(1, (pScrni->scrnIndex, X_INFO,
                    "Valid options are \"CW\", \"INVERT\" or \"CCW\"\n"));
        }
    }

   if (pGeode->Rotate == 1) 
     xf86DrvMsg(pScrni->scrnIndex, X_CONFIG, "Rotating screen clockwise\n");
   else if (pGeode->Rotate == 2)
      xf86DrvMsg(pScrni->scrnIndex, X_CONFIG, "Rotating screen 180 degrees\n");
   else if (pGeode->Rotate == 3)
     xf86DrvMsg(pScrni->scrnIndex, X_CONFIG, "Rotating screen 180 counter clockwise\n");

    if (pGeode->Rotate != 0) {
     xf86DrvMsg(pScrni->scrnIndex, X_CONFIG, "Option 'Rotate' will disable acceleration and enable shadow\n");
        pGeode->NoAccel = TRUE;
        pGeode->ShadowFB = TRUE;
    }

    /* XXX Init further private data here */

    /*
     * * This shouldn't happen because such problems should be caught in
     * * GeodeProbe(), but check it just in case.
     */
    if (pScrni->chipset == NULL) {
     xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "ChipID 0x%04X is not recognised\n", pGeode->Chipset);
        return FALSE;
    }

    if (pGeode->Chipset < 0) {
     xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "Chipset %s is not recognised\n", pScrni->chipset);
        return FALSE;
    }

    if (pScrni->memPhysBase == 0) {
        from = X_PROBED;
        pScrni->memPhysBase = gfx_get_frame_buffer_base();
    }

    pScrni->fbOffset = 0;

    if (pGeode->pEnt->device->videoRam == 0) {
        from = X_PROBED;
        pScrni->videoRam = pGeode->FBAvail / 1024;
    } else {
        pScrni->videoRam = pGeode->pEnt->device->videoRam;
        from = X_CONFIG;
    }

    /*
     * * xf86ValidateModes will check that the mode HTotal and VTotal values
     * * don't exceed the chipset's limit if pScrni->maxHValue adn
     * * pScrni->maxVValue are set. Since our GXValidMode()
     * * already takes care of this, we don't worry about setting them here.
     */
    /* Select valid modes from those available */

    /*
     * * min pitch 1024, max 2048 (Pixel count)
     * * min height 480, max 1024 (Pixel count)
     */

    minPitch = 1024;
    maxPitch = 8192;                   /* Can support upto 1600x1200 32Bpp */
    maxWidth = 1600;
    minHeight = 400;
    maxHeight = 1200;                  /* Can support upto 1600x1200 32Bpp */


    if (pGeode->Compression) {
        if (pScrni->depth > 16) {
            PitchInc = 4096;
        } else if (pScrni->depth == 16) {
            PitchInc = 2048;
        } else {
            PitchInc = 1024;
        }
    }
    else {
	/* When compression is off - use a linear pitch */
	if (pScrni->depth < 16)
		PitchInc = 1600;
	else if (pScrni->depth == 16)
		PitchInc = 3200;
	else
		PitchInc = 6400;
    }

    PitchInc <<= 3;                    /* in bits */

    pGeode->maxWidth = maxWidth;
    pGeode->maxHeight = maxHeight;

    /* by default use what user sets in the XF86Config file */
    modes = pScrni->display->modes;

    if (ddc != NULL && pScrni->monitor != NULL
        && pScrni->monitor->DDC == NULL) {
        pScrni->monitor->DDC = ddc;
        GXDecodeDDC(pScrni, ddc);
    }

    i = xf86ValidateModes(pScrni,
        pScrni->monitor->Modes,
        modes, GeodeClockRange,
        NULL, minPitch, maxPitch,
        PitchInc, minHeight, maxHeight,
        pScrni->display->virtualX,
        pScrni->display->virtualY, pGeode->FBAvail, LOOKUP_BEST_REFRESH);

    if (i == -1) {
     xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes were found\n");
        GXFreeRec(pScrni);
        return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrni);

    if (i == 0 || pScrni->modes == NULL) {
     xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes were found\n");
        GXFreeRec(pScrni);
        return FALSE;
    }

    xf86SetCrtcForModes(pScrni, 0);

    /* Set the current mode to the first in the list */
    pScrni->currentMode = pScrni->modes;
    DEBUGMSG(1, (0, X_INFO, "GXPreInit(12)!\n"));

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
        GXFreeRec(pScrni);
        return FALSE;
    }

    xf86LoaderReqSymbols(reqSymbol, NULL);
#else
    if (xf86LoadSubModule(pScrni, "fb") == NULL) {
        GXFreeRec(pScrni);
        return FALSE;
    }

    xf86LoaderReqSymLists(amdFbSymbols, NULL);
#endif

    if (pGeode->NoAccel == FALSE) {
        const char *module = "xaa";
        const char **symbols = &amdXaaSymbols[0];

#if XF86EXA
        if (pGeode->useEXA) {
            module = "exa";
            symbols = &amdExaSymbols[0];
        }
#endif
        if (!xf86LoadSubModule(pScrni, module)) {
            GXFreeRec(pScrni);
            return FALSE;
        }

        xf86LoaderReqSymLists(symbols, NULL);
    }

    if (pGeode->HWCursor == TRUE) {
        if (!xf86LoadSubModule(pScrni, "ramdac")) {
            GXFreeRec(pScrni);
            return FALSE;
        }

        xf86LoaderReqSymLists(amdRamdacSymbols, NULL);
    }

    /* Load shadowfb if needed */
    if (pGeode->ShadowFB) {
        if (!xf86LoadSubModule(pScrni, "shadowfb")) {
            GXFreeRec(pScrni);
            return FALSE;
        }

        xf86LoaderReqSymLists(amdShadowSymbols, NULL);
    }

    if (xf86RegisterResources(pGeode->pEnt->index, NULL, ResExclusive)) {
        DEBUGMSG(1, (pScrni->scrnIndex, X_ERROR,
                "xf86RegisterResources() found resource conflicts\n"));
        GXFreeRec(pScrni);
        return FALSE;
    }

    GXUnmapMem(pScrni);
    return TRUE;
}

/*----------------------------------------------------------------------------
 * GXRestore.
 *
 * Description	:This function restores the mode that was saved on server
                 entry
 * Parameters.
 * 	pScrni 		:Handle to ScreenPtr structure.
 *  Pmode       :poits to screen mode
 * 												
 * Returns		:none.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static void
GXRestore(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

   if (pGeode->useVGA && pGeode->FBVGAActive) {
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        vgaHWProtect(pScrni, TRUE);
        vgaHWRestore(pScrni, &pvgaHW->SavedReg, VGA_SR_ALL);
        vgaHWProtect(pScrni, FALSE);
    }
}

/*----------------------------------------------------------------------------
 * GXCalculatePitchBytes.
 *
 * Description	:This function restores the mode that was saved on server
 *
 * Parameters.
 * 		pScrni 	:Handle to ScreenPtr structure.
 *   	Pmode   :Points to screenmode
 * 									
 * Returns		:none.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static int
GXCalculatePitchBytes(unsigned int width, unsigned int bpp)
{
    int lineDelta = width * (bpp >> 3);

    if (width < 640) {
        /* low resolutions have both pixel and line doubling */
        DEBUGMSG(1, (0, X_PROBED, "lower resolution %d %d\n",
                width, lineDelta));
        lineDelta <<= 1;
    }
    /* needed in Rotate mode when in accel is turned off */
    if (1) {                           /*!pGeode->NoAccel */
        if (lineDelta > 4096)
            lineDelta = 8192;
        else if (lineDelta > 2048)
            lineDelta = 4096;
        else if (lineDelta > 1024)
            lineDelta = 2048;
        else
            lineDelta = 1024;
    }

    DEBUGMSG(1, (0, X_PROBED, "pitch %d %d\n", width, lineDelta));

    return lineDelta;
}

/*----------------------------------------------------------------------------
 * GXGetRefreshRate.
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
GXGetRefreshRate(DisplayModePtr pMode)
{
#define THRESHOLD 2
    unsigned int i;
    static int validRates[] = { 56, 60, 70, 72, 75, 85, 90, 100 };      /* Hz */
    unsigned long dotClock;
    int refreshRate;
    int selectedRate;

    dotClock = pMode->SynthClock * 1000;
    refreshRate = dotClock / pMode->CrtcHTotal / pMode->CrtcVTotal;

    if ((pMode->CrtcHTotal < 640) && (pMode->CrtcVTotal < 480))
        refreshRate >>= 2;             /* double pixel and double scan */

    DEBUGMSG(1, (0, X_PROBED, "dotclock %lu %d\n", dotClock, refreshRate));

    selectedRate = validRates[0];

    for (i = 0; i < (sizeof(validRates) / sizeof(validRates[0])); i++) {
        if (validRates[i] < (refreshRate + THRESHOLD)) {
            selectedRate = validRates[i];
        }
    }

    return selectedRate;
}

void
gx_clear_screen(ScrnInfoPtr pScrni, int width, int height, int bpp)
{
    /* no accels, mode is not yet set */
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long offset = gfx_get_display_offset();
    unsigned long pitch = gfx_get_display_pitch();
    unsigned long n = width * ((bpp + 7) >> 3);

    DEBUGMSG(1, (0, X_PROBED, "clear screen %lx %d %d %d %ld %ld\n", offset,
            width, height, bpp, pitch, n));
    while (height > 0) {
        memset(pGeode->FBBase + offset, 0, n);
        offset += pitch;
        --height;
    }
}

void
gx_clear_fb(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned char *fb = pGeode->FBBase + pGeode->FBOffset;

    memset(fb, 0, pGeode->FBSize);
    if (pGeode->ShadowPtr != NULL && pGeode->ShadowPtr != fb)
        memset(pGeode->ShadowPtr, 0, pGeode->ShadowSize);
}

void
gx_set_DvLineSize(unsigned int pitch)
{
    unsigned long temp, dv_size = MDC_DV_LINE_SIZE_1024;

    if (pitch > 1024) {
        dv_size = MDC_DV_LINE_SIZE_2048;
    }
    if (pitch > 2048) {
        dv_size = MDC_DV_LINE_SIZE_4096;
    }
    if (pitch > 4096) {
        dv_size = MDC_DV_LINE_SIZE_8192;
    }

    /* WRITE DIRTY/VALID CONTROL WITH LINE LENGTH */

    temp = READ_REG32(MDC_DV_CTL);
    WRITE_REG32(MDC_DV_CTL, (temp & ~MDC_DV_LINE_SIZE_MASK) | dv_size);
}

/*----------------------------------------------------------------------------
 * GXSetMode.
 *
 * Description	:This function sets parametrs for screen mode
 *
 * Parameters.
 * 		pScrni 	:Pointer to the screenInfo structure.
 *	 	Pmode   :Pointer to the screen modes
 * 												
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */

static Bool
GXSetMode(ScrnInfoPtr pScrni, DisplayModePtr pMode)
{
    int flags;
    static char *quals[4] = {
        " +hsync +vsync", " -hsync +vsync", " +hsync -vsync", " -hsync -vsync"
    };
    GeodeRec *pGeode = GEODEPTR(pScrni);

    gfx_wait_until_idle();
    /* disable video */
    gx_disable_dac_power(pScrni);

    /* unsigned int compOffset, compPitch, compSize; */
    DEBUGMSG(1, (0, X_INFO, "GXSetMode %p %p %p %p %p\n",
            gfx_virt_regptr, gfx_virt_gpptr,
            gfx_virt_spptr, gfx_virt_vidptr, gfx_virt_fbptr));

    /* Set the VT semaphore */
    pScrni->vtSema = TRUE;

    /* The timing will be adjusted later */
    flags = 0;
    if ((pMode->Flags & V_NHSYNC) != 0)
        flags |= 1;

    if ((pMode->Flags & V_NVSYNC) != 0)
        flags |= 2;

    DEBUGMSG(1, (0, X_PROBED,
            "Setting mode %dx%d %0.3f  %d %d %d %d  %d %d %d %d%s\n",
            pMode->CrtcHDisplay, pMode->CrtcVDisplay,
            pMode->SynthClock / 1000.0, pMode->CrtcHDisplay,
            pMode->CrtcHSyncStart, pMode->CrtcHSyncEnd, pMode->CrtcHTotal,
            pMode->CrtcVDisplay, pMode->CrtcVSyncStart, pMode->CrtcVSyncEnd,
            pMode->CrtcVTotal, quals[flags]));
    DEBUGMSG(1, (0, X_INFO, "Set display mode: %dHz Pitch %d\n",
            GXGetRefreshRate(pMode), pGeode->Pitch));

    /* TV not selected */
    DEBUGMSG(1, (0, X_PROBED, "Setting Display for CRT or TFT\n"));
    if (pGeode->CustomMode != 0) {
        DEBUGMSG(1, (0, X_PROBED, "Setting Custom mode\n"));
        GFX(set_display_timings(pScrni->bitsPerPixel, flags,
                pMode->CrtcHDisplay, pMode->CrtcHBlankStart,
                pMode->CrtcHSyncStart, pMode->CrtcHSyncEnd,
                pMode->CrtcHBlankEnd, pMode->CrtcHTotal,
                pMode->CrtcVDisplay, pMode->CrtcVBlankStart,
                pMode->CrtcVSyncStart, pMode->CrtcVSyncEnd,
                pMode->CrtcVBlankEnd, pMode->CrtcVTotal,
                (int)((pMode->SynthClock / 1000.0) * 0x10000)));
    } else if (pGeode->Panel != 0) {
        DEBUGMSG(0, (0, X_PROBED, "Setting Display for TFT\n"));
        DEBUGMSG(1, (0, X_PROBED, "Restore Panel %d %d %d %d %d\n",
                pGeode->FPBX, pGeode->FPBY,
                pMode->CrtcHDisplay,
                pMode->CrtcVDisplay, pScrni->bitsPerPixel));

        DEBUGMSG(1, (pScrni->scrnIndex, X_CONFIG, "FP Bios %d\n",
                pGeode->Panel));
        GFX(set_fixed_timings(pGeode->FPBX, pGeode->FPBY,
                pMode->CrtcHDisplay,
                pMode->CrtcVDisplay, pScrni->bitsPerPixel));
    } else {
        /* display is crt */
        DEBUGMSG(1, (0, X_PROBED,
                "Setting Display for CRT %dx%d-%d@%d\n",
                pMode->CrtcHDisplay, pMode->CrtcVDisplay,
                pScrni->bitsPerPixel, GXGetRefreshRate(pMode)));
        GFX(set_display_mode(pMode->CrtcHDisplay, pMode->CrtcVDisplay,
                pScrni->bitsPerPixel, GXGetRefreshRate(pMode)));

        /* adjust the pitch */
    }

    /* enable crt */
    GFX(set_crt_enable(CRT_ENABLE));
    GFX(set_display_pitch(pGeode->Pitch));
    GFX(set_display_offset(0L));
    GFX(wait_vertical_blank());

    DEBUGMSG(1, (0, X_PROBED, "Display mode set\n"));
    /* enable compression if option selected */
    if (pGeode->Compression) {
        DEBUGMSG(1, (0, X_PROBED, "Compression mode set %d\n",
                pGeode->Compression));
        /* set the compression parameters,and it will be turned on later. */
        gx_set_DvLineSize(pGeode->Pitch);

        gfx_set_compression_offset(pGeode->CBOffset);
        gfx_set_compression_pitch(pGeode->CBPitch);
        gfx_set_compression_size(pGeode->CBSize);

        /* set the compression buffer, all parameters already set */
        gfx_set_compression_enable(1);
    }

    if (pGeode->HWCursor) {
        /* Load blank cursor */
        GXLoadCursorImage(pScrni, NULL);
        GFX(set_cursor_position(pGeode->CursorStartOffset, 0, 0, 0, 0));
        GFX(set_cursor_enable(1));
   } else {
      DEBUGMSG(1,(0, X_INFO, "GXRestore ... "));
      GXRestore(pScrni);
      DEBUGMSG(1,(0, X_INFO, "done.\n"));
   }

    DEBUGMSG(1, (0, X_INFO, "done.\n"));
    /* Reenable the hardware cursor after the mode switch */
    if (pGeode->HWCursor == TRUE) {
        DEBUGMSG(1, (0, X_INFO, "GXShowCursor ... "));
        GXShowCursor(pScrni);
        DEBUGMSG(1, (0, X_INFO, "done.\n"));
    }

    if (!pGeode->Panel)
        GFX(set_display_offset(pGeode->PrevDisplayOffset));

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
    gx_enable_dac_power(pScrni);
    return TRUE;
}

/*----------------------------------------------------------------------------
 * GXEnterGraphics.
 *
 * Description	:This function will intiallize the displaytiming
				 structure for nextmode and switch to VGA mode.
 *
 * Parameters.
 *   	pScrn  	:Screen information will be stored in this structure.
 * 		pScrni	:Pointer to the screenInfo structure.
 *													
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments     :gfx_vga_mode_switch() will start and end the
 *				 switching based on the arguments 0 or 1.soft_vga
 *				 is disabled in this function.
 *----------------------------------------------------------------------------
 */
static Bool
GXEnterGraphics(ScreenPtr pScrn, ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    gfx_wait_until_idle();

    DEBUGMSG(1, (0, X_INFO, "GXEnterGraphics(1)!\n"));

    /* Save CRT State */
    pGeode->FBgfxdisplaytiming.dwDotClock = gfx_get_clock_frequency();
    pGeode->FBgfxdisplaytiming.wPitch = gfx_get_display_pitch();
    pGeode->FBgfxdisplaytiming.wBpp = gfx_get_display_bpp();
    pGeode->FBgfxdisplaytiming.wHTotal = gfx_get_htotal();
    pGeode->FBgfxdisplaytiming.wHActive = gfx_get_hactive();
    pGeode->FBgfxdisplaytiming.wHSyncStart = gfx_get_hsync_start();
    pGeode->FBgfxdisplaytiming.wHSyncEnd = gfx_get_hsync_end();
    pGeode->FBgfxdisplaytiming.wHBlankStart = gfx_get_hblank_start();
    pGeode->FBgfxdisplaytiming.wHBlankEnd = gfx_get_hblank_end();
    pGeode->FBgfxdisplaytiming.wVTotal = gfx_get_vtotal();
    pGeode->FBgfxdisplaytiming.wVActive = gfx_get_vactive();
    pGeode->FBgfxdisplaytiming.wVSyncStart = gfx_get_vsync_start();
    pGeode->FBgfxdisplaytiming.wVSyncEnd = gfx_get_vsync_end();
    pGeode->FBgfxdisplaytiming.wVBlankStart = gfx_get_vblank_start();
    pGeode->FBgfxdisplaytiming.wVBlankEnd = gfx_get_vblank_end();
    pGeode->FBgfxdisplaytiming.wPolarity = gfx_get_sync_polarities();

    /* Save Display offset */
    pGeode->FBDisplayOffset = gfx_get_display_offset();

    DEBUGMSG(1, (0, X_INFO, "GXEnterGraphics(2)!\n"));

    if (pGeode->useVGA) {
      vgaHWPtr pvgaHW = VGAHWPTR(pScrni);
      pGeode->FBBIOSMode = pvgaHW->readCrtc(pvgaHW, 0x040);
    }

    /* Save the current Compression state */
    pGeode->FBCompressionEnable = gfx_get_compression_enable();
    pGeode->FBCompressionOffset = gfx_get_compression_offset();
    pGeode->FBCompressionPitch = gfx_get_compression_pitch();
    pGeode->FBCompressionSize = gfx_get_compression_size();

    /* Save Cursor offset */
    pGeode->FBCursorOffset = gfx_get_cursor_offset();

#if defined(PNL_SUP)
    /* Save the Panel state */
    DEBUGMSG(1, (0, X_INFO, "GXEnterGraphics(3)!\n"));
    Pnl_SavePanelState();
#endif

    /* only if comming from VGA */
   if (pGeode->useVGA && pGeode->FBVGAActive) {
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

        gfx_outb(MDC_SEQUENCER_INDEX, MDC_SEQUENCER_CLK_MODE);
        sequencer = gfx_inb(MDC_SEQUENCER_DATA);
        sequencer |= MDC_CLK_MODE_SCREEN_OFF;
        gfx_outb(MDC_SEQUENCER_DATA, sequencer);

        gfx_delay_milliseconds(1);

        /* BLANK THE VGA DISPLAY */
        gfx_outw(MDC_SEQUENCER_INDEX, MDC_SEQUENCER_RESET);
        sequencer = gfx_inb(MDC_SEQUENCER_DATA);
        sequencer &= ~MDC_RESET_VGA_DISP_ENABLE;
        gfx_outb(MDC_SEQUENCER_DATA, sequencer);

        gfx_delay_milliseconds(1);
    }

    gx_clear_fb(pScrni);

    if (!GXSetMode(pScrni, pScrni->currentMode)) {
        return FALSE;
    }

    gx_enable_dac_power(pScrni);
    DEBUGMSG(1, (0, X_INFO, "GXEnterGraphics(4)!\n"));
    return TRUE;
}

void
gx_enable_dac_power(ScrnInfoPtr pScrni)
{
    /* enable the DAC POWER */
    gfx_write_vid32(RCDF_VID_MISC,
        gfx_read_vid32(RCDF_VID_MISC) & RCDF_GAMMA_BYPASS_BOTH);
}

void
gx_disable_dac_power(ScrnInfoPtr pScrni)
{
    /* disable the DAC POWER */
    gfx_write_vid32(RCDF_VID_MISC,
        RCDF_DAC_POWER_DOWN | RCDF_ANALOG_POWER_DOWN |
        (gfx_read_vid32(RCDF_VID_MISC) & RCDF_GAMMA_BYPASS_BOTH));
}

/*----------------------------------------------------------------------------
 * GXLeaveGraphics:
 *
 * Description	:This function will restore the displaymode parameters
 * 				 and switches the VGA mode
 *
 * Parameters.
 *    	pScrn 	:Screen information will be stored in this structure.
 * 		pScrni 	:Pointer to the screenInfo structure.
 * 												
 * Returns		:none.
 *
 * Comments		: gfx_vga_mode_switch() will start and end the switching
 *			     based on the arguments 0 or 1.soft_vga is disabled in
 *               this function.
 *----------------------------------------------------------------------------
 */
static void
GXLeaveGraphics(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    gfx_wait_until_idle();

    /* Restore VG registers */
    gfx_set_display_timings(pGeode->FBgfxdisplaytiming.wBpp,
        pGeode->FBgfxdisplaytiming.wPolarity,
        pGeode->FBgfxdisplaytiming.wHActive,
        pGeode->FBgfxdisplaytiming.wHBlankStart,
        pGeode->FBgfxdisplaytiming.wHSyncStart,
        pGeode->FBgfxdisplaytiming.wHSyncEnd,
        pGeode->FBgfxdisplaytiming.wHBlankEnd,
        pGeode->FBgfxdisplaytiming.wHTotal,
        pGeode->FBgfxdisplaytiming.wVActive,
        pGeode->FBgfxdisplaytiming.wVBlankStart,
        pGeode->FBgfxdisplaytiming.wVSyncStart,
        pGeode->FBgfxdisplaytiming.wVSyncEnd,
        pGeode->FBgfxdisplaytiming.wVBlankEnd,
        pGeode->FBgfxdisplaytiming.wVTotal,
        pGeode->FBgfxdisplaytiming.dwDotClock);

    gfx_set_compression_enable(0);

    /* Restore the previous Compression state */
    if (pGeode->FBCompressionEnable) {
        gfx_set_compression_offset(pGeode->FBCompressionOffset);
        gfx_set_compression_pitch(pGeode->FBCompressionPitch);
        gfx_set_compression_size(pGeode->FBCompressionSize);
        gfx_set_compression_enable(1);
    }

    gfx_set_display_pitch(pGeode->FBgfxdisplaytiming.wPitch);

    gfx_set_display_offset(pGeode->FBDisplayOffset);

    /* Restore Cursor */
    gfx_set_cursor_position(pGeode->FBCursorOffset, 0, 0, 0, 0);

   if (pGeode->useVGA) {
    pGeode->vesa->pInt->num = 0x10;
    pGeode->vesa->pInt->ax = 0x0 | pGeode->FBBIOSMode;
    pGeode->vesa->pInt->bx = 0;
    xf86ExecX86int10(pGeode->vesa->pInt);
    gfx_delay_milliseconds(3);
   }

    GXRestore(pScrni);

    gx_enable_dac_power(pScrni);
}

/*----------------------------------------------------------------------------
 * GXCloseScreen.
 *
 * Description		:This function will restore the original mode
 *					 and also it unmap video memory
 *
 * Parameters.
 *  	ScrnIndex	:Screen index value of the screen will be closed.
 * 		pScrn    	:Pointer to the screen structure.
 *	
 * 												
 * Returns			:TRUE on success and FALSE on Failure.
 *
 * Comments			:none.
 *----------------------------------------------------------------------------
 */
static Bool
GXCloseScreen(int scrnIndex, ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->ShadowPtr)
        xfree(pGeode->ShadowPtr);

    DEBUGMSG(0, (scrnIndex, X_PROBED, "GXCloseScreen %d\n", pScrni->vtSema));
    if (pScrni->vtSema)
        GXLeaveGraphics(pScrni);

    if (pGeode->AccelInfoRec)
        XAADestroyInfoRec(pGeode->AccelInfoRec);

    if (pGeode->AccelImageWriteBuffers) {
#if GX_USE_OFFSCRN_MEM
        xfree(pGeode->AccelImageWriteBuffers[0]);
#endif
        xfree(pGeode->AccelImageWriteBuffers);
        pGeode->AccelImageWriteBuffers = NULL;
    }

    if (pGeode->AccelColorExpandBuffers) {
        xfree(pGeode->AccelColorExpandBuffers);
        pGeode->AccelColorExpandBuffers = NULL;
    }

#ifdef XF86EXA
    if (pGeode->pExa) {
        exaDriverFini(pScrn);
        xfree(pGeode->pExa);
        pGeode->pExa = NULL;
    }
#endif

    pScrni->vtSema = FALSE;

    GXUnmapMem(pScrni);

    if (pGeode && (pScrn->CloseScreen = pGeode->CloseScreen)) {
        pGeode->CloseScreen = NULL;
        return ((*pScrn->CloseScreen) (scrnIndex, pScrn));
    }

    return TRUE;
}

#ifdef DPMSExtension
static void
GXPanelPower(int enable)
{
    unsigned long power = READ_VID32(RCDF_POWER_MANAGEMENT);

    if (enable != 0)
        power |= RCDF_PM_PANEL_POWER_ON;
    else
        power &= ~RCDF_PM_PANEL_POWER_ON;

    WRITE_VID32(RCDF_POWER_MANAGEMENT, power);
}

/*----------------------------------------------------------------------------
 * GXDPMSSet.
 *
 * Description	:This function sets geode into Power Management
 *               Signalling mode.				
 *
 * Parameters.
 * 	pScrni	 	:Pointer to screen info strucrure.
 * 	mode        :Specifies the power management mode.
 *	 												
 * Returns		:none.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static void
GXDPMSSet(ScrnInfoPtr pScrni, int mode, int flags)
{
    GeodeRec *pGeode;

    pGeode = GEODEPTR(pScrni);

    DEBUGMSG(1, (0, X_INFO, "GXDPMSSet! %d %d\n", mode, flags));

    /* Check if we are actively controlling the display */
    if (!pScrni->vtSema) {
        ErrorF("GXDPMSSet called when we not controlling the VT!\n");
        return;
    }

    switch (mode) {
    case DPMSModeOn:
        /* Screen: On; HSync: On; VSync: On */
        GFX(set_crt_enable(CRT_ENABLE));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerUp();
            GXPanelPower(1);
        }
#endif
        break;

    case DPMSModeStandby:
        /* Screen: Off; HSync: Off; VSync: On */
        GFX(set_crt_enable(CRT_STANDBY));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerDown();
            GXPanelPower(0);
        }
#endif
        break;

    case DPMSModeSuspend:
        /* Screen: Off; HSync: On; VSync: Off */
        GFX(set_crt_enable(CRT_SUSPEND));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerDown();
            GXPanelPower(0);
        }
#endif
        break;

    case DPMSModeOff:
        /* Screen: Off; HSync: Off; VSync: Off */
        GFX(set_crt_enable(CRT_DISABLE));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerDown();
            GXPanelPower(0);
        }
#endif
        break;
    }
}
#endif

/*----------------------------------------------------------------------------
 * GXScreenInit.
 *
 * Description	:This function will be called at the each ofserver
 *   			 generation.				
 *
 * Parameters.
 *   scrnIndex  :Specfies the screenindex value during generation.
 *   pScrn	 	:Pointer to screen info strucrure.
 * 	 argc       :parameters for command line arguments count
 *	 argv       :command line arguments if any it is not used.  												
 *
 * Returns		:none.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static Bool
GXScreenInit(int scrnIndex, ScreenPtr pScrn, int argc, char **argv)
{
    int i, l, bytpp, size, fbsize, fboffset, fbavail;
    int pitch, displayWidth, virtualX, virtualY;
    int HDisplay, VDisplay, maxHDisplay, maxVDisplay, maxX, maxY;
    unsigned char *FBStart;
    unsigned char **ap, *bp;
    DisplayModePtr p;
    GeodeRec *pGeode;
    VisualPtr visual;
    BoxRec AvailBox;
    RegionRec OffscreenRegion;
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
    Bool Inited = FALSE;

    pGeode = GXGetRec(pScrni);

   if (pGeode->useVGA) {
    if (!vgaHWGetHWRec(pScrni))
        return FALSE;
    if (!vgaHWMapMem(pScrni))
        return FALSE;

    vgaHWGetIOBase(VGAHWPTR(pScrni));
   }

    if (!GXMapMem(pScrni))
        return FALSE;

    /* If compression is not turned on - adjust the pitch to be linear */
    if (pGeode->Compression)
      pGeode->Pitch = GXCalculatePitchBytes(pScrni->virtualX, pScrni->bitsPerPixel);
    else
      pGeode->Pitch = ((pScrni->virtualX + 3) & ~3) * (pScrni->bitsPerPixel >> 3);

    pGeode->AccelPitch = pGeode->Pitch;
    bytpp = (pScrni->bitsPerPixel + 7) / 8;

    /* start of framebuffer for accels */
    fboffset = 0;
    /* 0x4000 for gfx_gu2_scratch_buffer */
    fbavail = pGeode->FBAvail - 0x4000;

#ifdef V4L2_VIDEO_BFR_SZ
    fbavail -= V4L2_VIDEO_BFR_SZ;
#endif

    /* allocate display frame buffer at zero offset */
    fbsize = pScrni->virtualY * pGeode->Pitch;
    pGeode->FBSize = fbsize;

    pGeode->CursorSize = 16 * 64;      /* 64x64 */
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
            pScrni->PointerMoved = GXPointerMoved;
        }

        if (pGeode->ShadowPtr == NULL) {
            pGeode->ShadowPitch =
                BitmapBytePad(pScrni->bitsPerPixel * virtualX);
            size = pGeode->ShadowPitch * virtualY;
            pGeode->ShadowPtr = xalloc(size);
            if (pGeode->ShadowPtr != NULL) {
                pGeode->ShadowSize = size;
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
        pGeode->CBPitch = 512 + 32;
        pGeode->CBSize = 512 + 32;
        size = maxY * pGeode->CBPitch;
        DEBUGMSG(1, (0, X_PROBED, "CB %#x size %#x (%d*%d)\n", fboffset, size,
                maxY, pGeode->CBPitch));
        if (size <= fbavail) {
            pGeode->CBOffset = fboffset;
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
#if XF86EXA
    if (!pGeode->NoAccel && pGeode->useEXA) {   /* exa acceleration enabled */
        if (!(pGeode->pExa = xnfcalloc(sizeof(ExaDriverRec), 1))) {
            xf86DrvMsg(scrnIndex, X_ERROR,
                "No ExaDriverRec Memory, disabled\n");
            pGeode->NoAccel = TRUE;
        } else {
            if ((size = pGeode->exaBfrSz) > 0 && size <= fbavail) {
                pGeode->exaBfrOffset = fboffset;
                fboffset += size;
                fbavail -= size;
            }
        }
    }
#endif

    if (!pGeode->NoAccel && !pGeode->useEXA) {  /* xaa acceleration enabled */
        if (pGeode->NoOfImgBuffers > 0) {
            pGeode->AccelImageWriteBuffers = NULL;
            pitch = pGeode->AccelPitch;
            size = pitch * pGeode->NoOfImgBuffers;
#if !GX_USE_OFFSCRN_MEM
            if (size <= fbavail) {
                bp = (unsigned char *)pGeode->FBBase + fboffset;
                l = sizeof(pGeode->AccelImageWriteBuffers[0]) *
                    pGeode->NoOfImgBuffers;
                ap = (unsigned char **)xalloc(l);
                if (ap != NULL) {
                    for (i = 0; i < pGeode->NoOfImgBuffers; ++i) {
                        ap[i] = bp;
                        DEBUGMSG(1, (scrnIndex, X_PROBED, "img line  %d %p\n",
                                i, ap[i]));
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
                        DEBUGMSG(1, (scrnIndex, X_PROBED, "img line  %d %x\n",
                                i, ap[i]));
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
            if (size <= fbavail) {
                bp = (unsigned char *)pGeode->FBBase + fboffset;
                l = sizeof(pGeode->AccelColorExpandBuffers[0]) *
                    pGeode->NoOfColorExpandLines;
                ap = (unsigned char **)xalloc(l);
                if (ap != NULL) {
                    for (i = 0; i < pGeode->NoOfColorExpandLines; ++i) {
                        ap[i] = bp;
                        DEBUGMSG(1, (scrnIndex, X_PROBED, "clr line  %d %p\n",
                                i, ap[i]));
                        bp += pitch;
                    }
                    pGeode->AccelColorExpandBuffers = ap;
                    fboffset += size;
                    fbavail -= size;
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
    if (!GXEnterGraphics(pScrn, pScrni))
        return FALSE;

    pScrni->virtualX = virtualX;
    pScrni->virtualY = virtualY;

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(1)!\n"));

    /* Reset visual list */
    miClearVisualTypes();
    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(2)!\n"));

    /* Setup the visual we support */
    if (pScrni->bitsPerPixel > 8) {
        DEBUGMSG(1, (scrnIndex, X_PROBED,
                "miSetVisualTypes %d %X %X %X\n",
                pScrni->depth, TrueColorMask,
                pScrni->rgbBits, pScrni->defaultVisual));

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
    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(3)!\n"));

    /* Set for RENDER extensions */
    miSetPixmapDepths();

    /* Call the framebuffer layer's ScreenInit function, and fill in other
     * * pScrn fields.
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

    GXRotationInit(pScrni);
    GXAdjustFrame(scrnIndex, pScrni->frameX0, pScrni->frameY0, 0);

    if (!pGeode->NoAccel) {
        if (!pGeode->useEXA) {
            AvailBox.x1 = 0;           /* SET UP GRAPHICS MEMORY AVAILABLE FOR PIXMAP CACHE */
            AvailBox.y1 =
                (fboffset + pGeode->AccelPitch - 1) / pGeode->AccelPitch;
            AvailBox.x2 = displayWidth;
            AvailBox.y2 = (fboffset + fbavail) / pGeode->AccelPitch;

            DEBUGMSG(1, (scrnIndex, X_PROBED,
                    "Memory manager initialized to (%d,%d) (%d,%d)\n",
                    AvailBox.x1, AvailBox.y1, AvailBox.x2, AvailBox.y2));

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
        }
#if XF86EXA
        else if (pGeode->pExa) {
            ExaDriverPtr pExa = pGeode->pExa;

            pExa->memoryBase = pGeode->FBBase;
            pExa->offScreenBase = fboffset;
            pExa->memorySize = fboffset + fbavail;
            pExa->pixmapOffsetAlign = 32;
            pExa->pixmapPitchAlign = 32;
            pExa->flags = EXA_OFFSCREEN_PIXMAPS;
            pExa->maxX = pGeode->maxWidth - 1;
            pExa->maxY = pGeode->maxHeight - 1;
        }
#endif
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(4)!\n"));
    xf86SetBlackWhitePixels(pScrn);

    if (!pGeode->ShadowFB) {
        GXDGAInit(pScrn);
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(5)!\n"));
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

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(6)!\n"));
    if (!pGeode->NoAccel) {
        GXAccelInit(pScrn);
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(7)!\n"));
    miInitializeBackingStore(pScrn);
    xf86SetBackingStore(pScrn);
    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(8)!\n"));
    /* Initialise software cursor */
    miDCInitialize(pScrn, xf86GetPointerScreenFuncs());
    /* Initialize HW cursor layer.
     * * Must follow software cursor initialization
     */
    if (pGeode->HWCursor) {
        if (!GXHWCursorInit(pScrn))
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                "Hardware cursor initialization failed\n");
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(9)!\n"));
    /* Setup default colourmap */
    if (!miCreateDefColormap(pScrn)) {
        return FALSE;
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(10)!\n"));
    if (pScrni->bitsPerPixel == 8) {
        /* Initialize colormap layer.
         * * Must follow initialization of the default colormap
         */
        if (!xf86HandleColormaps(pScrn, 256, 8,
                GXLoadPalette, NULL,
                CMAP_PALETTED_TRUECOLOR | CMAP_RELOAD_ON_MODE_SWITCH)) {
            return FALSE;
        }
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(11)!\n"));

    if (pGeode->ShadowFB) {
        DEBUGMSG(1, (0, X_INFO, "Shadowed, Rotate=%d, NoAccel=%d\n",
                pGeode->Rotate, pGeode->NoAccel));
        GXShadowFBInit(pScrn, pGeode, bytpp);
    }
#ifdef DPMSExtension
    xf86DPMSInit(pScrn, GXDPMSSet, 0);
#endif
    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(12)!\n"));

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(13)!\n"));
    GXInitVideo(pScrn);                /* needed for video */
    /* Wrap the screen's CloseScreen vector and set its
     * SaveScreen vector 
     */
    pGeode->CloseScreen = pScrn->CloseScreen;
    pScrn->CloseScreen = GXCloseScreen;

    pScrn->SaveScreen = GXSaveScreen;
    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(14)!\n"));

    /* Report any unused options */
    if (serverGeneration == 1) {
        xf86ShowUnusedOptions(pScrni->scrnIndex, pScrni->options);
    }

    DEBUGMSG(1, (0, X_INFO, "GXScreenInit(15)!\n"));
    return TRUE;
}

/*----------------------------------------------------------------------------
 * GXSwitchMode.
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
GXSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
    DEBUGMSG(1, (0, X_INFO, "GXSwitchMode!\n"));
    return GXSetMode(xf86Screens[scrnIndex], pMode);
}

/*----------------------------------------------------------------------------
 * GXAdjustFrame.
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
GXAdjustFrame(int scrnIndex, int x, int y, int flags)
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
    offset =
        pGeode->FBOffset + newY * pGeode->Pitch +
        newX * (pScrni->bitsPerPixel >> 3);
    gfx_set_display_offset(offset);
}

/*----------------------------------------------------------------------------
 * GXEnterVT.
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
GXEnterVT(int scrnIndex, int flags)
{
    DEBUGMSG(1, (0, X_INFO, "GXEnterVT!\n"));
    return GXEnterGraphics(NULL, xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * GXLeaveVT.
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
GXLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->PrevDisplayOffset = gfx_get_display_offset();
    DEBUGMSG(1, (0, X_INFO, "GXLeaveVT!\n"));
    GXLeaveGraphics(xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * GXFreeScreen.
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
GXFreeScreen(int scrnIndex, int flags)
{
    DEBUGMSG(1, (0, X_INFO, "GXFreeScreen!\n"));
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
        vgaHWFreeHWRec(xf86Screens[scrnIndex]);

    GXFreeRec(xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * GXValidMode.
 *
 * Description	:This function checks if a mode is suitable for selected
 *               chipset.
 * Parameters.
 *    scrnIndex :Specfies the screen index value.
 *     pMode	:Pointer to the screen mode structure..
 * 	 verbose    :not used for implementation.						
 *     flags    :not used for implementation
 *
 * Returns		:MODE_OK if the specified mode is supported or
 *               MODE_NO_INTERLACE.
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static int
GXValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
    unsigned int total_memory_required;
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    int ret = -1;
    unsigned int ptch;

    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(1, (0, X_NONE, "GeodeValidateMode: %dx%d %d %d\n",
            pMode->CrtcHDisplay, pMode->CrtcVDisplay,
            pScrni->bitsPerPixel, GXGetRefreshRate(pMode)));
    if (pGeode->CustomMode == 0) {

#if defined(PNL_SUP)
        if (pGeode->Panel != 0) {
	    DEBUGMSG(1, (0, X_NONE, "crtcH = %d, FPBX=%d, CrtcV=%d FPBY=%d\n",
				    pMode->CrtcHDisplay, pGeode->FPBX,
				    pMode->CrtcVDisplay, pGeode->FPBY));
            if (pMode->CrtcHDisplay > pGeode->FPBX ||
                pMode->CrtcVDisplay > pGeode->FPBY ||
                gfx_is_panel_mode_supported(pGeode->FPBX, pGeode->FPBY,
                    pMode->CrtcHDisplay, pMode->CrtcVDisplay,
                    pScrni->bitsPerPixel) < 0)
                return MODE_NOMODE;
        }
#endif /* PNL_SUP */

        DEBUGMSG(1, (0, X_NONE, "CRT mode\n"));
        if (pMode->Flags & V_INTERLACE)
            return MODE_NO_INTERLACE;

        ret = gfx_is_display_mode_supported(pMode->CrtcHDisplay,
            pMode->CrtcVDisplay,
            pScrni->bitsPerPixel, GXGetRefreshRate(pMode));
        if (ret < 0)
            return MODE_NOMODE;
    }

    if (pGeode->Compression)
	ptch = GXCalculatePitchBytes(pMode->CrtcHDisplay, pScrni->bitsPerPixel);
    else
	ptch = ((pMode->CrtcHDisplay + 3) & ~3) * (pScrni->bitsPerPixel >> 3);

    total_memory_required = ptch * pMode->CrtcVDisplay;

    DEBUGMSG(1, (0, X_NONE, "Total Mem %X %lX\n",
            total_memory_required, pGeode->FBAvail));

    if (total_memory_required > pGeode->FBAvail)
        return MODE_MEM;

    return MODE_OK;
}

/*----------------------------------------------------------------------------
 * GXLoadPalette.
 *
 * Description:		This function sets the  palette entry used for graphics 
 * 					data
 *
 * Parameters.
 *   	pScrni:		Points the screeninfo structure.
 *     	numColors:	Specifies the no of colors it supported.
 * 	 	indizes:	This is used get index value .						
 *     	LOCO:		to be added.
 *     	pVisual:	to be added.
 *
 * Returns:			MODE_OK if the specified mode is supported or
 *          	 	MODE_NO_INTERLACE.
 *
 * Comments:		none.
 *----------------------------------------------------------------------------
 */
static void
GXLoadPalette(ScrnInfoPtr pScrni,
    int numColors, int *indizes, LOCO * colors, VisualPtr pVisual)
{
    int i, index, color;

    for (i = 0; i < numColors; i++) {
        index = indizes[i] & 0xFF;
        color = (((unsigned long)(colors[index].red & 0xFF)) << 16) |
            (((unsigned long)(colors[index].green & 0xFF)) << 8) |
            ((unsigned long)(colors[index].blue & 0xFF));
        DEBUGMSG(0, (0, X_NONE, "GXLoadPalette: %d %d %X\n",
                numColors, index, color));

        GFX(set_display_palette_entry(index, color));
    }
}

static Bool
GXMapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    gfx_virt_regptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_MMIO, (unsigned int)
        gfx_get_cpu_register_base(), pGeode->cpu_reg_size);

    if (pGeode->DetectedChipSet & GX) {
        gfx_virt_gpptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
            VIDMEM_MMIO, (unsigned int)
            gfx_get_graphics_register_base(), pGeode->gp_reg_size);
    } else {
        gfx_virt_spptr = gfx_virt_regptr;
    }

    gfx_virt_vidptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_MMIO, (unsigned int)
        gfx_get_vid_register_base(), pGeode->vid_reg_size);

    gfx_virt_fbptr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_FRAMEBUFFER, pGeode->FBLinearAddr, pGeode->FBAvail);

    XpressROMPtr = (unsigned char *)xf86MapVidMem(pScrni->scrnIndex,
        VIDMEM_FRAMEBUFFER, 0xF0000, 0x10000);

    pGeode->FBBase = gfx_virt_fbptr;

    DEBUGMSG(1, (0, X_NONE, "Set mode %p %p %p %p %p\n",
            gfx_virt_regptr,
            gfx_virt_gpptr, gfx_virt_spptr, gfx_virt_vidptr, gfx_virt_fbptr));

    /* CHECK IF REGISTERS WERE MAPPED SUCCESSFULLY */
    if ((!gfx_virt_regptr) ||
        (!gfx_virt_gpptr) || (!gfx_virt_vidptr) || (!gfx_virt_fbptr)) {
        DEBUGMSG(1, (0, X_NONE, "Could not map hardware registers.\n"));
        return (FALSE);
    }

    DEBUGMSG(1, (0, X_NONE, "adapter info %lx %lx %lx %p\n",
            pGeode->cpu_version,
            pGeode->vid_version,
            pGeode->FBAvail, pGeode->FBBase));

    return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
GXUnmapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* unmap all the memory map's */
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_regptr, pGeode->cpu_reg_size);
    if (pGeode->DetectedChipSet & GX) {
        xf86UnMapVidMem(pScrni->scrnIndex,
            gfx_virt_gpptr, pGeode->gp_reg_size);
    }
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_vidptr, pGeode->vid_reg_size);
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_fbptr, pGeode->FBAvail);
    return TRUE;
}

/* End of file */
