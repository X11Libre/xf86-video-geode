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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "mipointer.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "miscstruct.h"
#include "micmap.h"
#include "vbe.h"
#include "fb.h"
#include "randrstr.h"

#include "amd.h"
#include "gfx_defs.h"
#include "gfx_regs.h"
#include "panel.h"

/* Bring in VGA functions */
#include "amd_gx_vga.c"

/* Chipset types */

#define GX1 0x1
#define GX  0x2

#define GX_CRT 0x04 | GX
#define GX_TFT 0x08 | GX

#define GX_MIN_PITCH 1024
#define GX_MAX_PITCH 8192
#define GX_MAX_WIDTH  1600
#define GX_MIN_HEIGHT  400
#define GX_MAX_HEIGHT 1200
#define GX_CB_PITCH   544
#define GX_CB_SIZE    544

static void GXPointerMoved(int index, int x, int y);

extern OptionInfoRec GX_GeodeOptions[];

extern const char *amdVgahwSymbols[];
extern const char *amdVbeSymbols[];
extern const char *amdInt10Symbols[];
extern const char *amdFbSymbols[];
extern const char *amdXaaSymbols[];
extern const char *amdExaSymbols[];
extern const char *amdRamdacSymbols[];

unsigned char *XpressROMPtr;

const char *gxShadowSymbols[] = {
    "ShadowInit",
    "ShadowSetup",
    "ShadowAdd",
    NULL
};

static inline void
gx_enable_dac_power(void)
{
    gfx_write_vid32(RCDF_VID_MISC,
	gfx_read_vid32(RCDF_VID_MISC) & RCDF_GAMMA_BYPASS_BOTH);
}

static inline void
gx_disable_dac_power(void)
{
    gfx_write_vid32(RCDF_VID_MISC,
	RCDF_DAC_POWER_DOWN | RCDF_ANALOG_POWER_DOWN |
	(gfx_read_vid32(RCDF_VID_MISC) & RCDF_GAMMA_BYPASS_BOTH));
}

/* Parser for the flatpanel info */

static inline int
GXGetFPInfo(const char *str, int *width, int *height)
{

    int ret = sscanf(str, "%dx%d", width, height);

    return (ret == 2) ? 0 : 1;
}

/* I borrowed this function from the i830 driver - its much better 
   then what we had before
*/

static int
GXGetRefreshRate(DisplayModePtr pMode)
{
    if (pMode->VRefresh)
	return (int)(pMode->VRefresh + 0.5);

    return (int)(pMode->Clock * 1000.0 / pMode->HTotal / pMode->VTotal + 0.5);
}

static int
GXCalculatePitchBytes(unsigned int width, unsigned int bpp)
{

    int delta = width * (bpp >> 3);

    /* Less then 640 has doubling enabled */

    if (width < 640)
	delta <<= 1;

    /* Calculate the pitch (compression rquires a power of 2) */

    if (delta > 4096)
	delta = 8192;
    else if (delta > 2048)
	delta = 4096;
    else if (delta > 1024)
	delta = 2048;
    else
	delta = 1024;

    return delta;
}

static void
GXFreeRec(ScrnInfoPtr pScrni)
{
    if (pScrni->driverPrivate != NULL) {
	xfree(pScrni->driverPrivate);
	pScrni->driverPrivate = NULL;
    }
}

static void
GXInitEXAMemory(ScrnInfoPtr pScrni, unsigned int *offset, unsigned int *avail)
{
    GeodePtr pGeode = GEODEPTR(pScrni);

    if (pGeode->exaBfrSz > 0 && pGeode->exaBfrSz <= *avail) {
	pGeode->exaBfrOffset = *offset;
	*offset += pGeode->exaBfrOffset;
	*avail -= pGeode->exaBfrOffset;
    }
}

static void
GXInitXAAMemory(ScrnInfoPtr pScrni, unsigned int *offset, unsigned int *avail)
{
    GeodePtr pGeode = GEODEPTR(pScrni);
    unsigned int size, i, pitch;

    /* XXX - FIXME - What if we are out of room?  Then what? */
    /* For now, we NULL them all out.                        */

    if (pGeode->NoOfImgBuffers > 0) {
	size = pGeode->displayPitch * pGeode->NoOfImgBuffers;
	if (size <= *avail) {
	    for (i = 0; i < pGeode->NoOfImgBuffers; i++) {
		pGeode->AccelImageWriteBuffers[i] = pGeode->FBBase + *offset;
		*offset += pGeode->displayPitch;
		*avail -= pGeode->displayPitch;
	    }
	} else {
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Not enough memory for image write buffers.\n");

	    for (i = 0; i < pGeode->NoOfImgBuffers; i++)
		pGeode->AccelImageWriteBuffers[i] = NULL;
	}
    }

    if (pGeode->NoOfColorExpandLines > 0) {
	pitch = ((pGeode->displayPitch + 31) >> 5) << 2;
	size = pitch * pGeode->NoOfColorExpandLines;

	if (size <= *avail) {
	    for (i = 0; i < pGeode->NoOfColorExpandLines; i++) {
		pGeode->AccelColorExpandBuffers[i] = pGeode->FBBase + *offset;
		*offset += pitch;
		*avail -= pitch;
	    }
	} else {
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Not enough memory for color expansion buffers.\n");

	    for (i = 0; i < pGeode->NoOfImgBuffers; i++)
		pGeode->AccelColorExpandBuffers[i] = NULL;
	}
    }
}

