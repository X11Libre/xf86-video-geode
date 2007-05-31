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
 * File Contents:   Direct graphics display routines are implemented and
 *                  graphics rendering are all done in memory.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "amd.h"
#include "shadowfb.h"
#include "servermd.h"

#define CLIP(sip,bp,u1,v1,u2,v2) 	\
   	u1 = bp->x1;  v1 = bp->y1; 		\
   	u2 = bp->x2;  v2 = bp->y2; 		\
   	if ( u1 < 0 )					\
		u1 = 0; 					\
   	if ( v1 < 0 ) 					\
		v1 = 0; 					\
   	if ( u1 > sip->virtualX ) 		\
		u1 = sip->virtualX; 		\
   	if ( v1 > sip->virtualY )		\
		v1 = sip->virtualY; 		\
   	if ( u2 < 0 ) 					\
		u2 = 0; 					\
   	if ( v2 < 0 )					\
   		v2 = 0; 					\
   	if ( u2 > sip->virtualX ) 		\
		u2 = sip->virtualX; 		\
   	if ( v2 > sip->virtualY ) 		\
		v2 = sip->virtualY;

void GXAccelSync(ScrnInfoPtr pScrni);

void
GXRotation0(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x;
    *newY = y;
}

void
GXRotation1(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = (h - 1) - y;
    *newY = x;
}

void
GXRotation2(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = (w - 1) - x;
    *newY = (h - 1) - y;
}

void
GXRotation3(int x, int y, int w, int h, int *newX, int *newY)
{
    *newY = (w - 1) - x;
    *newX = y;
}

void
GXRBltXlat0(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x;
    *newY = y;
}

void
GXRBltXlat1(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x - (h - 1);
    *newY = y;
}

void
GXRBltXlat2(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x - (w - 1);
    *newY = y - (h - 1);
}

void
GXRBltXlat3(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x;
    *newY = y - (w - 1);
}

/*----------------------------------------------------------------------------
 * GXPointerMoved.
 *
 * Description	:This function moves one screen memory from one area to other.
 *
 * Parameters.
 *    index		:Pointer to screen index.
 *     x		:Specifies the new x co-ordinates of new area.
 *     y		:Specifies the new y co-ordinates of new area.
 * Returns		:none
 *
 * Comments		:none
 *
 *----------------------------------------------------------------------------
 */
void
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

void
GXRefreshArea_Cpy(ScrnInfoPtr pScrni, int num, BoxPtr pbox)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int x1, y1, x2, y2, width, height;
    unsigned long src, dst;
    int Bpp = pScrni->bitsPerPixel >> 3;

    gfx_set_raster_operation(0xcc);    /* copy dst=src */
    for (; --num >= 0; ++pbox) {
        CLIP(pScrni, pbox, x1, y1, x2, y2);
        if ((width = x2 - x1) <= 0 || (height = y2 - y1) <= 0)
            continue;

        src = y1 * pGeode->ShadowPitch + x1 * Bpp;
        dst = pGeode->FBOffset + y1 * pGeode->Pitch + x1 * Bpp;
        gfx2_set_destination_stride(pGeode->Pitch);
        gfx2_color_bitmap_to_screen_blt(0, 0, dst, width, height,
            pGeode->ShadowPtr + src, pGeode->ShadowPitch);
    }
}

/*----------------------------------------------------------------------------
 * GXRefreshArea8.
 *
 * Description	:This function  copies the memory to be displayed from the
 *               shadow pointer by 8bpp.
 * Parameters.
 *    pScrni	:Pointer to ScrnInfo structure.
 *    num		:Specifies the num of squarebox area to be displayed.
 *    pbox		:Points to square of memory to be displayed.
 *
 * Returns		:none
 *
 * Comments		:none
 *
 *----------------------------------------------------------------------------
 */
