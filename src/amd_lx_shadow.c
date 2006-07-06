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
 * File Contents: Direct graphics display routines are implemented and 
 *                graphics rendering are all done in memory.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "amd.h"
#include "shadowfb.h"
#include "servermd.h"

#define CLIP(sip,bp,u1,v1,u2,v2) \
   u1 = bp->x1;  v1 = bp->y1; \
   u2 = bp->x2;  v2 = bp->y2; \
   if( u1 < 0 ) u1 = 0; \
   if( v1 < 0 ) v1 = 0; \
   if( u1 > sip->virtualX ) u1 = sip->virtualX; \
   if( v1 > sip->virtualY ) v1 = sip->virtualY; \
   if( u2 < 0 ) u2 = 0; \
   if( v2 < 0 ) v2 = 0; \
   if( u2 > sip->virtualX ) u2 = sip->virtualX; \
   if( v2 > sip->virtualY ) v2 = sip->virtualY;

void LXAccelSync(ScrnInfoPtr pScrni);

void
LXRotation0(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x;
    *newY = y;
}

void
LXRotation1(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = (h - 1) - y;
    *newY = x;
}

void
LXRotation2(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = (w - 1) - x;
    *newY = (h - 1) - y;
}

void
LXRotation3(int x, int y, int w, int h, int *newX, int *newY)
{
    *newY = (w - 1) - x;
    *newX = y;
}

void
LXRBltXlat0(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x;
    *newY = y;
}

void
LXRBltXlat1(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x - (h - 1);
    *newY = y;
}

void
LXRBltXlat2(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x - (w - 1);
    *newY = y - (h - 1);
}

void
LXRBltXlat3(int x, int y, int w, int h, int *newX, int *newY)
{
    *newX = x;
    *newY = y - (w - 1);
}

/*----------------------------------------------------------------------------
 * LXPointerMoved.
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
LXPointerMoved(int index, int x, int y)
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
LXRefreshArea_Cpy(ScrnInfoPtr pScrni, int num, BoxPtr pbox)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int x1, y1, x2, y2, width, height;
    unsigned long src, dst;
    int Bpp = pScrni->bitsPerPixel >> 3;

    gp_declare_blt(0);
    gp_set_raster_operation(0xcc);     /* copy dst=src */
    gp_write_parameters();
    for (; --num >= 0; ++pbox) {
        CLIP(pScrni, pbox, x1, y1, x2, y2);
        if ((width = x2 - x1) <= 0 || (height = y2 - y1) <= 0)
            continue;
        src = y1 * pGeode->ShadowPitch + x1 * Bpp;
        dst = pGeode->FBOffset + y1 * pGeode->Pitch + x1 * Bpp;
        gp_declare_blt(0);
        gp_set_strides(pGeode->Pitch, pGeode->ShadowPitch);
        gp_screen_to_screen_blt(dst, src, width, height, 0);
    }
}

/*----------------------------------------------------------------------------
 * LXRefreshArea8.
 *
 * Description	:This function  copies the memory to be displayed from the
 *                 shadow pointer by 8bpp.
 * Parameters.
 *    pScrni		:Pointer to ScrnInfo structure.
 *    num		:Specifies the num of squarebox area to be displayed.
 *    pbox		:Points to square of memory to be displayed.
 * Returns		:none
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/

static int lx_shdw_fmt[4] = {
    CIMGP_SOURCE_FMT_3_3_2,
    CIMGP_SOURCE_FMT_0_5_6_5,
    CIMGP_SOURCE_FMT_24BPP,
    CIMGP_SOURCE_FMT_8_8_8_8
};

void
LXRefreshArea_Blt(ScrnInfoPtr pScrni, int num, BoxPtr pbox)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int width, height, x1, y1, x2, y2, newX, newY;
    unsigned long src, dst;
    int Bpp = pScrni->bitsPerPixel >> 3;

    gp_set_source_format(lx_shdw_fmt[Bpp - 1]);
    gp_declare_blt(0);
    gp_set_raster_operation(0xcc);     /* copy dst=src */
    gp_write_parameters();
    for (; --num >= 0; ++pbox) {
        CLIP(pScrni, pbox, x1, y1, x2, y2);
        if ((width = x2 - x1) <= 0 || (height = y2 - y1) <= 0)
            continue;
        (*pGeode->Rotation) (x1, y1, pScrni->virtualX, pScrni->virtualY,
            &newX, &newY);
        (*pGeode->RBltXlat) (newX, newY, width, height, &newX, &newY);
        src = y1 * pGeode->ShadowPitch + x1 * Bpp;
        dst = pGeode->FBOffset + newY * pGeode->Pitch + newX * Bpp;
        gp_declare_blt(0);
        gp_set_strides(pGeode->Pitch, pGeode->ShadowPitch);
        gp_rotate_blt(dst, src, width, height, pGeode->Rotate * 90);
    }
}