static Bool
GXAllocateMemory(ScreenPtr pScrn, ScrnInfoPtr pScrni, int rotate)
{
    GeodePtr pGeode = GEODEPTR(pScrni);

    unsigned int fboffset, fbavail;
    unsigned int size;
    unsigned int bytpp = (pScrni->bitsPerPixel + 7) / 8;
    BOOL ret = TRUE;

    if (pGeode->tryCompression)
	pGeode->displayPitch =
	    GXCalculatePitchBytes(pScrni->virtualX, pScrni->bitsPerPixel);
    else
	pGeode->displayPitch =
	    ((pScrni->virtualX + 3) & ~3) * (pScrni->bitsPerPixel >> 3);

    pGeode->Pitch = pGeode->displayPitch;
    pGeode->displayWidth = pGeode->displayPitch / bytpp;
    pScrni->displayWidth = pGeode->displayWidth;

    fbavail = pGeode->FBAvail - 0x4000;

    pGeode->displayOffset = fboffset = 0;
    pGeode->displaySize = pScrni->virtualY * pGeode->displayPitch;

    fbavail -= pGeode->displaySize;
    fboffset += pGeode->displaySize;

    if (pGeode->tryCompression) {
	size = pScrni->virtualY * GX_CB_PITCH;

	if (size <= fbavail) {
	    pGeode->CBOffset = fboffset;

	    fboffset += size;
	    fbavail -= size;

	    pGeode->Compression = TRUE;
	} else {
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Not enough memory for compression\n");
	    pGeode->Compression = FALSE;
	}
    }

    if (pGeode->tryHWCursor) {
	pGeode->CursorSize = 1024;

	if (pGeode->CursorSize <= fbavail) {
	    pGeode->CursorStartOffset = fboffset;
	    fboffset += pGeode->CursorSize;
	    fbavail -= pGeode->CursorSize;
	    pGeode->HWCursor = TRUE;
	} else {
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Not enough memory for the hardware cursor\n");
	    pGeode->HWCursor = FALSE;
	}
    }

    if (!pGeode->NoAccel) {
	if (pGeode->useEXA)
	    GXInitEXAMemory(pScrni, &fboffset, &fbavail);
	else
	    GXInitXAAMemory(pScrni, &fboffset, &fbavail);
    }

    pGeode->shadowSize = 0;

    if (rotate != RR_Rotate_0) {
	if (rotate & (RR_Rotate_90 | RR_Rotate_270))
	    size = pGeode->displayPitch * pScrni->virtualX;
	else
	    size = pGeode->displayPitch * pScrni->virtualY;

	if (size <= fbavail) {
	    pGeode->shadowOffset = fboffset;
	    pGeode->shadowSize = size;

	    fboffset += size;
	    fbavail -= size;
	} else {
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Not enough memory for the shadow framebuffer\n");
	    ret = FALSE;
	}
    }

    /* Set the memory available for the offscreen pixmaps */

    if (!pGeode->NoAccel) {

	if (pGeode->useEXA && pGeode->pExa) {
	    ExaDriverPtr pExa = pGeode->pExa;

	    pExa->offScreenBase = fboffset;
	    pExa->memorySize = fboffset + fbavail;
	}

	/* XXX - We don't use XAA if we are rotated, so this should be fine.  Changing the 
	 * resolution will hurt us though */

	if (!pGeode->useEXA) {

	    if (!xf86FBManagerRunning(pScrn)) {

		unsigned int offset = fboffset;
		unsigned int avail = fbavail;
		RegionRec OffscreenRegion;
		BoxRec AvailBox;

		/* We only get one shot at allocating offscreen memory for XAA */
		/* That means if the mode changes with RandR, we'll probably be out of luck. 
		 * we can compensate for rotation by offsetting the start of offscreen memory to account
		 * for the shadow framebuffer even if we're not using it right now 
		 */

		if (pGeode->shadowSize == 0) {
		    size = (pScrn->width * bytpp) * pScrni->virtualX;

		    offset += size;
		    avail -= size;
		}

		AvailBox.x1 = 0;
		AvailBox.y1 =
		    (offset + pGeode->displayPitch -
		    1) / pGeode->displayPitch;

		AvailBox.x2 = pGeode->displayWidth;
		AvailBox.y2 = (offset + avail) / pGeode->displayPitch;

		if (AvailBox.y1 < AvailBox.y2) {
		    REGION_INIT(pScrn, &OffscreenRegion, &AvailBox, 2);

		    if (!xf86InitFBManagerRegion(pScrn, &OffscreenRegion))
			xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
			    "Memory manager initialization failed.\n");

		    REGION_UNINIT(pScrn, &OffscreenRegion);
		} else
		    xf86DrvMsg(pScrni->scrnIndex, X_INFO,
			"Cache disabled - no offscreen memory available.\n");
	    } else
		xf86DrvMsgp(pScrni->scrnIndex, X_INFO,
		    "XAA offscreen memory has already been allocated.\n");
	}
    }
}

