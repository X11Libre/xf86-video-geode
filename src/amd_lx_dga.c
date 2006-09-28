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
 * File contents: DGA(Direct Acess Graphics mode) is feature of
 *                XFree86 that allows the program to access directly to video
 *                memory on the graphics card.  DGA supports the double
 *                flickering.  This file has the functions to support the DGA
 *                modes.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xaa.h"
#include "xaalocal.h"
#include "amd.h"
#include "dgaproc.h"

/* forward declarations */
Bool LXDGAInit(ScreenPtr pScrn);
static Bool LX_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **,
    int *, int *, int *);
static void LX_CloseFramebuffer(ScrnInfoPtr pScrni);
static Bool LX_SetMode(ScrnInfoPtr, DGAModePtr);
static int LX_GetViewport(ScrnInfoPtr);
static void LX_SetViewport(ScrnInfoPtr, int, int, int);
static void LX_FillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void LX_BlitRect(ScrnInfoPtr, int, int, int, int, int, int);

extern void LXAdjustFrame(int, int, int, int);
extern Bool LXSwitchMode(int, DisplayModePtr, int);
extern void LXAccelSync(ScrnInfoPtr pScrni);

static DGAFunctionRec LXDGAFuncs = {
    LX_OpenFramebuffer,
    LX_CloseFramebuffer,
    LX_SetMode,
    LX_SetViewport,
    LX_GetViewport,
    LXAccelSync,
    LX_FillRect,
    LX_BlitRect,
    NULL
};

/*----------------------------------------------------------------------------
 * LXDGAInit.
 *
 * Description	:This function is used to initialize the DGA modes and sets
 *				 the viewport based on the screen mode.
 * Parameters.
 *	pScreeen	:Pointer to screen info structure.
 *
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments		:This function prepares the DGA mode settings for
 *				 other func reference.
 *
*----------------------------------------------------------------------------
*/
Bool
LXDGAInit(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    DGAModePtr modes = NULL, newmodes = NULL, currentMode;
    DisplayModePtr pMode, firstMode;
    int Bpp = pScrni->bitsPerPixel >> 3;
    int num = 0;
    Bool oneMore;

    pMode = firstMode = pScrni->modes;
    DEBUGMSG(0, (0, X_NONE, "LXDGAInit %d\n", Bpp));
    while (pMode) {
        /* one record is allocated here */
        newmodes = xrealloc(modes, (num + 1) * sizeof(DGAModeRec));
        oneMore = FALSE;

        if (!newmodes) {
            xfree(modes);
            return FALSE;
        }

        modes = newmodes;

      SECOND_PASS:                    /* DGA mode flgas and viewport parametrs are set here. */
        currentMode = modes + num;
        num++;
        currentMode->mode = pMode;
        currentMode->flags = DGA_CONCURRENT_ACCESS | DGA_PIXMAP_AVAILABLE;
        currentMode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT;
        if (pMode->Flags & V_DBLSCAN)
            currentMode->flags |= DGA_DOUBLESCAN;
        if (pMode->Flags & V_INTERLACE)
            currentMode->flags |= DGA_INTERLACED;
        currentMode->byteOrder = pScrni->imageByteOrder;
        currentMode->depth = pScrni->depth;
        currentMode->bitsPerPixel = pScrni->bitsPerPixel;
        currentMode->red_mask = pScrni->mask.red;
        currentMode->green_mask = pScrni->mask.green;
        currentMode->blue_mask = pScrni->mask.blue;
        currentMode->visualClass = (Bpp == 1) ? PseudoColor : TrueColor;
        currentMode->viewportWidth = pMode->HDisplay;
        currentMode->viewportHeight = pMode->VDisplay;
        currentMode->xViewportStep = 1;
        currentMode->yViewportStep = 1;
        currentMode->viewportFlags = DGA_FLIP_RETRACE;
        currentMode->offset = 0;
        currentMode->address = pGeode->FBBase;
        if (oneMore) {                 /* first one is narrow width */
            currentMode->bytesPerScanline =
                ((pMode->HDisplay * Bpp) + 3) & ~3L;
            currentMode->imageWidth = pMode->HDisplay;
            currentMode->imageHeight = pMode->VDisplay;
            currentMode->pixmapWidth = currentMode->imageWidth;
            currentMode->pixmapHeight = currentMode->imageHeight;
            currentMode->maxViewportX = currentMode->imageWidth -
                currentMode->viewportWidth;
            /* this might need to get clamped to some maximum */
            currentMode->maxViewportY = currentMode->imageHeight -
                currentMode->viewportHeight;
            oneMore = FALSE;
            goto SECOND_PASS;
        } else {
            currentMode->bytesPerScanline =
                ((pScrni->displayWidth * Bpp) + 3) & ~3L;
            currentMode->imageWidth = pScrni->displayWidth;
            currentMode->imageHeight = pMode->VDisplay;
            currentMode->pixmapWidth = currentMode->imageWidth;
            currentMode->pixmapHeight = currentMode->imageHeight;
            currentMode->maxViewportX = currentMode->imageWidth -
                currentMode->viewportWidth;
            /* this might need to get clamped to some maximum */
            currentMode->maxViewportY = currentMode->imageHeight -
                currentMode->viewportHeight;
        }
        pMode = pMode->next;
        if (pMode == firstMode)
            break;
    }
    pGeode->numDGAModes = num;
    pGeode->DGAModes = modes;

    return DGAInit(pScrn, &LXDGAFuncs, modes, num);
}