void
GXRefreshArea0_Cpu(ScrnInfoPtr pScrni, int num, BoxPtr pbox)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int width, height, x1, y1, x2, y2;
    unsigned char *src, *dst;
    int Bpp = pScrni->bitsPerPixel >> 3;

    GXAccelSync(pScrni);
    for (; --num >= 0; ++pbox) {
        CLIP(pScrni, pbox, x1, y1, x2, y2);
        if ((width = x2 - x1) <= 0 || (height = y2 - y1) <= 0)
            continue;

        src = pGeode->ShadowPtr + y1 * pGeode->ShadowPitch + x1 * Bpp;
        dst =
            pGeode->FBBase + pGeode->FBOffset + y1 * pGeode->Pitch + x1 * Bpp;
        width *= Bpp;
        while (--height >= 0) {
            memcpy(dst, src, width);
            dst += pGeode->Pitch;
            src += pGeode->ShadowPitch;
        }
    }
}

#define RefreshArea1_Cpu(nm,typ)										\
void GXRefreshArea1_Cpu##nm(ScrnInfoPtr pScrni, int num, BoxPtr pbox)	\
{                                                                     	\
	GeodeRec *pGeode = GEODEPTR(pScrni);                                	\
   	int l, width, height, x1, y1, x2, y2, newX, newY;                  	\
   	unsigned long src, dst, dp;                                        	\
   	typ *sp;                                                           	\
   	GXAccelSync(pScrni);                                              	\
   	for( ; --num>=0; ++pbox ) {                                        	\
      	CLIP(pScrni,pbox,x1,y1,x2,y2);                                  \
      	if( (width=x2-x1) <=0 || (height=y2-y1) <= 0 )					\
		   	continue;        											\
      	src = y1*pGeode->ShadowPitch + x1*sizeof(typ);                  \
      	newX = pScrni->virtualY-1 - y1;                                 \
      	newY = x1;                                                      \
      	dst = pGeode->FBOffset + newY*pGeode->Pitch + newX*sizeof(typ); \
      	while( --height >= 0 ) {                                        \
         	sp = (typ *)(pGeode->ShadowPtr + src);                      \
         	dp = (unsigned long)(pGeode->FBBase + dst);                 \
         	for( l=width; --l>=0; ) {                                   \
            	*(typ *)dp = *sp++;                                     \
	    		dp += pGeode->Pitch;                                    \
         	}                                                           \
	 		dst -= sizeof(typ);                                         \
	 		src += pGeode->ShadowPitch;                                 \
      }                                                               	\
   }                                                                  	\
}