static Bool
GXSaveScreen(ScreenPtr pScrn, int mode)
{
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
    GeodePtr pGeode = GEODEPTR(pScrni);

    if (pGeode->useVGA && !pScrni->vtSema)
	return vgaHWSaveScreen(pScrn, mode);

    return TRUE;
}

static void
GXProbeDDC(ScrnInfoPtr pScrni, int index)
{
    vbeInfoPtr pVbe;

    if (xf86LoadSubModule(pScrni, "vbe")) {
	pVbe = VBEInit(NULL, index);
	ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
	vbeFree(pVbe);
    }
}

/* From the Intel driver */

static xf86MonPtr
GXDoDDC(ScrnInfoPtr pScrni, int index)
{
    vbeInfoPtr pVbe;
    xf86MonPtr info = NULL;

    if (xf86LoadSubModule(pScrni, "vbe")) {
	xf86LoaderReqSymLists(amdVbeSymbols, NULL);
	info = vbeDoEDID(pVbe, NULL);
	xf86PrintEDID(info);
	xf86SetDDCproperties(pScrni, info);
	vbeFree(pVbe);
    } else
	xf86DrvMsg(pScrni->scrnIndex, X_INFO,
	    "We cannot do DDC without VBE.\n");

    return info;
}

static Bool
GXMapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int index = pScrni->scrnIndex;

    gfx_virt_regptr = (unsigned char *)xf86MapVidMem(index, VIDMEM_MMIO,
	gfx_get_cpu_register_base(), pGeode->cpu_reg_size);

    if (pGeode->DetectedChipSet & GX)
	gfx_virt_gpptr = (unsigned char *)xf86MapVidMem(index, VIDMEM_MMIO,
	    gfx_get_graphics_register_base(), pGeode->gp_reg_size);
    else
	gfx_virt_spptr = gfx_virt_regptr;

    gfx_virt_vidptr = (unsigned char *)xf86MapVidMem(index, VIDMEM_MMIO,
	gfx_get_vid_register_base(), pGeode->vid_reg_size);

    gfx_virt_fbptr = (unsigned char *)xf86MapVidMem(index, VIDMEM_FRAMEBUFFER,
	pGeode->FBLinearAddr, pGeode->FBAvail);

    XpressROMPtr = xf86MapVidMem(index, VIDMEM_FRAMEBUFFER, 0xF0000, 0x10000);

    pGeode->FBBase = gfx_virt_fbptr;

    if ((!gfx_virt_regptr) || (!gfx_virt_gpptr) ||
	(!gfx_virt_vidptr) || (!gfx_virt_fbptr))
	return FALSE;

    xf86DrvMsg(index, X_INFO, "Found Geode %lx %lx %lx %p\n",
	pGeode->cpu_version, pGeode->vid_version,
	pGeode->FBAvail, pGeode->FBBase);

    return TRUE;
}

