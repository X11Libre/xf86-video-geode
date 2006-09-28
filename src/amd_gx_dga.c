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
 * File contents:   DGA(Direct Acess Graphics mode) is feature of XFree86 that
 *                  allows the program to access directly to video memory on
 *                  the graphics card.DGA supports the double flickering. This
 *                  file has the functions to support the DGA modes.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 * */

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
Bool GXDGAInit(ScreenPtr pScrn);
static Bool GX_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **,
    int *, int *, int *);
static void GX_CloseFramebuffer(ScrnInfoPtr pScrn);
static Bool GX_SetMode(ScrnInfoPtr, DGAModePtr);
static int GX_GetViewport(ScrnInfoPtr);
static void GX_SetViewport(ScrnInfoPtr, int, int, int);
static void GX_FillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void GX_BlitRect(ScrnInfoPtr, int, int, int, int, int, int);

extern void GXAdjustFrame(int, int, int, int);
extern Bool GXSwitchMode(int, DisplayModePtr, int);
extern void GXAccelSync(ScrnInfoPtr pScrni);

static DGAFunctionRec GXDGAFuncs = {
    GX_OpenFramebuffer,
    GX_CloseFramebuffer,
    GX_SetMode,
    GX_SetViewport,
    GX_GetViewport,
    GXAccelSync,
    GX_FillRect,
    GX_BlitRect,
    NULL
};

/*----------------------------------------------------------------------------
 * GXDGAInit.
 *
 * Description	:This function is used to intiallize the DGA modes and sets 
 * 				the viewport based on the screen mode.
 * 				
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
GXDGAInit(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    DGAModePtr modes = NULL, newmodes = NULL, currentMode;
    DisplayModePtr pMode, firstMode;
    int Bpp = pScrni->bitsPerPixel >> 3;
    int num = 0;
    Bool oneMore;

    pMode = firstMode = pScrni->modes;
    DEBUGMSG(0, (0, X_NONE, "GXDGAInit %d\n", Bpp));
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
    return DGAInit(pScrn, &GXDGAFuncs, modes, num);
}

/*----------------------------------------------------------------------------
 * GX_SetMode.
 *
 * Description	:This function is sets into the DGA mode.
 *.
 * Parameters.
 *	pScreeen	:Pointer to screen info structure.
 *	pMode		:Points to the DGAmode ptr data
 *
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments		:none.
 *----------------------------------------------------------------------------
 */