void
LXRefreshArea0_Cpu(ScrnInfoPtr pScrni, int num, BoxPtr pbox)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int width, height, x1, y1, x2, y2;
    unsigned char *src, *dst;
    int Bpp = pScrni->bitsPerPixel >> 3;

    LXAccelSync(pScrni);
    for (; --num >= 0; ++pbox) {
        CLIP(pScrni, pbox, x1, y1, x2, y2);
        if ((width = x2 - x1) <= 0 || (height = y2 - y1) <= 0)
            continue;
        src = pGeode->ShadowPtr + y1 * pGeode->ShadowPitch + x1 * Bpp;
        dst = pGeode->FBBase + pGeode->FBOffset + y1 * pGeode->Pitch + x1 *
            Bpp;
        width *= Bpp;
        while (--height >= 0) {
            memcpy(dst, src, width);
            dst += pGeode->Pitch;
            src += pGeode->ShadowPitch;
        }
    }
}

#define RefreshArea1_Cpu(nm,typ) \
void LXRefreshArea1_Cpu##nm(ScrnInfoPtr pScrni, int num, BoxPtr pbox) \
{                                                                     \
   GeodeRec *pGeode = GEODEPTR(pScrni);                                \
   int l, width, height, x1, y1, x2, y2, newX, newY;                  \
   unsigned long src, dst, dp;                                        \
   typ *sp;                                                           \
   LXAccelSync(pScrni);                                               \
   for (; --num>=0; ++pbox) {                                         \
      CLIP(pScrni,pbox,x1,y1,x2,y2);                                  \
      if ((width=x2-x1) <=0 || (height=y2-y1) <= 0) continue;         \
      src = y1*pGeode->ShadowPitch + x1*sizeof(typ);                  \
      newX = pScrni->virtualY-1 - y1;                                 \
      newY = x1;                                                      \
      dst = pGeode->FBOffset + newY*pGeode->Pitch + newX*sizeof(typ); \
      while (--height >= 0) {                                         \
         sp = (typ *)(pGeode->ShadowPtr + src);                       \
         dp = (unsigned long)(pGeode->FBBase + dst);                  \
         for (l=width; --l>=0;) {                                     \
            *(typ *)dp = *sp++;                                       \
	        dp += pGeode->Pitch;                                      \
         }                                                            \
		 dst -= sizeof(typ);                                          \
		 src += pGeode->ShadowPitch;                                  \
      }                                                               \
   }                                                                  \
}