static Bool
GXPreInit(ScrnInfoPtr pScrni, int flags)
{
    GeodePtr pGeode;
    ClockRangePtr GeodeClockRange;
    OptionInfoRec *GeodeOptions = &GX_GeodeOptions[0];

    rgb defaultWeight = { 0, 0, 0 };
    int ret, modecnt;
    int maj, min;
    char *s, *panelgeo;
    char **modes;

    pGeode = pScrni->driverPrivate = xnfcalloc(sizeof(GeodeRec), 1);

    if (pGeode == NULL)
	return FALSE;

#if INT10_SUPPORT
    pGeode->vesa = xcalloc(sizeof(VESARec), 1);
#endif

    if (pScrni->numEntities != 1)
	return FALSE;

    pGeode->pEnt = xf86GetEntityInfo(pScrni->entityList[0]);

    if (pGeode->pEnt->resources)
	return FALSE;

    /* ISSUE - this won't work without VGA, but its too early to know if we can use VGA or not */

    if (flags & PROBE_DETECT) {
	GXProbeDDC(pScrni, pGeode->pEnt->index);
	return TRUE;
    }

    pGeode->cpu_version = gfx_detect_cpu();

    if ((pGeode->cpu_version & 0xFF) == GFX_CPU_REDCLOUD) {
	int ret;
	QQ_WORD msrValue;

	pGeode->DetectedChipSet = GX_CRT;

	ret = gfx_msr_read(RC_ID_DF, MBD_MSR_CONFIG, &msrValue);

	if (!ret)
	    pGeode->DetectedChipSet =
		((msrValue.low & RCDF_CONFIG_FMT_MASK) ==
		RCDF_CONFIG_FMT_FP) ? GX_TFT : GX_CRT;
    } else
	pGeode->DetectedChipSet = GX1;

    pGeode->vid_version = gfx_detect_video();
    pGeode->FBLinearAddr = gfx_get_frame_buffer_base();

    if (pGeode->DetectedChipSet & GX) {
	pGeode->cpu_reg_size = 0x4000;
	pGeode->gp_reg_size = 0x4000;
	pGeode->vid_reg_size = 0x4000;
    } else {
	pGeode->cpu_reg_size = 0x9000;
	pGeode->vid_reg_size = 0x1000;
    }

    /* Fill in the monitor information */
    pScrni->monitor = pScrni->confScreen->monitor;

    if (!xf86SetDepthBpp(pScrni, 8, 8, 8, Support24bppFb | Support32bppFb))
	return FALSE;

    switch (pScrni->depth) {
    case 8:
	pScrni->rgbBits = 8;
    case 16:
    case 24:
    case 32:
	break;
    default:
	xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
	    "The driver does not support %d as a depth.\n", pScrni->depth);
	return FALSE;
    }

    xf86PrintDepthBpp(pScrni);

    if (!xf86SetWeight(pScrni, defaultWeight, defaultWeight))
	return FALSE;

    if (!xf86SetDefaultVisual(pScrni, -1))
	return FALSE;

    pScrni->progClock = TRUE;
    xf86CollectOptions(pScrni, NULL);
    xf86ProcessOptions(pScrni->scrnIndex, pScrni->options, GeodeOptions);

    /* Set up our various options that may get reversed as we go on */
    pGeode->useVGA = TRUE;
    pGeode->FBVGAActive = FALSE;

    pGeode->tryHWCursor = TRUE;
    pGeode->tryCompression = TRUE;

    pGeode->NoAccel = FALSE;
    pGeode->CustomMode = FALSE;
    pGeode->useEXA = FALSE;

    if (pGeode->DetectedChipSet == GX_TFT)
	pGeode->Panel = TRUE;
    else if (pGeode->DetectedChipSet == GX_CRT)
	pGeode->Panel = FALSE;

    pGeode->NoOfImgBuffers = DEFAULT_IMG_LINE_BUFS;
    pGeode->NoOfColorExpandLines = DEFAULT_CLR_LINE_BUFS;
    pGeode->exaBfrSz = DEFAULT_EXA_SCRATCH_BFRSZ;

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOVGA, FALSE))
	pGeode->useVGA = FALSE;

    xf86GetOptValBool(GeodeOptions, GX_OPTION_HW_CURSOR,
	&pGeode->tryHWCursor);

    /* For compatability - allow SWCursor too */

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_SW_CURSOR, FALSE))
	pGeode->tryHWCursor = FALSE;

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOCOMPRESSION, FALSE))
	pGeode->tryCompression = FALSE;

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOACCEL, FALSE))
	pGeode->NoAccel = TRUE;

    xf86GetOptValInteger(GeodeOptions, GX_OPTION_OSM_IMG_BUFS,
	&(pGeode->NoOfImgBuffers));

    if (pGeode->NoOfImgBuffers <= 0)
	pGeode->NoOfImgBuffers = 0;

    xf86GetOptValInteger(GeodeOptions, GX_OPTION_OSM_CLR_BUFS,
	&(pGeode->NoOfColorExpandLines));

    if (pGeode->NoOfColorExpandLines <= 0)
	pGeode->NoOfColorExpandLines = 0;

    xf86GetOptValInteger(GeodeOptions, GX_OPTION_OSM_CLR_BUFS,
	&(pGeode->exaBfrSz));

    if (pGeode->exaBfrSz <= 0)
	pGeode->exaBfrSz = 0;

    /* We are going to see if we can get rid of this option */

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_CUSTOM_MODE, FALSE))
	pGeode->CustomMode = TRUE;

    if (pGeode->Panel == TRUE) {
	if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOPANEL, FALSE))
	    pGeode->Panel = FALSE;
    }

    panelgeo = xf86GetOptValString(GeodeOptions, GX_OPTION_PANEL_GEOMETRY);

    if ((s = xf86GetOptValString(GeodeOptions, GX_OPTION_ACCEL_METHOD))) {
	if (!xf86NameCmp(s, "EXA"))
	    pGeode->useEXA = TRUE;
    }

    xf86DrvMsg(pScrni->scrnIndex, X_INFO,
	"Using %s acceleration architecture\n",
	pGeode->useEXA ? "EXA" : "XAA");

    /* Set up the panel */

    if (pGeode->Panel) {
	if (panelgeo != NULL) {
	    if (GXGetFPInfo(panelgeo, &pGeode->FPBX, &pGeode->FPBY))
		pGeode->Panel = FALSE;
	}
#ifdef PNL_SUP
	else {
	    int b, f;

	    /* The bitdepth and refresh isn't used anywhere else in the driver */

	    if ((pGeode->Panel = Pnl_IsPanelEnabledInBIOS()))
		Pnl_GetPanelInfoFromBIOS(&pGeode->FPBX, &pGeode->FPBY, &b,
		    &f);
	}
#endif
    }

    /* Set up the VGA */

    if (pGeode->useVGA) {
#if INT10_SUPPORT
	VESARec *pVesa;

	if (!xf86LoadSubModule(pScrni, "int10"))
	    return FALSE;
	xf86LoaderReqSymLists(amdInt10Symbols, NULL);

	pVesa = pGeode->vesa;

	if ((pVesa->pInt = xf86InitInt10(pGeode->pEnt->index)) == NULL) {
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Unable to initialize 1NT10 support\n");
	    pGeode->useVGA = FALSE;
	}
#endif

	if (!xf86LoadSubModule(pScrni, "vgahw"))
	    return FALSE;

	xf86LoaderReqSymLists(amdVgahwSymbols, NULL);
	pGeode->FBVGAActive = gu2_get_vga_active();
    }

    pGeode->FBAvail = gfx_get_frame_buffer_size();

    if (pScrni->memPhysBase == 0)
	pScrni->memPhysBase = gfx_get_frame_buffer_base();

    pScrni->fbOffset = 0;

    if (pGeode->pEnt->device->videoRam == 0)
	pScrni->videoRam = pGeode->FBAvail / 1024;
    else
	pScrni->videoRam = pGeode->pEnt->device->videoRam;

    if (!GXMapMem(pScrni))
	return FALSE;

    pGeode->maxWidth = GX_MAX_WIDTH;
    pGeode->maxHeight = GX_MAX_HEIGHT;

    GeodeClockRange = (ClockRangePtr) xnfcalloc(sizeof(ClockRange), 1);
    GeodeClockRange->next = NULL;
    GeodeClockRange->minClock = 25175;
    GeodeClockRange->maxClock = 229500;
    GeodeClockRange->clockIndex = -1;
    GeodeClockRange->interlaceAllowed = TRUE;
    GeodeClockRange->doubleScanAllowed = FALSE;

    if (pGeode->useVGA)
	pScrni->monitor->DDC = GXDoDDC(pScrni, pGeode->pEnt->index);
    else
	pScrni->monitor->DDC = NULL;

    /* I'm still not 100% sure this uses the right values */

    modecnt = xf86ValidateModes(pScrni,
	pScrni->monitor->Modes,
	pScrni->display->modes,
	GeodeClockRange,
	NULL, GX_MIN_PITCH, GX_MAX_PITCH,
	32, GX_MIN_HEIGHT, GX_MAX_HEIGHT,
	pScrni->display->virtualX,
	pScrni->display->virtualY, pGeode->FBAvail, LOOKUP_BEST_REFRESH);

    if (modecnt <= 0) {
	xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes were found\n");
	GXFreeRec(pScrni);
	return FALSE;
    }

    xf86PruneDriverModes(pScrni);

    if (pScrni->modes == NULL) {
	xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes were found\n");
	GXFreeRec(pScrni);
	return FALSE;
    }

    xf86SetCrtcForModes(pScrni, 0);
    pScrni->currentMode = pScrni->modes;

    xf86PrintModes(pScrni);
    xf86SetDpi(pScrni, 0, 0);

    /* Load the modules we'll need */

    if (xf86LoadSubModule(pScrni, "fb") == NULL) {
	GXFreeRec(pScrni);
	return FALSE;
    }

    xf86LoaderReqSymLists(amdFbSymbols, NULL);

    if (pGeode->NoAccel == FALSE) {
	const char *module = "xaa";
	const char **symbols = &amdXaaSymbols[0];

	if (pGeode->useEXA) {
	    module = "exa";
	    symbols = &amdExaSymbols[0];
	}

	if (!xf86LoadSubModule(pScrni, module)) {
	    GXFreeRec(pScrni);
	    return FALSE;
	}

	xf86LoaderReqSymLists(symbols, NULL);
    }

    if (pGeode->tryHWCursor == TRUE) {
	if (!xf86LoadSubModule(pScrni, "ramdac")) {
	    GXFreeRec(pScrni);
	    return FALSE;
	}

	xf86LoaderReqSymLists(amdRamdacSymbols, NULL);
    }

    if (xf86RegisterResources(pGeode->pEnt->index, NULL, ResExclusive)) {
	xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
	    "Couldn't register the resources.\n");
	GXFreeRec(pScrni);
	return FALSE;
    }

    return TRUE;
}

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

