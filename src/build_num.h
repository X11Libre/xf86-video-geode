#ifndef BUILD_NUM_H
#define BUILD_NUM_H

/* <LIC_AMD_STD>
 * Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
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
 * </LIC_AMD_STD>  */
/* <CTL_AMD_STD>
 * </CTL_AMD_STD>  */
/* <DOC_AMD_STD>
 * </DOC_AMD_STD>  */

#ifdef AMD_BUILDNUM_DEFINED
#error "Caution:  You have muliple version files in your path.  Please correct this."
#endif

#define AMD_BUILDNUM_DEFINED

/* Define the following for your driver */

#define _NAME "AMD Linux Xorg LX/GX display driver"
#define _MAJOR 04
#define _MINOR 00

/* This defines the current buildlevel and revision of the code */
#define _BL 00
#define _BLREV 01

#ifdef _BLREV
#define _BUILDLEVEL _x(_BL)_x(_BLREV)
#else
#define _BUILDLEVEL _x(_BL)
#endif

/* Use this macro to get the version string */

#define _VERSION_NUMBER _x(_MAJOR) "." _x(_MINOR) "." _BUILDLEVEL
#define AMD_VERSION _NAME " " _VERSION_NUMBER

/* Syntatic sugar for use above - you probably don't have to touch this */

#define _x(s) _s(s)
#define _s(s) #s

#endif