RefreshArea1_Cpu(8, unsigned char)
RefreshArea1_Cpu(16, unsigned short)
RefreshArea1_Cpu(32, unsigned int)
#define RefreshArea2_Cpu(nm,typ) \
void LXRefreshArea2_Cpu##nm(ScrnInfoPtr pScrni, int num, BoxPtr pbox) \
{                                                                     \
   GeodeRec *pGeode = GEODEPTR(pScrni);                                \
   int l, width, height, x1, y1, x2, y2, newX, newY;                  \
   unsigned long src, dst, dp;                                        \
   typ *sp;                                                           \
   LXAccelSync(pScrni);                                               \
   for (; --num>=0; ++pbox) {                                         \
      CLIP(pScrni,pbox,x1,y1,x2,y2);                                  \
      if ((width=x2-x1) <=0 || (height=y2-y1) <= 0) continue;         \
      src = y1*pGeode->ShadowPitch + x1*sizeof(typ);                  \
      newX = pScrni->virtualX-1 - x1;                                 \
      newY = pScrni->virtualY-1 - y1;                                 \
      dst = pGeode->FBOffset + newY*pGeode->Pitch + newX*sizeof(typ); \
      while (--height >= 0) {                                         \
         sp = (typ *)(pGeode->ShadowPtr + src);                       \
         dp = (unsigned long)(pGeode->FBBase + dst);                  \
         for( l=width; --l>=0; ) {                                    \
            *(typ *)dp = *sp++;                                       \
            dp -= sizeof(typ);                                        \
         }                                                            \
		 src += pGeode->ShadowPitch;                                  \
		 dst -= pGeode->Pitch;                                        \
      }                                                               \
   }                                                                  \
}
RefreshArea2_Cpu(8, unsigned char)
RefreshArea2_Cpu(16, unsigned short)
RefreshArea2_Cpu(32, unsigned int)
#define RefreshArea3_Cpu(nm,typ) \
void LXRefreshArea3_Cpu##nm(ScrnInfoPtr pScrni, int num, BoxPtr pbox) \
{                                                                     \
   GeodeRec *pGeode = GEODEPTR(pScrni);                                \
   int l, width, height, x1, y1, x2, y2, newX, newY;                  \
   unsigned long src, dst, dp;                                        \
   typ *sp;                                                           \
   LXAccelSync(pScrni);                                               \
   for (; --num>=0; ++pbox) {                                         \
      CLIP(pScrni,pbox,x1,y1,x2,y2);                                  \
      if ((width=x2-x1) <=0 || (height=y2-y1) <= 0) continue;         \
      src = y1*pGeode->ShadowPitch + x1*sizeof(typ);                  \
      newX = y1;                                                      \
      newY = pScrni->virtualX-1 - x1;                                 \
      dst = pGeode->FBOffset + newY*pGeode->Pitch + newX*sizeof(typ); \
      while (--height >= 0) {                                         \
         sp = (typ *)(pGeode->ShadowPtr + src);                       \
         dp = (unsigned long)(pGeode->FBBase + dst);                  \
         for( l=width; --l>=0; ) {                                    \
            *(typ *)dp = *sp++;                                       \
			dp -= pGeode->Pitch;                                      \
         }                                                            \
		 dst += sizeof(typ);                                          \
		 src += pGeode->ShadowPitch;                                  \
      }                                                               \
   }                                                                  \
}
RefreshArea3_Cpu(8, unsigned char)
RefreshArea3_Cpu(16, unsigned short)
RefreshArea3_Cpu(32, unsigned int)

    void LXRotationInit(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    switch (pGeode->Rotate) {
    case 1:
        pGeode->Rotation = LXRotation1;
        pGeode->RBltXlat = LXRBltXlat1;
        break;
    case 2:
        pGeode->Rotation = LXRotation2;
        pGeode->RBltXlat = LXRBltXlat2;
        break;
    case 3:
        pGeode->Rotation = LXRotation3;
        pGeode->RBltXlat = LXRBltXlat3;
        break;
    default:
        pGeode->Rotation = LXRotation0;
        pGeode->RBltXlat = LXRBltXlat0;
        break;
    }
}

void
LXShadowFBInit(ScreenPtr pScrn, GeodeRec *pGeode, int bytpp)
{
    RefreshAreaFuncPtr refreshArea;

    if (pGeode->NoAccel || !pGeode->ShadowInFBMem || 0) {
        switch (bytpp) {
        case 2:
            switch (pGeode->Rotate) {
            case 1:
                refreshArea = LXRefreshArea1_Cpu16;
                break;
            case 2:
                refreshArea = LXRefreshArea2_Cpu16;
                break;
            case 3:
                refreshArea = LXRefreshArea3_Cpu16;
                break;
            default:
                refreshArea = LXRefreshArea0_Cpu;
                break;
            }
            break;
        case 4:
            switch (pGeode->Rotate) {
            case 1:
                refreshArea = LXRefreshArea1_Cpu32;
                break;
            case 2:
                refreshArea = LXRefreshArea2_Cpu32;
                break;
            case 3:
                refreshArea = LXRefreshArea3_Cpu32;
                break;
            default:
                refreshArea = LXRefreshArea0_Cpu;
                break;
            }
            break;
        default:
            switch (pGeode->Rotate) {
            case 1:
                refreshArea = LXRefreshArea1_Cpu8;
                break;
            case 2:
                refreshArea = LXRefreshArea2_Cpu8;
                break;
            case 3:
                refreshArea = LXRefreshArea3_Cpu8;
                break;
            default:
                refreshArea = LXRefreshArea0_Cpu;
                break;
            }
            break;
        }
    } else {
        refreshArea = pGeode->Rotate ? LXRefreshArea_Blt : LXRefreshArea_Cpy;
    }
    ShadowFBInit(pScrn, refreshArea);
}
