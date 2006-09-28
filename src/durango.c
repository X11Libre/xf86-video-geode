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
 * This is the main file used to add Durango graphics support to a software 
 * project.  The main reason to have a single file include the other files
 * is that it centralizes the location of the compiler options.  This file
 * should be tuned for a specific implementation, and then modified as needed
 * for new Durango releases.  The releases.txt file indicates any updates to
 * this main file, such as a new definition for a new hardware platform. 
 *
 * In other words, this file should be copied from the Durango source files
 * once when a software project starts, and then maintained as necessary.  
 * It should not be recopied with new versions of Durango unless the 
 * developer is willing to tune the file again for the specific project.
 *  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* COMPILER OPTIONS
 * These compiler options specify how the Durango routines are compiled 
 * for the different hardware platforms.  For best performance, a driver 
 * would build for a specific platform.  The "dynamic" switches are set 
 * by diagnostic applications such as Darwin that will run on a variety
 * of platforms and use the appropriate code at runtime.  Each component
 * may be separately dynamic, so that a driver has the option of being 
 * tuned for a specific 2D accelerator, but will still run with a variety
 * of chipsets. 
 */

#define GFX_DISPLAY_DYNAMIC			0       /* runtime selection */
#define GFX_DISPLAY_GU1				0       /* 1st generation display controller */
#define GFX_DISPLAY_GU2				1       /* 2nd generation display controller */
#define GFX_DISPLAY_GU3				0       /* 3nd generation display controller */

#define GFX_INIT_DYNAMIC            0  /* runtime selection */
#define GFX_INIT_GU1                0  /* Geode family      */
#define GFX_INIT_GU2                1  /* Redcloud          */
#define GFX_INIT_GU3                0  /* Castle            */

#define GFX_MSR_DYNAMIC             1  /* runtime selection */
#define GFX_MSR_REDCLOUD            1  /* Redcloud          */

#define GFX_2DACCEL_DYNAMIC			0       /* runtime selection                                */
#define GFX_2DACCEL_GU1				0       /* 1st generation 2D accelerator    */
#define GFX_2DACCEL_GU2				1       /* 2nd generation 2D accelerator    */

#define GFX_VIDEO_DYNAMIC			0       /* runtime selection        */
#define GFX_VIDEO_CS5530			0       /* support for CS5530       */
#define GFX_VIDEO_SC1200			0       /* support for SC1200       */
#define GFX_VIDEO_REDCLOUD			1       /* support for Redcloud */
#define GFX_VIDEO_CASTLE			0       /* support for Castle       */

#define GFX_VIP_DYNAMIC				0       /* runtime selection        */
#define GFX_VIP_SC1200				0       /* support for SC1200       */

#define GFX_DECODER_DYNAMIC			0       /* runtime selection                        */
#define GFX_DECODER_SAA7114			0       /* Philips SAA7114 decoder          */

#define GFX_TV_DYNAMIC				0       /* runtime selection                        */
#define GFX_TV_FS451				0       /* Focus Enhancements FS450         */
#define GFX_TV_SC1200				0       /* SC1200 integrated TV encoder */

#define GFX_I2C_DYNAMIC				0       /* runtime selection                        */
#define GFX_I2C_ACCESS				0       /* support for ACCESS.BUS           */
#define GFX_I2C_GPIO				0       /* support for CS5530 GPIOs         */

#define GFX_VGA_DYNAMIC				0       /* runtime selection                        */
#define GFX_VGA_GU1					0       /* 1st generation graphics unit */

#define FB4MB						1       /* Set to use 4Mb video ram for 
                                                                 * Pyramid                                          */

#define GFX_NO_IO_IN_WAIT_MACROS    1  /* Set to remove I/O accesses in GP 
                                        * bit testing                                  */

/* ROUTINES TO READ VALUES
 * These are routines used by Darwin or other diagnostics to read the 
 * current state of the hardware.  Display drivers or embedded applications 
 * can reduce the size of the Durango code by not including these routines. 
 */
#define GFX_READ_ROUTINES			1       /* add routines to read values      */

/* HEADER FILE FOR DURANGO ROUTINE DEFINITIONS
 * Needed since some of the Durango routines call other Durango routines.
 * Also defines the size of chipset array (GFX_CSPTR_SIZE).
 */
