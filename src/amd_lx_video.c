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
 * File Contents: This file consists of main Xfree video supported routines.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 *
 */

/* 
 * Fixes & Extensions to support Y800 greyscale modes 
 * Alan Hourihane <alanh@fairlite.demon.co.uk>
 */
#ifndef AMD_V4L2_VIDEO
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "regionstr.h"

#include "amd.h"
#include "Xv.h"
#include "xaa.h"
#include "xaalocal.h"
#include "dixstruct.h"
#include "fourcc.h"
#include "amd_fourcc.h"

#if DEBUGLVL>0
#define DBLOG(n,s...) do { if((DEBUGLVL)>=(n)) fprintf(zdfp,s); } while(0)
#include "xf86_ansic.h"
extern FILE *zdfp;
#else
#define DBLOG(n,s...) do {} while(0)
#endif

#define OFF_DELAY 	200            /* milliseconds */
#define FREE_DELAY 	60000

#define OFF_TIMER 	0x01
#define FREE_TIMER	0x02
#define CLIENT_VIDEO_ON	0x04

#define TIMER_MASK      (OFF_TIMER | FREE_TIMER)
#define XV_PROFILE 0
#define REINIT  1

#ifndef XvExtension
void
LXInitVideo(ScreenPtr pScrn)
{
}

void
LXResetVideo(ScrnInfoPtr pScrni)
{
}

void
LXSetVideoPosition(int x, int y, int width, int height,
    short src_w, short src_h, short drw_w,
    short drw_h, int id, int offset, ScrnInfoPtr pScrni)
{
}
#else

#define DBUF 1
void LXInitVideo(ScreenPtr pScrn);
void LXResetVideo(ScrnInfoPtr pScrni);
static XF86VideoAdaptorPtr LXSetupImageVideo(ScreenPtr);
static void LXInitOffscreenImages(ScreenPtr);
static void LXStopVideo(ScrnInfoPtr, pointer, Bool);
static int LXSetPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
static int LXGetPortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
static void LXQueryBestSize(ScrnInfoPtr, Bool,
    short, short, short, short, unsigned int *, unsigned int *, pointer);
static int LXPutImage(ScrnInfoPtr, short, short, short, short, short, short,
    short, short, int, unsigned char *, short, short, Bool,
    RegionPtr, pointer);
static int LXQueryImageAttributes(ScrnInfoPtr, int, unsigned short *,
    unsigned short *, int *, int *);

static void LXBlockHandler(int, pointer, pointer, pointer);
void LXSetVideoPosition(int x, int y, int width, int height,
    short src_w, short src_h, short drw_w,
    short drw_h, int id, int offset, ScrnInfoPtr pScrni);

extern void LXAccelSync(ScrnInfoPtr pScrni);

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvColorKey, xvColorKeyMode, xvFilter;

#if DBUF
static Atom xvDoubleBuffer;
#endif

/*----------------------------------------------------------------------------
 * LXInitVideo
 *
 * Description:  This is the initialization routine. It creates a new video
 *               adapter and calls LXSetupImageVideo to initialize it by
 *				 filling XF86VideoAdaptorREc. Then it lists the existing
 *				 adaptors and adds the new one to it. Finally the list of
 *				 XF86VideoAdaptorPtr pointers are passed to the
 *				 xf86XVScreenInit().
 *
 * Parameters.
 * ScreenPtr
 *		pScrn	:Screen handler pointer having screen information.
 *
 * Returns		:none
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/
void
LXInitVideo(ScreenPtr pScrn)
{
    GeodeRec *pGeode;
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];

    DBLOG(1, "LXInitVideo()\n");

    pGeode = GEODEPTR(pScrni);

    if (!pGeode->NoAccel) {
        XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
        XF86VideoAdaptorPtr newAdaptor = NULL;

        int num_adaptors;

        newAdaptor = LXSetupImageVideo(pScrn);
        LXInitOffscreenImages(pScrn);

        num_adaptors = xf86XVListGenericAdaptors(pScrni, &adaptors);

        if (newAdaptor) {
            if (!num_adaptors) {
                num_adaptors = 1;
                adaptors = &newAdaptor;
            } else {
                newAdaptors =          /* need to free this someplace */
                    xalloc((num_adaptors +
                        1) * sizeof(XF86VideoAdaptorPtr *));
                if (newAdaptors) {
                    memcpy(newAdaptors, adaptors, num_adaptors *
                        sizeof(XF86VideoAdaptorPtr));
                    newAdaptors[num_adaptors] = newAdaptor;
                    adaptors = newAdaptors;
                    num_adaptors++;
                }
            }
        }

        if (num_adaptors)
            xf86XVScreenInit(pScrn, adaptors, num_adaptors);

        if (newAdaptors)
            xfree(newAdaptors);
    }
}

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] = {
    {
            0,
            "XV_IMAGE",
            1024, 1024,
            {1, 1}
        }
};

#define NUM_FORMATS 4