static Bool
GX_SetMode(ScrnInfoPtr pScrni, DGAModePtr pMode)
{
    static int OldDisplayWidth[MAXSCREENS];
    int index = pScrni->pScreen->myNum;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DEBUGMSG(0, (0, X_NONE, "GX_SetMode\n"));

    if (!pMode) {
        /* restore the original mode put the ScreenParameters back
         */
        pScrni->displayWidth = OldDisplayWidth[index];
        DEBUGMSG(0,
            (0, X_NONE, "GX_SetMode !pMode %d\n", pScrni->displayWidth));
        GXSwitchMode(index, pScrni->currentMode, 0);
        pGeode->DGAactive = FALSE;
    } else {
        if (!pGeode->DGAactive) {      /* save the old parameters */
            OldDisplayWidth[index] = pScrni->displayWidth;
            pGeode->DGAactive = TRUE;
            DEBUGMSG(0,
                (0, X_NONE, "GX_SetMode pMode+ NA %d\n",
                    pScrni->displayWidth));
        }
#if defined(STB_X)
        Gal_get_display_offset(&pGeode->PrevDisplayOffset);
#else
        pGeode->PrevDisplayOffset = gfx_get_display_offset();
#endif
        pScrni->displayWidth = pMode->bytesPerScanline /
            (pMode->bitsPerPixel >> 3);
        DEBUGMSG(0,
            (0, X_NONE, "GX_SetMode pMode+  %d\n", pScrni->displayWidth));
        GXSwitchMode(index, pMode->mode, 0);
    }
    /* enable/disable Compression */
    if (pGeode->Compression) {
        GFX(set_compression_enable(!pGeode->DGAactive));
    }

    /* enable/disable cursor */
    if (pGeode->HWCursor) {
        GFX(set_cursor_enable(!pGeode->DGAactive));
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
 * GX_GetViewPort.
 *
 * Description	:This function is Gets the viewport window memory.
 *.
 * Parameters.
 *	pScrni		:Pointer to screen info structure.
 *	
 * Returns		:returns the viewport status.
 *
 * Comments		:none.
 *----------------------------------------------------------------------------
 */
static int
GX_GetViewport(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    return pGeode->DGAViewportStatus;
}

/*----------------------------------------------------------------------------
 * GX_SetViewPort.
 *
 * Description	:This function is Gets the viewport window memory.
 *
 * Parameters.
 *	pScrn		:Pointer to screen info structure.
		x		:x-cordinate of viewport window
 *		y		:y-codinate of the viewport window.
 *	flags		:indicates the viewport to be flipped or not.
 *
 * Returns		:returns the viewport status  as zero.
 *
 * Comments		:none.
 *----------------------------------------------------------------------------
 */
static void
GX_SetViewport(ScrnInfoPtr pScrni, int x, int y, int flags)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    GXAdjustFrame(pScrni->pScreen->myNum, x, y, flags);
    pGeode->DGAViewportStatus = 0;     /*GXAdjustFrame loops until finished */
}

/*----------------------------------------------------------------------------
 * GX_FillRect.
 *
 * Description	:This function is Gets the viewport window memory.
 *.
 * Parameters.
 *	pScrn		:Pointer to screen info structure.
 *		x		:x-cordinate of viewport window
 *		y		:y-codinate of the viewport window.
 *		w		:width of the rectangle
 *      h		:height of the rectangle.
 *	color		:color to be filled in rectangle.
 *
 * Returns		:returns the viewport status  as zero.
 *
 * Comments		:This function is implemented by solidfill routines..
 *----------------------------------------------------------------------------
 */
static void
GX_FillRect(ScrnInfoPtr pScrn, int x, int y,
    int w, int h, unsigned long color)
{
    GeodeRec *pGeode = GEODEPTR(pScrn);

    if (pGeode->AccelInfoRec) {
        (*pGeode->AccelInfoRec->SetupForSolidFill) (pScrn, color, GXcopy, ~0);
        (*pGeode->AccelInfoRec->SubsequentSolidFillRect) (pScrn, x, y, w, h);
        SET_SYNC_FLAG(pGeode->AccelInfoRec);
    }
}

/*----------------------------------------------------------------------------
 * GX_BlitRect.
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
 *----------------------------------------------------------------------------
 */
static void
GX_BlitRect(ScrnInfoPtr pScrni,
    int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->AccelInfoRec) {
        int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
        int ydir = (srcy < dsty) ? -1 : 1;

        (*pGeode->AccelInfoRec->SetupForScreenToScreenCopy)
            (pScrni, xdir, ydir, GXcopy, ~0, -1);
        (*pGeode->AccelInfoRec->SubsequentScreenToScreenCopy) (pScrni, srcx,
            srcy, dstx, dsty, w, h);
        SET_SYNC_FLAG(pGeode->AccelInfoRec);
    }
}

/*----------------------------------------------------------------------------
 * GX_OpenFramebuffer.
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
 *----------------------------------------------------------------------------
 */
static Bool
GX_OpenFramebuffer(ScrnInfoPtr pScrni,
    char **name, unsigned char **mem, int *size, int *offset, int *flags)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    *name = NULL;                      /* no special device */
    *mem = (unsigned char *)pGeode->FBLinearAddr;
    *size = pGeode->FBSize;
    *offset = 0;
    *flags = DGA_NEED_ROOT;
    return TRUE;
}

static void
GX_CloseFramebuffer(ScrnInfoPtr pScrni)
{
}

/* end of file */
