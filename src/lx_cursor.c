/* Copyright (c) 2003-2008 Advanced Micro Devices, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "geode.h"

Bool
LXCursorInit(ScreenPtr pScrn)
{
    return xf86_cursors_init(pScrn, 32, 32,
	HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
	HARDWARE_CURSOR_INVERT_MASK |
	HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
	HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32);
}

static int
_getrow(unsigned char *src, int stride, int x, int y)
{
    x = ((x & ~31) << 1) | (x & 31);
    src += y * stride;
    return (src[x >> 3] >> (x & 7)) & 1;
}

static int
_getmask(unsigned char *src, int stride, int x, int y)
{
    x = ((x & ~31) << 1) | (1 << 5) | (x & 31);
    src += y * stride;
    return (src[x >> 3] >> (x & 7)) & 1;
}

void
LXLoadCursorImage(ScrnInfoPtr pScrni, unsigned char *src)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long andMask[32], xorMask[32];
    int y, x;

    for (y = 0; y < 32; y++) {
	andMask[y] = (src) ? 0 : ~0;
	xorMask[y] = 0;
    }

    if (src != NULL) {
	for (y = 0; y < 32; y++) {
	    for (x = 0; x < 32; x++) {
		xorMask[y] |= _getrow(src, 8, x, y) << (31 - x);
		andMask[y] |= _getmask(src, 8, x, y) << (31 - x);
	    }
	}
    }

    vg_set_mono_cursor_shape32(pGeode->CursorStartOffset, &andMask[0],
	&xorMask[0], 32, 32);
}