static XF86VideoFormatRec Formats[NUM_FORMATS] = {
    {8, PseudoColor}, {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

#if DBUF
#define NUM_ATTRIBUTES 4
#else
#define NUM_ATTRIBUTES 3
#endif

static XF86AttributeRec Attributes[NUM_ATTRIBUTES] = {
#if DBUF
    {XvSettable | XvGettable, 0, 1, "XV_DOUBLE_BUFFER"},
#endif
    {XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
    {XvSettable | XvGettable, 0, 1, "XV_FILTER"},
    {XvSettable | XvGettable, 0, 1, "XV_COLORKEYMODE"}
};

static XF86ImageRec Images[] = {
    XVIMAGE_UYVY,
    XVIMAGE_YUY2,
    XVIMAGE_Y2YU,
    XVIMAGE_YVYU,
    XVIMAGE_Y800,
    XVIMAGE_I420,
    XVIMAGE_YV12
};

#define NUM_IMAGES (sizeof(Images)/sizeof(Images[0]));

typedef struct
{
    FBAreaPtr area;
    FBLinearPtr linear;
    RegionRec clip;
    CARD32 filter;
    CARD32 colorKey;
    CARD32 colorKeyMode;
    CARD32 videoStatus;
    Time offTime;
    Time freeTime;
#if DBUF
    Bool doubleBuffer;
    int currentBuffer;
#endif
}
GeodePortPrivRec, *GeodePortPrivPtr;

#define GET_PORT_PRIVATE(pScrni) \
   (GeodePortPrivRec *)((GEODEPTR(pScrni))->adaptor->pPortPrivates[0].ptr)

/*----------------------------------------------------------------------------
 * LXSetColorKey
 *
 * Description	:This function reads the color key for the pallete and
 *				  sets the video color key register.
 *
 * Parameters.
 * ScreenInfoPtr
 *		pScrni	:Screen  pointer having screen information.
 *		pPriv	:Video port private data
 *
 * Returns		:none
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/
static INT32
LXSetColorkey(ScrnInfoPtr pScrni, GeodePortPrivRec * pPriv)
{
    int red, green, blue;
    unsigned long key;

    switch (pScrni->depth) {
    case 8:
        vg_get_display_palette_entry(pPriv->colorKey & 0xFF, &key);
        red = ((key >> 16) & 0xFF);
        green = ((key >> 8) & 0xFF);
        blue = (key & 0xFF);
        break;
    case 16:
        red = (pPriv->colorKey & pScrni->mask.red) >>
            pScrni->offset.red << (8 - pScrni->weight.red);
        green = (pPriv->colorKey & pScrni->mask.green) >>
            pScrni->offset.green << (8 - pScrni->weight.green);
        blue = (pPriv->colorKey & pScrni->mask.blue) >>
            pScrni->offset.blue << (8 - pScrni->weight.blue);
        break;
    default:
        /* for > 16 bpp we send in the mask in xf86SetWeight. This
         * function is providing the offset by 1 more. So we take 
         * this as a special case and subtract 1 for > 16
         */
        red = (pPriv->colorKey & pScrni->mask.red) >>
            (pScrni->offset.red - 1) << (8 - pScrni->weight.red);
        green = (pPriv->colorKey & pScrni->mask.green) >>
            (pScrni->offset.green - 1) << (8 - pScrni->weight.green);
        blue = (pPriv->colorKey & pScrni->mask.blue) >>
            (pScrni->offset.blue - 1) << (8 - pScrni->weight.blue);
        break;
    }

    DBLOG(1, "LXSetColorkey() %08x %d\n", blue | (green << 8) | (red << 16),
        pPriv->colorKeyMode);
    if (pPriv->colorKeyMode != 0)
        df_set_video_color_key((blue | (green << 8) | (red << 16)),
            0xFFFFFF, 1);
    else
        df_set_video_color_key(0, 0, 1);

    REGION_EMPTY(pScrni->pScreen, &pPriv->clip);

    return 0;
}

/*----------------------------------------------------------------------------
 * LXResetVideo
 *
 * Description	: This function resets the video
 *
 * Parameters.
 * ScreenInfoPtr
 *		pScrni	:Screen  pointer having screen information.
 *
 * Returns		:None
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/

void
LXResetVideo(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(1, "LXResetVideo()\n");

    if (!pGeode->NoAccel) {
        GeodePortPrivRec *pPriv = pGeode->adaptor->pPortPrivates[0].ptr;

        LXAccelSync(pScrni);
        df_set_video_palette(NULL);
        LXSetColorkey(pScrni, pPriv);
    }
}

/*----------------------------------------------------------------------------
 * LXSetupImageVideo
 *
 * Description	: This function allocates space for a Videoadaptor and initializes
 *				  the XF86VideoAdaptorPtr record.
 *
 * Parameters.
 * ScreenPtr
 *		pScrn	:Screen handler pointer having screen information.
 *
 * Returns		:XF86VideoAdaptorPtr :- pointer to the initialized video adaptor record.
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/

static XF86VideoAdaptorPtr
LXSetupImageVideo(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86Screens[pScrn->myNum];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    XF86VideoAdaptorPtr adapt;
    GeodePortPrivRec *pPriv;

    DBLOG(1, "LXSetupImageVideo()\n");

    if (!(adapt = xcalloc(1, sizeof(XF86VideoAdaptorRec) +
                sizeof(GeodePortPrivRec) + sizeof(DevUnion))))
        return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    adapt->name = "Advanced Micro Devices";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncoding;
    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = Formats;
    adapt->nPorts = 1;
    adapt->pPortPrivates = (DevUnion *) (&adapt[1]);
    pPriv = (GeodePortPrivRec *) (&adapt->pPortPrivates[1]);
    adapt->pPortPrivates[0].ptr = (pointer) (pPriv);
    adapt->pAttributes = Attributes;
    adapt->nImages = NUM_IMAGES;
    adapt->nAttributes = NUM_ATTRIBUTES;
    adapt->pImages = Images;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = LXStopVideo;
    adapt->SetPortAttribute = LXSetPortAttribute;
    adapt->GetPortAttribute = LXGetPortAttribute;
    adapt->QueryBestSize = LXQueryBestSize;
    adapt->PutImage = LXPutImage;
    adapt->QueryImageAttributes = LXQueryImageAttributes;

    pPriv->filter = 0;
    pPriv->colorKey = pGeode->videoKey;
    pPriv->colorKeyMode = 0;
    pPriv->videoStatus = 0;
#if DBUF
    pPriv->doubleBuffer = TRUE;
    pPriv->currentBuffer = 0;          /* init to first buffer */
#endif

    /* gotta uninit this someplace */
#if defined(REGION_NULL)
    REGION_NULL(pScrn, &pPriv->clip);
#else
    REGION_INIT(pScrn, &pPriv->clip, NullBox, 0);
#endif

    pGeode->adaptor = adapt;

    pGeode->BlockHandler = pScrn->BlockHandler;
    pScrn->BlockHandler = LXBlockHandler;

    xvColorKey = MAKE_ATOM("XV_COLORKEY");
    xvColorKeyMode = MAKE_ATOM("XV_COLORKEYMODE");
    xvFilter = MAKE_ATOM("XV_FILTER");
#if DBUF
    xvDoubleBuffer = MAKE_ATOM("XV_DOUBLE_BUFFER");
#endif

    LXResetVideo(pScrni);

    return adapt;
}

