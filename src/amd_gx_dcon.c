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
#include <xf86_ansic.h>
#include <xf86_libc.h>
#include <xf86Resources.h>
#include <xf86_ansic.h>
#include <compiler.h>

#include "amd.h"

/* OLPC board revision */

#define REV_TESTA 0
#define REV_TESTB 1

/* Values in the embedded controller */

#define TESTA_REVISION 0x09
#define EC_VER_CMD 0x09

/* This is a special color map used for video in the GX engine */
/* on a rev-B DCON, this should be adjusted */

extern unsigned long gfx_gamma_ram_redcloud[];

/* Get the current board revision in a roundabout way by querying the
   embedded controller 
*/

static int eccmd(ScrnInfoPtr pScrni, unsigned char cmd) {

	unsigned char ret;
	int i;

	ret = inb(0x6c);

	if (ret & 1)
		ret = inb(0x68);
	
	/* Write the command */
	outb(0x6C, cmd);
	
	/* Wait for the 2 response */
	for(i = 0; i < 1000; i++) {
		ret = inb(0x6C);
		if ((ret & 3) == 1)
			break;
	}
	
	if (i == 100) {
		xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "Error waiting for the EC command (%x)\n", ret);
		ret = -1;		
		goto eread;
	}

	/* get the response */
	ret = inb(0x68);

 eread:	
	/* Clear the "ownership flag" */
	outb(0x6C, 0xFF);
	return ret;
}

static int boardrev(ScrnInfoPtr pScrni) {
	int i, ret;

	ret = eccmd(pScrni, 0x09);

	if (ret == -1)
		return -1;

	return ret == TESTA_REVISION ? REV_TESTA : REV_TESTB;
}

#define RTC_BASE_PORT 0x70
#define RTC_PORT(x)     (RTC_BASE_PORT + (x))

static inline char cmos_read(unsigned char addr) {
	outb(RTC_PORT(0), addr);
	return inb(RTC_PORT(1));
}

static inline void cmos_write(unsigned char val, unsigned char addr) {
	outb(RTC_PORT(0), addr);
	outb(RTC_PORT(1), val);
}
	
static int dcon_avail(void) {
	return cmos_read(440 / 8) & 1;
}

void gx_dcon_init(ScrnInfoPtr pScrni) {

	int rev = boardrev(pScrni);
	int i;

	if (rev == -1) {
		xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "This is not an OLPC board\n");
		return;  
	}
	if (dcon_avail() == 0) {
		xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "No DCON is present\n");
		return;
	}

	xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "OLPC board revision %s\n", rev == REV_TESTB ? "testB" : "testA");
	xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "DCON detected.\n");

	/* FIXME:  Panel setup should go here */
	/* FIXME:  Mode setup should go here */
	/* FIXME:  Controller setup should go here */
 
	/* Update the Xv map on a rev-b board */

	if (rev == REV_TESTB) {
		for(i = 0; i < 256; i++) {
			unsigned char r, g, b;
			r = (gfx_gamma_ram_redcloud[i] >> 16) & 0xFF;
			g = (gfx_gamma_ram_redcloud[i] >> 8) & 0xFF;
			b = gfx_gamma_ram_redcloud[i] & 0xFF;

			gfx_gamma_ram_redcloud[i] = ((r >> 2) << 16) | ((g >> 1) << 8) | (b >> 2);
		}
	}
}