/*----------------------------------------------------------------------------
 * LX_SetMode.
 *
 * Description	:This function is sets into the DGA mode.
 *.
 * Parameters.
 *	pScreeen	:Pointer to screen info structure.
 *	pMode		:Points to the DGAmode ptr data
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments		:none.
 *			
 *
*----------------------------------------------------------------------------
*/
static Bool
LX_SetMode(ScrnInfoPtr pScrni, DGAModePtr pMode)
{
    static int OldDisplayWidth[MAXSCREENS];
    int index = pScrni->pScreen->myNum;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(0, (0, X_NONE, "LX_SetMode\n"));

    if (!pMode) {
        /* restore the original mode
         * put the ScreenParameters back
         */
        pScrni->displayWidth = OldDisplayWidth[index];
        DEBUGMSG(0,
            (0, X_NONE, "LX_SetMode !pMode %d\n", pScrni->displayWidth));
        LXSwitchMode(index, pScrni->currentMode, 0);
        pGeode->DGAactive = FALSE;
    } else {
        if (!pGeode->DGAactive) {      /* save the old parameters */
            OldDisplayWidth[index] = pScrni->displayWidth;
            pGeode->DGAactive = TRUE;
            DEBUGMSG(0, (0, X_NONE, "LX_SetMode pMode+ NA %d\n",
                    pScrni->displayWidth));
        }
        pGeode->PrevDisplayOffset = vg_get_display_offset();

        pScrni->displayWidth = pMode->bytesPerScanline /
            (pMode->bitsPerPixel >> 3);
        DEBUGMSG(0, (0, X_NONE,
                "LX_SetMode pMode+  %d\n", pScrni->displayWidth));
        LXSwitchMode(index, pMode->mode, 0);
    }
    /* enable/disable Compression */
    if (pGeode->Compression) {
        vg_set_compression_enable(!pGeode->DGAactive);
    }

    /* enable/disable cursor */
    if (pGeode->HWCursor) {
        vg_set_cursor_enable(!pGeode->DGAactive);
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
 * LX_GetViewPort.
 *
 * Description	:This function is Gets the viewport window memory.
 *.
 * Parameters.
 *	pScrni		:Pointer to screen info structure.
 *	
 * Returns		:returns the viewport status.
 *
 * Comments		:none.
 *			
 *
*----------------------------------------------------------------------------
*/
static int
LX_GetViewport(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    return pGeode->DGAViewportStatus;
}

/*----------------------------------------------------------------------------
 * LX_SetViewPort.
 *
 * Description	:This function is Gets the viewport window memory.
 *
 * Parameters.
 *	pScrni		:Pointer to screen info structure.
		x		:x-cordinate of viewport window
 *		y		:y-codinate of the viewport window.
 *	flags		:indicates the viewport to be flipped or not.
 * Returns		:returns the viewport status  as zero.
 *
 * Comments		:none.
 *			
*----------------------------------------------------------------------------
*/
static void
LX_SetViewport(ScrnInfoPtr pScrni, int x, int y, int flags)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    LXAdjustFrame(pScrni->pScreen->myNum, x, y, flags);
    pGeode->DGAViewportStatus = 0;     /* LXAdjustFrame loops until finished */
}

/*----------------------------------------------------------------------------
 * LX_FillRect.
 *
 * Description	:This function is Gets the viewport window memory.
 *.
 * Parameters.
 *	pScrni		:Pointer to screen info structure.
 *		x		:x-cordinate of viewport window
 *		y		:y-codinate of the viewport window.
 *		w		:width of the rectangle
 *      h		:height of the rectangle.
 *	color		:color to be filled in rectangle.
 *
 * Returns		:returns the viewport status  as zero.
 *
 * Comments		:This function is implemented by solidfill routines..
 *			
*----------------------------------------------------------------------------
*/
static void
LX_FillRect(ScrnInfoPtr pScrni, int x, int y,
    int w, int h, unsigned long color)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->AccelInfoRec) {
        (*pGeode->AccelInfoRec->SetupForSolidFill) (pScrni, color, GXcopy,
            ~0);
        (*pGeode->AccelInfoRec->SubsequentSolidFillRect) (pScrni, x, y, w, h);
        SET_SYNC_FLAG(pGeode->AccelInfoRec);
    }
}