/*----------------------------------------------------------------------------
 * LXStopVideo
 *
 * Description	:This function is used to stop input and output video
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *		exit		:Flag indicating whether the offscreen areas used for video
 *					 to be deallocated or not.
 * Returns		:none
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/
static void
LXStopVideo(ScrnInfoPtr pScrni, pointer data, Bool exit)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    DBLOG(1, "LXStopVideo()\n");

    REGION_EMPTY(pScrni->pScreen, &pPriv->clip);

    LXAccelSync(pScrni);
    if (exit) {
        if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
            df_set_video_enable(0, 0);
        }
        if (pPriv->area) {
            xf86FreeOffscreenArea(pPriv->area);
            pPriv->area = NULL;
        }
        pPriv->videoStatus = 0;
        pGeode->OverlayON = FALSE;
    } else {
        if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
            pPriv->videoStatus |= OFF_TIMER;
            pPriv->offTime = currentTime.milliseconds + OFF_DELAY;
        }
    }
}

/*----------------------------------------------------------------------------
 * LXSetPortAttribute
 *
 * Description	:This function is used to set the attributes of a port like colorkeymode,
 *				  double buffer support and filter.
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *		attribute	:The port attribute to be set
 *		value		:Value of the attribute to be set.  
 *					 
 * Returns		:Sucess if the attribute is supported, else BadMatch
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/
static int
LXSetPortAttribute(ScrnInfoPtr pScrni,
    Atom attribute, INT32 value, pointer data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    DBLOG(1, "LXSetPortAttribute(%d,%#x)\n", attribute, value);

    LXAccelSync(pScrni);
    if (attribute == xvColorKey) {
        pPriv->colorKey = value;
        LXSetColorkey(pScrni, pPriv);
    }
#if DBUF
    else if (attribute == xvDoubleBuffer) {
        if ((value < 0) || (value > 1))
            return BadValue;
        pPriv->doubleBuffer = value;
    }
#endif
    else if (attribute == xvColorKeyMode) {
        pPriv->colorKeyMode = value;
        LXSetColorkey(pScrni, pPriv);
    } else if (attribute == xvFilter) {
        if ((value < 0) || (value > 1))
            return BadValue;
        pPriv->filter = value;
    } else
        return BadMatch;

    return Success;
}

/*----------------------------------------------------------------------------
 * LXGetPortAttribute
 *
 * Description	:This function is used to get the attributes of a port like hue,
 *				 saturation,brightness or contrast.
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *		attribute	:The port attribute to be read
 *		value		:Pointer to the value of the attribute to be read.  
 *					 
 * Returns		:Sucess if the attribute is supported, else BadMatch
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/
static int
LXGetPortAttribute(ScrnInfoPtr pScrni,
    Atom attribute, INT32 * value, pointer data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    DBLOG(1, "LXGetPortAttribute(%d)\n", attribute);

    if (attribute == xvColorKey) {
        *value = pPriv->colorKey;
    }
#if DBUF
    else if (attribute == xvDoubleBuffer) {
        *value = (pPriv->doubleBuffer) ? 1 : 0;
    }
#endif
    else if (attribute == xvColorKeyMode) {
        *value = pPriv->colorKeyMode;
    } else if (attribute == xvFilter) {
        *value = pPriv->filter;
    } else
        return BadMatch;

    return Success;
}

/*----------------------------------------------------------------------------
 * LXQueryBestSize
 *
 * Description	:
 *	This function provides a way to query what the destination dimensions
 *	would end up being if they were to request that an area vid_w by vid_h
 *  from the video stream be scaled to rectangle of drw_w by drw_h on 
 *	the screen.
 *
 * Parameters.
 * ScreenInfoPtr
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *      vid_w,vid_h	:Width and height of the video data.
 *		drw_w,drw_h :Width and height of the scaled rectangle.
 *		p_w,p_h		:Width and height of the destination rectangle. 
 *					 
 * Returns		:None
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/
static void
LXQueryBestSize(ScrnInfoPtr pScrni, Bool motion, short vid_w, short vid_h,
    short drw_w, short drw_h, unsigned int *p_w,
    unsigned int *p_h, pointer data)
{
    *p_w = drw_w;
    *p_h = drw_h;

    if (*p_w > 16384)
        *p_w = 16384;

    DBLOG(1, "LXQueryBestSize(%d, src %dx%d scl %dx%d dst %dx%d)\n", motion,
        vid_w, vid_h, drw_w, drw_h, *p_w, *p_h);
}

static void
LXCopyGreyscale(unsigned char *src, unsigned char *dst, int srcp, int dstp,
    int h, int w)
{
    int i;
    unsigned char *src2 = src;
    unsigned char *dst2 = dst;
    unsigned char *dst3;
    unsigned char *src3;

    dstp <<= 1;

    while (h--) {
        dst3 = dst2;
        src3 = src2;
        for (i = 0; i < w; i++) {
            *dst3++ = *src3++;         /* Copy Y data */
            *dst3++ = 0x80;            /* Fill UV with 0x80 - greyscale */
        }
        src3 = src2;
        for (i = 0; i < w; i++) {
            *dst3++ = *src3++;         /* Copy Y data */
            *dst3++ = 0x80;            /* Fill UV with 0x80 - greyscale */
        }
        dst2 += dstp;
        src2 += srcp;
    }
}