static void
GXSetDvLineSize(unsigned int pitch)
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

/* XXX - this is nothing like the original function - not sure exactly what the purpose is for this quite yet */

static void
GXAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long offset;

    offset =
	pGeode->FBOffset + y * pGeode->Pitch +
	x * (pScrni->bitsPerPixel >> 3);

    gfx_set_display_offset(offset);
}

static Bool
GXSetVideoMode(ScrnInfoPtr pScrni, DisplayModePtr pMode)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int flags;

    pScrni->vtSema = TRUE;

    gx_disable_dac_power();

    if (pMode->Flags & V_NHSYNC)
	flags |= 1;
    if (pMode->Flags & V_NVSYNC)
	flags |= 2;

    /* XXX Fixme - for now, we'll use the fixed settings for panel - but I would
     * like to see these go away too 
     */

    /* XXX Question - why even use set_display_mode at all - shouldn't the
     * mode timings be the same that we advertise? */

    if (pGeode->Panel) {
	GFX(set_fixed_timings(pGeode->FPBX, pGeode->FPBY,
		pMode->CrtcHDisplay, pMode->CrtcVDisplay,
		pScrni->bitsPerPixel));
    } else {
	GFX(set_display_timings(pScrni->bitsPerPixel, flags,
		pMode->CrtcHDisplay, pMode->CrtcHBlankStart,
		pMode->CrtcHSyncStart, pMode->CrtcHSyncEnd,
		pMode->CrtcHBlankEnd, pMode->CrtcHTotal,
		pMode->CrtcVDisplay, pMode->CrtcVBlankStart,
		pMode->CrtcVSyncStart, pMode->CrtcVSyncEnd,
		pMode->CrtcVBlankEnd, pMode->CrtcVTotal,
		(int)((pMode->SynthClock / 1000.0) * 0x10000)));
    }

    GFX(set_crt_enable(CRT_ENABLE));
    GFX(set_display_pitch(pGeode->displayPitch));
    GFX(set_display_offset(0L));
    GFX(wait_vertical_blank());

    if (pGeode->Compression) {
	GXSetDvLineSize(pGeode->Pitch);

	gfx_set_compression_offset(pGeode->CBOffset);
	gfx_set_compression_pitch(GX_CB_PITCH);
	gfx_set_compression_size(GX_CB_SIZE);

	gfx_set_compression_enable(1);
    }

    if (pGeode->HWCursor && !(pMode->Flags & V_DBLSCAN)) {
	GXLoadCursorImage(pScrni, NULL);
	GFX(set_cursor_position(pGeode->CursorStartOffset, 0, 0, 0, 0));
	GXShowCursor(pScrni);
    } else {
	GFX(set_cursor_enable(0));
	pGeode->HWCursor = FALSE;
    }

    GXAdjustFrame(pScrni->scrnIndex, pScrni->frameX0, pScrni->frameY0, 0);
    gx_enable_dac_power();

    return TRUE;
}