RefreshArea1_Cpu(8, unsigned char)
RefreshArea1_Cpu(16, unsigned short)
RefreshArea1_Cpu(32, unsigned int)
#define RefreshArea2_Cpu(nm,typ) 										\
void GXRefreshArea2_Cpu##nm(ScrnInfoPtr pScrni, int num, BoxPtr pbox)	\
{                                                                     	\
   	GeodeRec *pGeode = GEODEPTR(pScrni);                                	\
   	int l, width, height, x1, y1, x2, y2, newX, newY;                  	\
   	unsigned long src, dst, dp;                                        	\
   	typ *sp;                                                           	\
   	GXAccelSync(pScrni);                                              	\
   	for( ; --num>=0; ++pbox ) {                                        	\
      	CLIP(pScrni,pbox,x1,y1,x2,y2);                                  \
      	if( (width=x2-x1) <=0 || (height=y2-y1) <= 0 ) 					\
		   	continue;        											\
      	src = y1*pGeode->ShadowPitch + x1*sizeof(typ);                  \
      	newX = pScrni->virtualX-1 - x1;                                 \
      	newY = pScrni->virtualY-1 - y1;                                 \
      	dst = pGeode->FBOffset + newY*pGeode->Pitch + newX*sizeof(typ);	\
      	while( --height >= 0 ) {                                        \
         	sp = (typ *)(pGeode->ShadowPtr + src);                      \
         	dp = (unsigned long)(pGeode->FBBase + dst);                 \
         	for( l=width; --l>=0; ) {                                   \
            	*(typ *)dp = *sp++;                                     \
            	dp -= sizeof(typ);                                      \
         	}                                                           \
	 		src += pGeode->ShadowPitch;                                 \
	 		dst -= pGeode->Pitch;                                      	\
      	}                                                               \
   	}                                                                  	\
}
RefreshArea2_Cpu(8, unsigned char)
RefreshArea2_Cpu(16, unsigned short)
RefreshArea2_Cpu(32, unsigned int)
#define RefreshArea3_Cpu(nm,typ) 										\
void GXRefreshArea3_Cpu##nm(ScrnInfoPtr pScrni, int num, BoxPtr pbox)	\
{                                                                     	\
   	GeodeRec *pGeode = GEODEPTR(pScrni);                                	\
   	int l, width, height, x1, y1, x2, y2, newX, newY;                  	\
   	unsigned long src, dst, dp;                                        	\
   	typ *sp;                                                           	\
   	GXAccelSync(pScrni);                                              	\
   	for( ; --num>=0; ++pbox ) {                                        	\
      	CLIP(pScrni,pbox,x1,y1,x2,y2);                                  \
      	if( (width=x2-x1) <=0 || (height=y2-y1) <= 0 )					\
		   	continue;        											\
      	src = y1*pGeode->ShadowPitch + x1*sizeof(typ);                  \
      	newX = y1;                                                      \
      	newY = pScrni->virtualX-1 - x1;                                 \
      	dst = pGeode->FBOffset + newY*pGeode->Pitch + newX*sizeof(typ);	\
      	while( --height >= 0 ) {                                        \
         	sp = (typ *)(pGeode->ShadowPtr + src);                      \
         	dp = (unsigned long)(pGeode->FBBase + dst);                 \
         	for( l=width; --l>=0; ) {                                   \
            	*(typ *)dp = *sp++;                                     \
	    	dp -= pGeode->Pitch;                                      	\
         	}                                                           \
	 		dst += sizeof(typ);                                         \
	 		src += pGeode->ShadowPitch;                                 \
      	}                                                               \
   	}                                                                  	\
}
RefreshArea3_Cpu(8, unsigned char)
RefreshArea3_Cpu(16, unsigned short)
RefreshArea3_Cpu(32, unsigned int)

    void GXRotationInit(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    switch (pGeode->Rotate) {
    case 1:
        pGeode->Rotation = GXRotation1;
        pGeode->RBltXlat = GXRBltXlat1;
        break;
    case 2:
        pGeode->Rotation = GXRotation2;
        pGeode->RBltXlat = GXRBltXlat2;
        break;
    case 3:
        pGeode->Rotation = GXRotation3;
        pGeode->RBltXlat = GXRBltXlat3;
        break;
    default:
        pGeode->Rotation = GXRotation0;
        pGeode->RBltXlat = GXRBltXlat0;
        break;
    }
}

void
GXShadowFBInit(ScreenPtr pScrn, GeodeRec *pGeode, int bytpp)
{
    RefreshAreaFuncPtr refreshArea;

    switch (bytpp) {
    case 2:
        switch (pGeode->Rotate) {
        case 1:
            refreshArea = GXRefreshArea1_Cpu16;
            break;
        case 2:
            refreshArea = GXRefreshArea2_Cpu16;
            break;
        case 3:
            refreshArea = GXRefreshArea3_Cpu16;
            break;
        default:
            refreshArea = GXRefreshArea0_Cpu;
            break;
        }
        break;
    case 4:
        switch (pGeode->Rotate) {
        case 1:
            refreshArea = GXRefreshArea1_Cpu32;
            break;
        case 2:
            refreshArea = GXRefreshArea2_Cpu32;
            break;
        case 3:
            refreshArea = GXRefreshArea3_Cpu32;
            break;
        default:
            refreshArea = GXRefreshArea0_Cpu;
            break;
        }
        break;
    default:
        switch (pGeode->Rotate) {
        case 1:
            refreshArea = GXRefreshArea1_Cpu8;
            break;
        case 2:
            refreshArea = GXRefreshArea2_Cpu8;
            break;
        case 3:
            refreshArea = GXRefreshArea3_Cpu8;
            break;
        default:
            refreshArea = GXRefreshArea0_Cpu;
            break;
        }
        break;
    }
    ShadowFBInit(pScrn, refreshArea);
}

/* End of file */