/*----------------------------------------------------------------------------
 * LXCopyData420
 *
 * Description	: Copies data from src to destination
 *
 * Parameters.
 *		src	: pointer to the source data
 *		dst	: pointer to destination data
 *		srcp	: pitch of the srcdata
 *		dstp	: pitch of the destination data 
 *		h & w	: height and width of source data
 *					 
 * Returns		:None
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/

static void
LXCopyData420(unsigned char *src, unsigned char *dst, int srcp, int dstp,
    int h, int w)
{
    while (h--) {
        memcpy(dst, src, w);
        src += srcp;
        dst += dstp;
    }
}

/*----------------------------------------------------------------------------
 * LXCopyData422
 *
 * Description	: Copies data from src to destination
 *
 * Parameters.
 *		src			: pointer to the source data
 *		dst			: pointer to destination data
 *		srcp	: pitch of the srcdata
 *		dstp	: pitch of the destination data 
 *		h & w		: height and width of source data
 *					 
 * Returns		:None
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/

static void
LXCopyData422(unsigned char *src, unsigned char *dst,
    int srcp, int dstp, int h, int w)
{
    w <<= 1;
    while (h--) {
        memcpy(dst, src, w);
        src += srcp;
        dst += dstp;
    }
}

static FBAreaPtr
LXAllocateMemory(ScrnInfoPtr pScrni, FBAreaPtr area, int numlines)
{
    ScreenPtr pScrn = screenInfo.screens[pScrni->scrnIndex];
    FBAreaPtr new_area;

    if (area) {
        if ((area->box.y2 - area->box.y1) >= numlines)
            return area;

        if (xf86ResizeOffscreenArea(area, pScrni->displayWidth, numlines))
            return area;

        xf86FreeOffscreenArea(area);
    }

    new_area = xf86AllocateOffscreenArea(pScrn, pScrni->displayWidth,
        numlines, 0, NULL, NULL, NULL);

    if (!new_area) {
        int max_w, max_h;

        xf86QueryLargestOffscreenArea(pScrn, &max_w, &max_h, 0,
            FAVOR_WIDTH_THEN_AREA, PRIORITY_EXTREME);

        if ((max_w < pScrni->displayWidth) || (max_h < numlines))
            return NULL;

        xf86PurgeUnlockedOffscreenAreas(pScrn);
        new_area = xf86AllocateOffscreenArea(pScrn, pScrni->displayWidth,
            numlines, 0, NULL, NULL, NULL);
    }

    return new_area;
}

static BoxRec dstBox;
static int srcPitch = 0, srcPitch2 = 0, dstPitch = 0, dstPitch2 = 0;
static INT32 Bx1, Bx2, By1, By2;
static int top, left, npixels, nlines;
static int offset, s1offset = 0, s2offset = 0, s3offset = 0;
static unsigned char *dst_start;
static int d2offset = 0, d3offset = 0;

static DF_VIDEO_SOURCE_PARAMS vSrcParams;

void
LXSetVideoPosition(int x, int y, int width, int height,
    short src_w, short src_h, short drw_w, short drw_h,
    int id, int offset, ScrnInfoPtr pScrni)
{
    long ystart, xend, yend;
    unsigned long lines = 0;
    unsigned long y_extra, uv_extra = 0;
    DF_VIDEO_POSITION vidPos;

    DBLOG(1, "LXSetVideoPosition(%d,%d %dx%d, src %dx%d, dst %dx%d, id %d, "
        "ofs %d)\n", x, y, width, height, src_w, src_h, drw_w, drw_h,
        id, offset);

    xend = x + drw_w;
    yend = y + drw_h;

    /* TOP CLIPPING */

    if (y < 0) {
        if (src_h < drw_h)
            lines = (-y) * src_h / drw_h;
        else
            lines = (-y);
        ystart = 0;
        drw_h += y;
        y_extra = lines * dstPitch;
        uv_extra = (lines >> 1) * (dstPitch2);
    } else {
        ystart = y;
        lines = 0;
        y_extra = 0;
    }

    memset(&vidPos, 0, sizeof(vidPos));
    vidPos.x = x;
    vidPos.y = ystart;
    vidPos.width = xend - x;
    vidPos.height = yend - ystart;

    DBLOG(1, "video_pos %d,%d %dx%d\n", vidPos.x, vidPos.y, vidPos.width,
        vidPos.height);
    df_set_video_position(&vidPos);

    vSrcParams.y_offset = offset + y_extra;
    if ((id == FOURCC_Y800) || (id == FOURCC_I420) || (id == FOURCC_YV12)) {
        vSrcParams.u_offset = offset + d3offset + uv_extra;
        vSrcParams.v_offset = offset + d2offset + uv_extra;
    } else {
        vSrcParams.u_offset = vSrcParams.v_offset = 0;
    }
    vSrcParams.flags = DF_SOURCEFLAG_IMPLICITSCALING;

    DBLOG(1, "video_format %#x yofs %#x uofs %#x vofs %#x yp %d uvp %d wh "
        "%dx%d flg %#x\n", vSrcParams.video_format, vSrcParams.y_offset,
        vSrcParams.u_offset, vSrcParams.v_offset, vSrcParams.y_pitch,
        vSrcParams.uv_pitch, vSrcParams.width, vSrcParams.height,
        vSrcParams.flags);

    df_configure_video_source(&vSrcParams, &vSrcParams);
}