static Bool
GXSwitchMode(int index, DisplayModePtr pMode, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[index];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int ret = TRUE;
    int rotate;

    /* Syn the engine and shutdown the DAC momentarily */

    gfx_wait_until_idle();

    /* Set up the memory for the new mode */
    rotate = GXGetRotation(pScrni->pScreen);
    ret = GXAllocateMemory(pScrni->pScreen, pScrni, rotate);

    if (ret) {
	if (pGeode->curMode != pMode)
	    ret = GXSetVideoMode(pScrni, pMode);
    }

    if (ret)
	ret = GXRotate(pScrni, pMode);

    /* Go back the way it was */

    if (ret == FALSE) {
	if (!GXSetVideoMode(pScrni, pGeode->curMode))
	    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
		"Could not restore the previous mode\n");
    } else
	pGeode->curMode = pMode;

    return ret;
}

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

    gx_enable_dac_power();
}

static Bool
GXCloseScreen(int scrnIndex, ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pScrni->vtSema)
	GXLeaveGraphics(pScrni);

    if (pGeode->AccelInfoRec)
	XAADestroyInfoRec(pGeode->AccelInfoRec);

    if (pGeode->AccelImageWriteBuffers) {
	xfree(pGeode->AccelImageWriteBuffers[0]);
	xfree(pGeode->AccelImageWriteBuffers);
	pGeode->AccelImageWriteBuffers = NULL;
    }

    if (pGeode->AccelColorExpandBuffers) {
	xfree(pGeode->AccelColorExpandBuffers);
	pGeode->AccelColorExpandBuffers = NULL;
    }

    if (pGeode->pExa) {
	exaDriverFini(pScrn);
	xfree(pGeode->pExa);
	pGeode->pExa = NULL;
    }

    pScrni->vtSema = FALSE;

    GXUnmapMem(pScrni);

    if (pGeode && (pScrn->CloseScreen = pGeode->CloseScreen)) {
	pGeode->CloseScreen = NULL;
	return ((*pScrn->CloseScreen) (scrnIndex, pScrn));
    }

    return TRUE;
}

static Bool
GXEnterGraphics(ScreenPtr pScrn, ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    gfx_wait_until_idle();

    /* Save off the current state (should this be somewhere else)? */

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

    pGeode->FBDisplayOffset = gfx_get_display_offset();

    if (pGeode->useVGA) {
	vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

	pGeode->FBBIOSMode = pvgaHW->readCrtc(pvgaHW, 0x040);
    }

    pGeode->FBCompressionEnable = gfx_get_compression_enable();
    pGeode->FBCompressionOffset = gfx_get_compression_offset();
    pGeode->FBCompressionPitch = gfx_get_compression_pitch();
    pGeode->FBCompressionSize = gfx_get_compression_size();

#ifdef PNL_SUP
    Pnl_SavePanelState();
#endif

    /* Turn off the VGA */

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

    /* Set up the memory */
    /* XXX - FIXME - when we alow inital rotation, it should be here */
    GXAllocateMemory(pScrn, pScrni, RR_Rotate_0);

    /* Clear the framebuffer */
    memset(pGeode->FBBase + pGeode->displayOffset, 0, pGeode->displaySize);

    /* Set up the video mode */
    GXSetVideoMode(pScrni, pScrni->currentMode);
    pGeode->curMode = pScrni->currentMode;

    return TRUE;
}

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

	GFX(set_display_palette_entry(index, color));
    }
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

