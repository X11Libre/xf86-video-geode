/* Copyright (c) 2006 Advanced Micro Devices, Inc.
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

/* Includes that are used by all drivers */
#include <xf86.h>
#include <xf86_OSproc.h>
#include <xf86Resources.h>
#include <compiler.h>

#include "geode.h"
#include <unistd.h>

static Bool
dcon_present(void)
{
    return access("/sys/devices/platform/dcon", F_OK) == 0;
}

Bool
dcon_init(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (!dcon_present()) {
	xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "No DCON is present\n");
	return FALSE;
    }

    /* TODO: Print board revision once sysfs exports it. */
    xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "DCON detected.\n");

    /* Panel size setup */
    pGeode->PanelX = DCON_DEFAULT_XRES;
    pGeode->PanelY = DCON_DEFAULT_YRES;

    /* FIXME:  Mode setup should go here */
    /* FIXME:  Controller setup should go here */
    return TRUE;
}