/*----------------------------------------------------------------------------
 * LXDisplayVideo
 *
 * Description	: This function sets up the video registers for playing video
 *		  It sets up the video format,width, height & position of the 
 *		  video window ,video offsets( y,u,v) and video pitches(y,u,v)	
 * Parameters.
 *					 
 * Returns	:None
 *
 * Comments	:None
 *
*----------------------------------------------------------------------------
*/

static void
LXDisplayVideo(ScrnInfoPtr pScrni, int id, int offset, short width,
    short height, int pitch, int x1, int y1, int x2, int y2,
    BoxPtr dstBox, short src_w, short src_h, short drw_w, short drw_h)
{
    DBLOG(1, "LXDisplayVideo(id %d, ofs %d, %dx%d, p %d, %d,%d, %d,%d, src "
        "%dx%d dst %dx%d)\n", id, offset, width, height, pitch, x1, y1,
        x2, y2, src_w, src_h, drw_w, drw_h);

    LXAccelSync(pScrni);

    switch (id) {
    case FOURCC_UYVY:
        vSrcParams.video_format = DF_VIDFMT_UYVY;
        break;
    case FOURCC_Y800:
    case FOURCC_YV12:
    case FOURCC_I420:
        vSrcParams.video_format = DF_VIDFMT_Y0Y1Y2Y3;
        break;
    case FOURCC_YUY2:
        vSrcParams.video_format = DF_VIDFMT_YUYV;
        break;
    case FOURCC_Y2YU:
        vSrcParams.video_format = DF_VIDFMT_Y2YU;
        break;
    case FOURCC_YVYU:
        vSrcParams.video_format = DF_VIDFMT_YVYU;
        break;
    }

    vSrcParams.width = width;
    vSrcParams.height = height;
    vSrcParams.y_pitch = dstPitch;
    vSrcParams.uv_pitch = dstPitch2;

    df_set_video_filter_coefficients(NULL, 1);
    if ((drw_w >= src_w) && (drw_h >= src_h))
        df_set_video_scale(width, height, drw_w, drw_h,
            DF_SCALEFLAG_CHANGEX | DF_SCALEFLAG_CHANGEY);
    else if (drw_w < src_w)
        df_set_video_scale(drw_w, height, drw_w, drw_h,
            DF_SCALEFLAG_CHANGEX | DF_SCALEFLAG_CHANGEY);
    else if (drw_h < src_h)
        df_set_video_scale(width, drw_h, drw_w, drw_h,
            DF_SCALEFLAG_CHANGEX | DF_SCALEFLAG_CHANGEY);

    LXSetVideoPosition(dstBox->x1, dstBox->y1, width, height, src_w,
        src_h, drw_w, drw_h, id, offset, pScrni);

    df_set_video_enable(1, 0);
}

#if REINIT
static Bool
RegionsEqual(RegionPtr A, RegionPtr B)
{
    int *dataA, *dataB;
    int num;

    num = REGION_NUM_RECTS(A);
    if (num != REGION_NUM_RECTS(B)) {
        return FALSE;
    }

    if ((A->extents.x1 != B->extents.x1) ||
        (A->extents.x2 != B->extents.x2) ||
        (A->extents.y1 != B->extents.y1) || (A->extents.y2 != B->extents.y2))
        return FALSE;

    dataA = (int *)REGION_RECTS(A);
    dataB = (int *)REGION_RECTS(B);

    while (num--) {
        if ((dataA[0] != dataB[0]) || (dataA[1] != dataB[1]))
            return FALSE;
        dataA += 2;
        dataB += 2;
    }

    return TRUE;
}
#endif