static void
GXDPMSSet(ScrnInfoPtr pScrni, int mode, int flags)
{
    GeodeRec *pGeode;

    pGeode = GEODEPTR(pScrni);

    if (!pScrni->vtSema)
	return;

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

static Bool
GXCreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrni = xf86Screens[pScreen->myNum];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pScreen->CreateScreenResources = pGeode->CreateScreenResources;
    if (!(*pScreen->CreateScreenResources) (pScreen))
	return FALSE;

    if (xf86LoaderCheckSymbol("GXRandRSetConfig")
	&& pGeode->rotation != RR_Rotate_0) {
	Rotation(*GXRandRSetConfig) (ScreenPtr pScreen, Rotation rr, int rate,
	    RRScreenSizePtr pSize) = NULL;
	RRScreenSize p;
	Rotation requestedRotation = pGeode->rotation;

	pGeode->rotation = RR_Rotate_0;

	/* Just setup enough for an initial rotate */

	p.width = pScreen->width;
	p.height = pScreen->height;
	p.mmWidth = pScreen->mmWidth;
	p.mmHeight = pScreen->mmHeight;

	GXRandRSetConfig = LoaderSymbol("GXRandRSetConfig");
	if (GXRandRSetConfig) {
	    pGeode->starting = TRUE;
	    (*GXRandRSetConfig) (pScreen, requestedRotation, 0, &p);
	    pGeode->starting = FALSE;
	}
    }

    return TRUE;
}

static Bool
GXScreenInit(int scrnIndex, ScreenPtr pScrn, int argc, char **argv)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    XF86ModReqInfo shadowReq;
    int maj, min, ret, rotate;
    BOOL shadowfb = TRUE;

    /* FIXME:  VGA stuff - what does this do? */

    /* 
     * if (pGeode->useVGA) {
     * if (!vgaHWGetHWRec(pScrni))
     * return FALSE;
     * if (!vgaHWMapMem(pScrni))
     * return FALSE;
     * 
     * vgaHWGetIOBase(VGAHWPTR(pScrni));
     * }
     */

    pGeode->starting = TRUE;

    if (!pGeode->NoAccel) {

	if (pGeode->useEXA) {

	    if (!(pGeode->pExa = xnfcalloc(sizeof(ExaDriverRec), 1))) {
		xf86DrvMsg(scrnIndex, X_ERROR,
		    "Couldn't allocate the EXA structure.\n");
		pGeode->NoAccel = TRUE;
	    } else {
		ExaDriverPtr pExa = pGeode->pExa;

		pExa->memoryBase = pGeode->FBBase;

		/* This is set in GXAllocateMemory */
		pExa->memorySize = 0;

		pExa->pixmapOffsetAlign = 32;
		pExa->pixmapPitchAlign = 32;
		pExa->flags = EXA_OFFSCREEN_PIXMAPS;
		pExa->maxX = pGeode->maxWidth - 1;
		pExa->maxY = pGeode->maxHeight - 1;
	    }
	} else {
	    pGeode->AccelImageWriteBuffers =
		xcalloc(sizeof(pGeode->AccelImageWriteBuffers[0]),
		pGeode->NoOfImgBuffers);
	    pGeode->AccelColorExpandBuffers =
		xcalloc(sizeof(pGeode->AccelColorExpandBuffers[0]),
		pGeode->NoOfColorExpandLines);
	}
    }

    /* XXX FIXME - Take down any of the structures on failure? */

    if (!GXEnterGraphics(pScrn, pScrni))
	return FALSE;

    miClearVisualTypes();

    /* XXX Again - take down anything? */

    if (pScrni->bitsPerPixel > 8) {
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

    miSetPixmapDepths();

    /* Point at the visible area to start */

    ret = fbScreenInit(pScrn, pGeode->FBBase + pGeode->displayOffset,
	pScrni->virtualX, pScrni->virtualY,
	pScrni->xDpi, pScrni->yDpi, pGeode->displayWidth,
	pScrni->bitsPerPixel);

    if (!ret)
	return FALSE;

    xf86SetBlackWhitePixels(pScrn);

    /* Set up the color ordering */

    if (pScrni->bitsPerPixel > 8) {
	VisualPtr visual = pScrn->visuals + pScrn->numVisuals;

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

    /* Must follow the color ordering */
    fbPictureInit(pScrn, 0, 0);

    if (!pGeode->NoAccel)
	GXAccelInit(pScrn);

    miInitializeBackingStore(pScrn);
    xf86SetBackingStore(pScrn);

    /* Set up the soft cursor */
    miDCInitialize(pScrn, xf86GetPointerScreenFuncs());

    /* Set up the HW cursor - must follow the soft cursor init */

    if (pGeode->tryHWCursor) {
	if (!GXHWCursorInit(pScrn))
	    xf86DrvMsg(scrnIndex, X_ERROR,
		"Hardware cursor initialization failed.\n");
    }

    /* Set up the color map */

    if (!miCreateDefColormap(pScrn))
	return FALSE;

    if (pScrni->bitsPerPixel == 8) {
	/* Must follow initialization of the default colormap */

	if (!xf86HandleColormaps(pScrn, 256, 8,
		GXLoadPalette, NULL,
		CMAP_PALETTED_TRUECOLOR | CMAP_RELOAD_ON_MODE_SWITCH)) {
	    return FALSE;
	}
    }
#ifdef DPMSExtension
    xf86DPMSInit(pScrn, GXDPMSSet, 0);
#endif

    GXInitVideo(pScrn);

    /* Set up RandR */

    xf86DisableRandR();		       /* We provide our own RandR goodness */

    /* Try to set up the shadow FB for rotation */

    memset(&shadowReq, 0, sizeof(shadowReq));
    shadowReq.majorversion = 1;
    shadowReq.minorversion = 1;

    rotate = RR_Rotate_0;

    if (LoadSubModule(pScrni->module, "shadow",
	    NULL, NULL, NULL, &shadowReq, &maj, &min)) {

	rotate = RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270;
	shadowSetup(pScrn);
    } else {
	LoaderErrorMsg(NULL, "shadow", maj, min);
	xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
	    "Error loading shadow - rotation not available.\n");
    }

    GXRandRInit(pScrn, rotate);

    pGeode->PointerMoved = pScrni->PointerMoved;
    pScrni->PointerMoved = GXPointerMoved;
    pGeode->CreateScreenResources = pScrn->CreateScreenResources;
    pScrn->CreateScreenResources = GXCreateScreenResources;

    /* XXX - Allow user to specify initial rotation */
    pGeode->rotation = RR_Rotate_0;

    pGeode->CloseScreen = pScrn->CloseScreen;
    pScrn->CloseScreen = GXCloseScreen;
    pScrn->SaveScreen = GXSaveScreen;

    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScrni->scrnIndex, pScrni->options);

    pGeode->starting = FALSE;

    return TRUE;
}