#include "gfx_rtns.h"                  /* routine definitions                   */
#include "gfx_priv.h"

/* VARIABLES USED FOR RUNTIME SELECTION
 * If part of the graphics subsystem is declared as dynamic, then the 
 * following variables are used to specify which platform has been detected.
 * The variables are set in the "gfx_detect_cpu" routine.  The values should 
 * be bit flags to allow masks to be used to check for multiple platforms.
 */

#if GFX_DISPLAY_DYNAMIC
int gfx_display_type = 0;
#endif

#if GFX_INIT_DYNAMIC
int gfx_init_type = 0;
#endif

#if GFX_MSR_DYNAMIC
int gfx_msr_type = 0;
#endif

#if GFX_2DACCEL_DYNAMIC
int gfx_2daccel_type = 0;
#endif

#if GFX_VIDEO_DYNAMIC
int gfx_video_type = 0;
#endif

#if GFX_VIP_DYNAMIC
int gfx_vip_type = 0;
#endif

#if GFX_DECODER_DYNAMIC
int gfx_decoder_type = 0;
#endif

#if GFX_TV_DYNAMIC
int gfx_tv_type = 0;
#endif

#if GFX_I2C_DYNAMIC
int gfx_i2c_type = 0;
#endif

#if GFX_VGA_DYNAMIC
int gfx_vga_type = 0;
#endif

/* DEFINE POINTERS TO MEMORY MAPPED REGIONS
 * These pointers are used by the Durango routines to access the hardware. 
 * The variables must be set by the project's initialization code after
 * mapping the regions in the appropriate manner. 
 */

/* DEFINE VIRTUAL ADDRESSES */
/* Note: These addresses define the starting base expected by all    */
/*       Durango offsets.  Under an OS that requires these pointers  */
/*       to be mapped to linear addresses (i.e Windows), it may not  */
/*       be possible to keep these base offsets.  In these cases,    */
/*       the addresses are modified to point to the beginning of the */
/*       relevant memory region and the access macros are adjusted   */
/*       to subtract the offset from the default base.  For example, */
/*       the register pointer could be moved to be 0x40008000, while */
/*       the WRITE_REG* macros are modified to subtract 0x8000 from  */
/*       the offset.                                                 */

unsigned char *gfx_virt_regptr = (unsigned char *)0x40000000;
unsigned char *gfx_virt_fbptr = (unsigned char *)0x40800000;
unsigned char *gfx_virt_vidptr = (unsigned char *)0x40010000;
unsigned char *gfx_virt_vipptr = (unsigned char *)0x40015000;
unsigned char *gfx_virt_spptr = (unsigned char *)0x40000000;
unsigned char *gfx_virt_gpptr = (unsigned char *)0x40000000;

/* DEFINE PHYSICAL ADDRESSES */

unsigned char *gfx_phys_regptr = (unsigned char *)0x40000000;
unsigned char *gfx_phys_fbptr = (unsigned char *)0x40800000;
unsigned char *gfx_phys_vidptr = (unsigned char *)0x40010000;
unsigned char *gfx_phys_vipptr = (unsigned char *)0x40015000;

/* HEADER FILE FOR GRAPHICS REGISTER DEFINITIONS 
 * This contains only constant definitions, so it should be able to be 
 * included in any software project as is.
 */
#include "gfx_regs.h"                  /* graphics register definitions */

/* HEADER FILE FOR REGISTER ACCESS MACROS
 * This file contains the definitions of the WRITE_REG32 and similar macros
 * used by the Durango routines to access the hardware.  The file assumes 
 * that the environment can handle 32-bit pointer access.  If this is not
 * the case, or if there are special requirements, then this header file 
 * should not be included and the project must define the macros itself.
 * (A project may define WRITE_REG32 to call a routine, for example).
 */
#include "gfx_defs.h"                  /* register access macros */
#include <xf86_ansic.h>
#include <compiler.h>

#define INB(port) inb(port)
#define INW(port) inw(port)
#define IND(port) inl(port)
#define OUTB(port,data) outb(port, data)
#define OUTW(port,data) outw(port, data)
#define OUTD(port,data) outl(port, data)

unsigned char gfx_inb(unsigned short port);
unsigned short gfx_inw(unsigned short port);
unsigned long gfx_ind(unsigned short port);
void gfx_outb(unsigned short port, unsigned char data);
void gfx_outw(unsigned short port, unsigned short data);
void gfx_outd(unsigned short port, unsigned long data);