/*----------------------------------------------------------------------------
 * LXPutImage	:
 *	This function writes a single frame of video into a drawable.
 *	The position and size of the source rectangle is specified by src_x,src_y,
 *	src_w and src_h. This data is stored in a system memory buffer at buf.  
 *	The position and size of the destination rectangle is specified by drw_x,
 *  drw_y,drw_w,drw_h.The data is in the format indicated by the image
 *	descriptor and represents a source of size width by height.  If sync is
 *	TRUE the driver should not return from this function until it is through
 *	reading the data from buf.  Returning when sync is TRUE indicates that
 *	it is safe for the data at buf to be replaced,freed, or modified.
 *
 * Description		: 
 * Parameters.
 *					 
 * Returns		:None
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/

static int
LXPutImage(ScrnInfoPtr pScrni, short src_x, short src_y, short drw_x,
    short drw_y, short src_w, short src_h, short drw_w, short drw_h,
    int id, unsigned char *buf, short width, short height, Bool sync,
    RegionPtr clipBoxes, pointer data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int new_h;

#if REINIT
    BOOL ReInitVideo = FALSE;
    static BOOL DoReinitAgain = 0;
#endif

#if XV_PROFILE
    long oldtime, newtime;

    UpdateCurrentTime();
    oldtime = currentTime.milliseconds;
#endif
    DBLOG(1, "LXPutImage(src %d,%d %dx%d dst %d,%d %dx%d, id %d %dx%d sync "
        "%d)\n", src_x, src_y, src_w, src_h, drw_x, drw_y, drw_w, drw_h,
        id, width, height, sync);

#if REINIT
    /* update cliplist */
    if (!RegionsEqual(&pPriv->clip, clipBoxes)) {
        ReInitVideo = TRUE;
    }

    if (DoReinitAgain)
        ReInitVideo = TRUE;

    if (ReInitVideo) {
        DBLOG(1, "Regional Not Equal - Init\n");
#endif
        DoReinitAgain = ~DoReinitAgain;
        if (drw_w > 16384)
            drw_w = 16384;

        /* Clip */
        Bx1 = src_x;
        Bx2 = src_x + src_w;
        By1 = src_y;
        By2 = src_y + src_h;

        if ((Bx1 >= Bx2) || (By1 >= By2))
            return Success;

        dstBox.x1 = drw_x;
        dstBox.x2 = drw_x + drw_w;
        dstBox.y1 = drw_y;
        dstBox.y2 = drw_y + drw_h;

        dstBox.x1 -= pScrni->frameX0;
        dstBox.x2 -= pScrni->frameX0;
        dstBox.y1 -= pScrni->frameY0;
        dstBox.y2 -= pScrni->frameY0;

        switch (id) {
        case FOURCC_YV12:
        case FOURCC_I420:
            srcPitch = (width + 3) & ~3;        /* of luma */
            dstPitch = (width + 31) & ~31;

            s2offset = srcPitch * height;
            d2offset = dstPitch * height;

            srcPitch2 = ((width >> 1) + 3) & ~3;
            dstPitch2 = ((width >> 1) + 15) & ~15;

            s3offset = (srcPitch2 * (height >> 1)) + s2offset;
            d3offset = (dstPitch2 * (height >> 1)) + d2offset;

            new_h = dstPitch * height; /* Y */
            new_h += (dstPitch2 * height);      /* U+V */
            new_h += pGeode->Pitch - 1;
            new_h /= pGeode->Pitch;
            break;

        case FOURCC_UYVY:
        case FOURCC_YUY2:
        case FOURCC_Y800:
        default:
            dstPitch = ((width << 1) + 3) & ~3;
            srcPitch = (width << 1);
            new_h = ((dstPitch * height) + pGeode->Pitch - 1) / pGeode->Pitch;
            break;
        }

#if DBUF
        if (pPriv->doubleBuffer)
            new_h <<= 1;
#endif

        if (!(pPriv->area = LXAllocateMemory(pScrni, pPriv->area, new_h)))
            return BadAlloc;

        /* copy data */
        top = By1;
        left = Bx1 & ~1;
        npixels = ((Bx2 + 1) & ~1) - left;

        switch (id) {
        case FOURCC_YV12:
        case FOURCC_I420:{
                int tmp;

                top &= ~1;
                offset = (pPriv->area->box.y1 * pGeode->Pitch) +
                    (top * dstPitch);

#if DBUF
                if (pPriv->doubleBuffer && pPriv->currentBuffer)
                    offset += (new_h >> 1) * pGeode->Pitch;
#endif

                dst_start = pGeode->FBBase + offset + left;
                tmp = ((top >> 1) * srcPitch2) + (left >> 1);
                s2offset += tmp;
                s3offset += tmp;
                if (id == FOURCC_I420) {
                    tmp = s2offset;
                    s2offset = s3offset;
                    s3offset = tmp;
                }
                nlines = ((By2 + 1) & ~1) - top;
            }
            break;

        case FOURCC_UYVY:
        case FOURCC_YUY2:
        case FOURCC_Y800:
        default:
            left <<= 1;
            buf += (top * srcPitch) + left;
            nlines = By2 - top;
            offset = (pPriv->area->box.y1 * pGeode->Pitch) + (top * dstPitch);
#if DBUF
            if (pPriv->doubleBuffer && pPriv->currentBuffer)
                offset += (new_h >> 1) * pGeode->Pitch;
#endif

            dst_start = pGeode->FBBase + offset + left;
            break;
        }
        s1offset = (top * srcPitch) + left;

#if REINIT
        /* update cliplist */
        REGION_COPY(pScrni->pScreen, &pPriv->clip, clipBoxes);
        if (pPriv->colorKeyMode == 0) {
            /* draw these */
            XAAFillSolidRects(pScrni, pPriv->colorKey, GXcopy, ~0,
                REGION_NUM_RECTS(clipBoxes), REGION_RECTS(clipBoxes));
        }
        LXDisplayVideo(pScrni, id, offset, width, height, dstPitch,
            Bx1, By1, Bx2, By2, &dstBox, src_w, src_h, drw_w, drw_h);
    }