static int
GXValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int p, ret;

    if (pMode->type != M_T_USERDEF) {

	if (pGeode->Panel) {
	    if (pMode->CrtcHDisplay > pGeode->FPBX ||
		pMode->CrtcVDisplay > pGeode->FPBY ||
		gfx_is_panel_mode_supported(pGeode->FPBX, pGeode->FPBY,
		    pMode->CrtcHDisplay, pMode->CrtcVDisplay,
		    pScrni->bitsPerPixel) < 0) {

		return MODE_BAD;
	    }
	}

	ret = gfx_is_display_mode_supported(pMode->CrtcHDisplay,
	    pMode->CrtcVDisplay,
	    pScrni->bitsPerPixel, GXGetRefreshRate(pMode));
	if (ret < 0) {
	    return MODE_BAD;
	}
    }

    if (pMode->Flags & V_INTERLACE)
	return MODE_NO_INTERLACE;

    if (pGeode->tryCompression)
	p = GXCalculatePitchBytes(pMode->CrtcHDisplay, pScrni->bitsPerPixel);
    else
	p = ((pMode->CrtcHDisplay + 3) & ~3) * (pScrni->bitsPerPixel >> 3);

    if (p * pMode->CrtcVDisplay > pGeode->FBAvail)
	return MODE_MEM;

    return MODE_OK;
}

/* XXX - Way more to do here */

static Bool
GXEnterVT(int scrnIndex, int flags)
{
    return GXEnterGraphics(NULL, xf86Screens[scrnIndex]);
}

static void
GXLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrni = xf86Screens[scrnIndex];
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->PrevDisplayOffset = gfx_get_display_offset();
    GXLeaveGraphics(xf86Screens[scrnIndex]);
}

static void
GXFreeScreen(int scrnIndex, int flags)
{
    GeodeRec *pGeode = GEODEPTR(xf86Screens[scrnIndex]);

    if (pGeode->useVGA) {
	if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    }

    GXFreeRec(xf86Screens[scrnIndex]);
}

void
GXSetupChipsetFPtr(ScrnInfoPtr pScrn)
{
    pScrn->PreInit = GXPreInit;
    pScrn->ScreenInit = GXScreenInit;
    pScrn->SwitchMode = GXSwitchMode;
    pScrn->AdjustFrame = GXAdjustFrame;
    pScrn->EnterVT = GXEnterVT;
    pScrn->LeaveVT = GXLeaveVT;
    pScrn->FreeScreen = GXFreeScreen;
    pScrn->ValidMode = GXValidMode;
}

/* STUPID STUFF */

static void
GXPointerMoved(int index, int x, int y)
{
    ScrnInfoPtr pScrni = xf86Screens[index];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    Bool frameChanged = FALSE;

    if (x < 0)
	x = 0;
    else if (x >= pScrni->virtualX)
	x = pScrni->virtualX - 1;

    if (y < 0)
	y = 0;
    else if (y >= pScrni->virtualY)
	y = pScrni->virtualY - 1;

    if (pScrni->frameX0 > x) {
	pScrni->frameX0 = x;
	pScrni->frameX1 = x + pGeode->HDisplay - 1;
	frameChanged = TRUE;
    }

    if (pScrni->frameX1 < x) {
	pScrni->frameX1 = x + 1;
	pScrni->frameX0 = x - pGeode->HDisplay + 1;
	frameChanged = TRUE;
    }

    if (pScrni->frameY0 > y) {
	pScrni->frameY0 = y;
	pScrni->frameY1 = y + pGeode->VDisplay - 1;
	frameChanged = TRUE;
    }

    if (pScrni->frameY1 < y) {
	pScrni->frameY1 = y;
	pScrni->frameY0 = y - pGeode->VDisplay + 1;
	frameChanged = TRUE;
    }

    if (frameChanged && pScrni->AdjustFrame != NULL)
	pScrni->AdjustFrame(pScrni->scrnIndex, pScrni->frameX0,
	    pScrni->frameY0, 0);
}