unsigned char
gfx_inb(unsigned short port)
{
    return inb(port);
}

unsigned short
gfx_inw(unsigned short port)
{
    return inw(port);
}

unsigned long
gfx_ind(unsigned short port)
{
    return inl(port);
}

void
gfx_outb(unsigned short port, unsigned char data)
{
    outb(port, data);
}

void
gfx_outw(unsigned short port, unsigned short data)
{
    outw(port, data);
}

void
gfx_outd(unsigned short port, unsigned long data)
{
    outl(port, data);
}

/*-----------------------------------------------------------------
 * gfx_msr_asm_read
 * Read the contents of a 64 bit MSR into address pointers
 *-----------------------------------------------------------------*/

#define gfx_msr_asm_read(msr,adr,high,low)      \
     __asm__ __volatile__(                      \
        " mov $0x0AC1C, %%edx\n"                \
        " mov $0xFC530007, %%eax\n"             \
        " out %%eax,%%dx\n"                     \
        " add $2,%%dl\n"                        \
        " in %%dx, %%ax"                        \
        : "=a" (*(low)), "=d" (*(high))         \
        : "c" (msr | adr))

/*-----------------------------------------------------------------
 * gfx_msr_asm_write
 * Write the contents of address pointers to a MSR.
 *-----------------------------------------------------------------*/

#define gfx_msr_asm_write(msr,adr,high,low) \
  { int d0, d1, d2, d3, d4;        \
  __asm__ __volatile__(            \
    " push %%ebx\n"                \
    " mov $0x0AC1C, %%edx\n"       \
    " mov $0xFC530007, %%eax\n"    \
    " out %%eax,%%dx\n"            \
    " add $2,%%dl\n"               \
    " mov %6, %%ebx\n"             \
    " mov %7, %0\n"                \
    " mov %5, %3\n"                \
    " xor %2, %2\n"                \
    " xor %1, %1\n"                \
    " out %%ax, %%dx\n"            \
    " pop %%ebx\n"                 \
    : "=a"(d0),"=&D"(d1),"=&S"(d2), \
      "=c"(d3),"=d"(d4)  \
    : "1"(msr | adr),"2"(*(high)),"3"(*(low))); \
  }

/* INITIALIZATION ROUTINES 
 * These routines are used during the initialization of the driver to 
 * perform such tasks as detecting the type of CPU and video hardware.  
 * The routines require the use of IO, so the above IO routines need 
 * to be implemented before the initialization routines will work
 * properly.
 */

#include "gfx_init.c"

/* INCLUDE MSR ACCESS ROUTINES */

#include "gfx_msr.c"

/* INCLUDE GRAPHICS ENGINE ROUTINES 
 * These routines are used to program the 2D graphics accelerator.  If
 * the project does not use graphics acceleration (direct frame buffer
 * access only), then this file does not need to be included. 
 */
#include "gfx_rndr.c"                  /* graphics engine routines              */

/* INCLUDE DISPLAY CONTROLLER ROUTINES 
 * These routines are used if the display mode is set directly.  If the 
 * project uses VGA registers to set a display mode, then these files
 * do not need to be included.
 */
#include "gfx_mode.h"                  /* display mode tables                   */
#include "gfx_disp.c"                  /* display controller routines   */

/* INCLUDE VIDEO OVERLAY ROUTINES
 * These routines control the video overlay hardware. 
 */
#include "gfx_vid.c"                   /* video overlay routines                */

/* VIDEO PORT AND VIDEO DECODER ROUTINES
 * These routines rely on the I2C routines.
 */
#include "gfx_vip.c"                   /* video port routines                   */
#include "gfx_dcdr.c"                  /* video decoder routines                */

/* I2C BUS ACCESS ROUTINES
 * These routines are used by the video decoder and possibly an 
 * external TV encoer. 
 */
#include "gfx_i2c.c"                   /* I2C bus access routines               */

/* TV ENCODER ROUTINES
 * This file does not need to be included if the system does not
 * support TV output.
 */
#include "gfx_tv.c"                    /* TV encoder routines                   */

/* VGA ROUTINES
 * This file is used if setting display modes using VGA registers.
 */
#include "gfx_vga.c"                   /* VGA routines                                  */

/* END OF FILE */