#endif

    switch (id) {
    case FOURCC_Y800:
        LXCopyGreyscale(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
        break;
    case FOURCC_YV12:
    case FOURCC_I420:
        LXCopyData420(buf + s1offset, dst_start, srcPitch, dstPitch, nlines,
            npixels);
        LXCopyData420(buf + s2offset, dst_start + d2offset, srcPitch2,
            dstPitch2, nlines >> 1, npixels >> 1);
        LXCopyData420(buf + s3offset, dst_start + d3offset, srcPitch2,
            dstPitch2, nlines >> 1, npixels >> 1);
        break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
        LXCopyData422(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
        break;
    }
#if !REINIT
    /* update cliplist */
    REGION_COPY(pScrni->pScreen, &pPriv->clip, clipBoxes);
    if (pPriv->colorKeyMode == 0) {
        /* draw these */
        XAAFillSolidRects(pScrni, pPriv->colorKey, GXcopy, ~0,
            REGION_NUM_RECTS(clipBoxes), REGION_RECTS(clipBoxes));
    }
    LXDisplayVideo(pScrni, id, offset, width, height, dstPitch,
        Bx1, By1, Bx2, By2, &dstBox, src_w, src_h, drw_w, drw_h);
#endif

#if XV_PROFILE
    UpdateCurrentTime();
    newtime = currentTime.milliseconds;
    DBLOG(1, "PI %d\n", newtime - oldtime);
#endif

#if DBUF
    pPriv->currentBuffer ^= 1;
#endif

    pPriv->videoStatus = CLIENT_VIDEO_ON;
    pGeode->OverlayON = TRUE;

    return Success;
}

/*----------------------------------------------------------------------------
 * LXQueryImageAttributes
 *
 * Description	:This function is called to let the driver specify how data
 *				 for a particular image of size width by height should be 
 *				 stored. 		
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		id		:Id for the video format
 *		width		:width  of the image (can be modified by the driver)  
 *		height		:height of the image (can be modified by the driver)  
 * Returns		: Size of the memory required for storing this image
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/
static int
LXQueryImageAttributes(ScrnInfoPtr pScrni, int id, unsigned short *w,
    unsigned short *h, int *pitches, int *offsets)
{
    int size;
    int tmp;

    if (*w > 1024)
        *w = 1024;
    if (*h > 1024)
        *h = 1024;

    *w = (*w + 1) & ~1;
    if (offsets)
        offsets[0] = 0;

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        *h = (*h + 1) & ~1;
        size = (*w + 3) & ~3;
        if (pitches)
            pitches[0] = size;
        size *= *h;
        if (offsets)
            offsets[1] = size;
        tmp = ((*w >> 1) + 3) & ~3;
        if (pitches)
            pitches[1] = pitches[2] = tmp;
        tmp *= (*h >> 1);
        size += tmp;
        if (offsets)
            offsets[2] = size;
        size += tmp;
        break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    case FOURCC_Y800:
    default:
        size = *w << 1;
        if (pitches)
            pitches[0] = size;
        size *= *h;
        break;
    }

    DBLOG(1, "LXQueryImageAttributes(%d)= %d, %dx%d %d/%d/%d %d/%d/%d\n",
        id, size, *w, *h, pitches[0], pitches[1], pitches[2], offsets[0],
        offsets[1], offsets[2]);

    return size;
}

static void
LXBlockHandler(int i, pointer blockData, pointer pTimeout, pointer pReadmask)
{
    ScreenPtr pScrn = screenInfo.screens[i];
    ScrnInfoPtr pScrni = xf86Screens[i];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    GeodePortPrivRec *pPriv = GET_PORT_PRIVATE(pScrni);

    pScrn->BlockHandler = pGeode->BlockHandler;
    (*pScrn->BlockHandler) (i, blockData, pTimeout, pReadmask);
    pScrn->BlockHandler = LXBlockHandler;

    if (pPriv->videoStatus & TIMER_MASK) {
        DBLOG(1, "LXBlockHandler(%d)\n", i);
        LXAccelSync(pScrni);
        UpdateCurrentTime();
        if (pPriv->videoStatus & OFF_TIMER) {
            if (pPriv->offTime < currentTime.milliseconds) {
                df_set_video_enable(0, 0);
                pPriv->videoStatus = FREE_TIMER;
                pPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
            }
        } else {                       /* FREE_TIMER */
            if (pPriv->freeTime < currentTime.milliseconds) {
                if (pPriv->area) {
                    xf86FreeOffscreenArea(pPriv->area);
                    pPriv->area = NULL;
                }
                pPriv->videoStatus = 0;
            }
        }
    }
}

/****************** Offscreen stuff ***************/

typedef struct
{
    FBAreaPtr area;
    FBLinearPtr linear;
    Bool isOn;
}
OffscreenPrivRec, *OffscreenPrivPtr;