/*----------------------------------------------------------------------------
 * LX_BlitRect.
 *
 * Description	:This function implementing Blit and it moves a
 *			 	 Rectangular block of data from one location to other
 *			 	 Location.
 *
 * Parameters.
 *	pScrni		:Pointer to screen info structure.
 *	srcx		:x-cordinate of the src rectangle
 *	srcy		:y-codinate of src rectangle.
 *	  w			:width of the rectangle
 *    h			:height of the rectangle.
 *	dstx		:x-cordinate of the dst rectangle.
 *	dsty		:y -coordinates of the dst rectangle.
 * Returns		:none.
 *
 * Comments		:none
 *			
*----------------------------------------------------------------------------
*/
static void
LX_BlitRect(ScrnInfoPtr pScrni,
    int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->AccelInfoRec) {
        int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
        int ydir = (srcy < dsty) ? -1 : 1;

        (*pGeode->AccelInfoRec->SetupForScreenToScreenCopy)
            (pScrni, xdir, ydir, GXcopy, ~0, -1);
        (*pGeode->AccelInfoRec->SubsequentScreenToScreenCopy)
            (pScrni, srcx, srcy, dstx, dsty, w, h);
        SET_SYNC_FLAG(pGeode->AccelInfoRec);
    }
}

/*----------------------------------------------------------------------------
 * LX_OpenFramebuffer.
 *
 * Description	:This function open the framebuffer driver for DGA.
 *			
 * Parameters.
 *	pScrni		:Pointer to screen info structure.
 *	srcx		:x-cordinate of the src rectangle
 *	srcy		:y-codinate of src rectangle.
 *		w		:width of the rectangle
 *    	h		:height of the rectangle.
 *	dstx		:x-cordinate of the dst rectangle.
 *	dsty		:y -coordinates of the dst rectangle.
 * Returns		:none.
 *
 * Comments		:none
 *			
*----------------------------------------------------------------------------
*/
static Bool
LX_OpenFramebuffer(ScrnInfoPtr pScrni,
    char **name, unsigned char **mem, int *size, int *offset, int *flags)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    *name = NULL;                      /* no special device */
    *mem = (unsigned char *)pGeode->FBLinearAddr;
    *size = pGeode->FBAvail;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}

static void
LX_CloseFramebuffer(ScrnInfoPtr pScrni)
{
}