/*----------------------------------------------------------------------------
 * LXAllocateSurface
 *
 * Description	:This function allocates an area of w by h in the offscreen
 * Parameters.
 *		pScrni	:Screen handler pointer having screen information.
 * 
 * Returns		:None
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/

static int
LXAllocateSurface(ScrnInfoPtr pScrni, int id, unsigned short w,
    unsigned short h, XF86SurfacePtr surface)
{
    FBAreaPtr area;
    int pitch, fbpitch, numlines;
    OffscreenPrivRec *pPriv;

    DBLOG(1, "LXAllocateSurface(id %d, %dx%d)\n", id, w, h);

    if ((w > 1024) || (h > 1024))
        return BadAlloc;

    w = (w + 1) & ~1;
    pitch = ((w << 1) + 15) & ~15;
    fbpitch = pScrni->bitsPerPixel * pScrni->displayWidth >> 3;
    numlines = ((pitch * h) + fbpitch - 1) / fbpitch;

    if (!(area = LXAllocateMemory(pScrni, NULL, numlines)))
        return BadAlloc;

    surface->width = w;
    surface->height = h;

    if (!(surface->pitches = xalloc(sizeof(int))))
        return BadAlloc;
    if (!(surface->offsets = xalloc(sizeof(int)))) {
        xfree(surface->pitches);
        return BadAlloc;
    }
    if (!(pPriv = xalloc(sizeof(OffscreenPrivRec)))) {
        xfree(surface->pitches);
        xfree(surface->offsets);
        return BadAlloc;
    }

    pPriv->area = area;
    pPriv->isOn = FALSE;

    surface->pScrn = pScrni;
    surface->id = id;
    surface->pitches[0] = pitch;
    surface->offsets[0] = area->box.y1 * fbpitch;
    surface->devPrivate.ptr = (pointer) pPriv;

    return Success;
}

static int
LXStopSurface(XF86SurfacePtr surface)
{
    OffscreenPrivRec *pPriv = (OffscreenPrivRec *) surface->devPrivate.ptr;

    DBLOG(1, "LXStopSurface()\n");

    if (pPriv->isOn) {
        pPriv->isOn = FALSE;
    }

    return Success;
}

static int
LXFreeSurface(XF86SurfacePtr surface)
{
    OffscreenPrivRec *pPriv = (OffscreenPrivRec *) surface->devPrivate.ptr;

    DBLOG(1, "LXFreeSurface()\n");

    if (pPriv->isOn)
        LXStopSurface(surface);

    xf86FreeOffscreenArea(pPriv->area);
    xfree(surface->pitches);
    xfree(surface->offsets);
    xfree(surface->devPrivate.ptr);

    return Success;
}

static int
LXGetSurfaceAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 * value)
{
    return LXGetPortAttribute(pScrni, attribute, value,
        (pointer) (GET_PORT_PRIVATE(pScrni)));
}

static int
LXSetSurfaceAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 value)
{
    return LXSetPortAttribute(pScrni, attribute, value,
        (pointer) (GET_PORT_PRIVATE(pScrni)));
}

static int
LXDisplaySurface(XF86SurfacePtr surface, short src_x, short src_y,
    short drw_x, short drw_y, short src_w, short src_h,
    short drw_w, short drw_h, RegionPtr clipBoxes)
{
    OffscreenPrivRec *pPriv = (OffscreenPrivRec *) surface->devPrivate.ptr;
    ScrnInfoPtr pScrni = surface->pScrn;
    GeodePortPrivRec *portPriv = GET_PORT_PRIVATE(pScrni);
    INT32 x1, y1, x2, y2;
    BoxRec dstBox;

    DBLOG(1, "LXDisplaySurface(src %d,%d %dx%d, dst %d,%d %dx%d)\n",
        src_x, src_y, src_w, src_h, drw_x, drw_y, drw_w, drw_h);

    DEBUGMSG(0, (0, X_NONE, "DisplaySuface\n"));
    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

    if ((x1 >= x2) || (y1 >= y2))
        return Success;

    dstBox.x1 -= pScrni->frameX0;
    dstBox.x2 -= pScrni->frameX0;
    dstBox.y1 -= pScrni->frameY0;
    dstBox.y2 -= pScrni->frameY0;

    xf86XVFillKeyHelper(pScrni->pScreen, portPriv->colorKey, clipBoxes);

    LXDisplayVideo(pScrni, surface->id, surface->offsets[0],
        surface->width, surface->height, surface->pitches[0],
        x1, y1, x2, y2, &dstBox, src_w, src_h, drw_w, drw_h);

    pPriv->isOn = TRUE;
    if (portPriv->videoStatus & CLIENT_VIDEO_ON) {
        REGION_EMPTY(pScrni->pScreen, &portPriv->clip);
        UpdateCurrentTime();
        portPriv->videoStatus = FREE_TIMER;
        portPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
    }

    return Success;
}

/*----------------------------------------------------------------------------
 * LXInitOffscreenImages
 *
 * Description	:This function sets up the offscreen memory management.It fills 
 *				 in the XF86OffscreenImagePtr structure with functions to handle
 *				 offscreen memory operations. 	
 *
 * Parameters.
 *		pScrn	:Screen handler pointer having screen information.
 * 
 * Returns		: None
 *
 * Comments		:None
 *
*----------------------------------------------------------------------------
*/
static void
LXInitOffscreenImages(ScreenPtr pScrn)
{
    XF86OffscreenImagePtr offscreenImages;

    DBLOG(1, "LXInitOffscreenImages()\n");

    /* need to free this someplace */
    if (!(offscreenImages = xalloc(sizeof(XF86OffscreenImageRec))))
        return;

    offscreenImages[0].image = &Images[0];
    offscreenImages[0].flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    offscreenImages[0].alloc_surface = LXAllocateSurface;
    offscreenImages[0].free_surface = LXFreeSurface;
    offscreenImages[0].display = LXDisplaySurface;
    offscreenImages[0].stop = LXStopSurface;
    offscreenImages[0].setAttribute = LXSetSurfaceAttribute;
    offscreenImages[0].getAttribute = LXGetSurfaceAttribute;
    offscreenImages[0].max_width = 1024;
    offscreenImages[0].max_height = 1024;
    offscreenImages[0].num_attributes = NUM_ATTRIBUTES;
    offscreenImages[0].attributes = Attributes;

    xf86XVRegisterOffscreenImages(pScrn, offscreenImages, 1);
}

#endif /* !XvExtension */
#endif /* !AMD_V4L2_VIDEO */
