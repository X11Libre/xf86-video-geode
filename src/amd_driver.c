/*
 * Copyright (c) 2006 Avanced Micro Devices, Inc.
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
 * File Contents: This is the main module configures the interfacing 
 *                with the X server. The individual modules will be 
 *                loaded based upon the options selected from the 
 *                XF86Config. This file also has modules for finding 
 *                supported modes, turning on the modes based on options.
 *
 * Project:       Amd Xfree Frame buffer device driver.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Includes that are used by all drivers */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86cmap.h"

#include "amd.h"
#include "build_num.h"

#define RC_MAX_DEPTH 24

#include "fb.h"

/* Machine independent stuff */
#include "mipointer.h"
#include "mibank.h"
#include "micmap.h"
/* All drivers implementing backing store need this */
#include "mibstore.h"
#include "vgaHW.h"
#include "vbe.h"

/* Check for some extensions */
#ifdef XFreeXDGA
#define _XF86_DGA_SERVER_
#include <X11/extensions/xf86dgastr.h>
#endif /* XFreeXDGA */

#ifdef DPMSExtension
#include "globals.h"
#include "opaque.h"
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#endif /* DPMSExtension */

/* A few things all drivers should have */
#define AMD_NAME            "AMD"
#define AMD_DRIVER_NAME     "amd"
#define _id(n,m)			n##m
#define _cat(n,m) 			_id(n,m)
#define AMD_VERSION_CURRENT ((_cat(0x,_MAJOR) << 24) | \
							(_cat(0x,_MINOR) << 16) | (_cat(0x,_BL) << 8) \
							| _cat(0x,_BLREV))

/* Forward definitions */
static const OptionInfoRec *AmdAvailableOptions(int chipid, int busid);
static void AmdIdentify(int);
static Bool AmdProbe(DriverPtr, int);
static int CPUDetected;

/* driver record contains the functions needed by the server after loading
 * the driver module.
 */
_X_EXPORT DriverRec AMD = {
    AMD_VERSION_CURRENT,
    AMD_DRIVER_NAME,
    AmdIdentify,
    AmdProbe,
    AmdAvailableOptions,
    NULL,
    0
};

/* Advanced Micro Devices Chip Models */
typedef struct _DEVICE_MODEL
{
    int DeviceId;
    int Model;
}
DeviceModel;

DeviceModel ChipModel[] = {
#ifdef HAVE_LX
    {PCI_CHIP_GEODELX, LX},
#endif
#ifdef HAVE_GX
    {PCI_CHIP_REDCLOUD, GX2},
#endif
    {-1, 0}
};

/* Supported chipsets */
SymTabRec GeodeChipsets[] = {
#ifdef HAVE_LX
    {PCI_CHIP_GEODELX, "GeodeLX"},
#endif
#ifdef HAVE_GX
    {PCI_CHIP_REDCLOUD, "REDCLOUD"},
#endif
    {-1, NULL}
};

PciChipsets GeodePCIchipsets[] = {
#ifdef HAVE_LX
    {PCI_CHIP_GEODELX, PCI_CHIP_GEODELX, RES_SHARED_VGA},
#endif
#ifdef HAVE_GX
    {PCI_CHIP_REDCLOUD, PCI_CHIP_REDCLOUD, RES_SHARED_VGA},
#endif
    {-1, -1, RES_UNDEFINED},
};

#ifdef HAVE_LX
void LXSetupChipsetFPtr(ScrnInfoPtr pScrni);

OptionInfoRec LX_GeodeOptions[] = {
    {LX_OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_NOCOMPRESSION, "NoCompression", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_ACCEL_METHOD, "AccelMethod", OPTV_STRING, {0}, FALSE},
    {LX_OPTION_TV_SUPPORT, "TV", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_TV_OUTPUT, "TV_Output", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_TV_OVERSCAN, "TVOverscan", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_ROTATE, "Rotate", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_NOPANEL, "NoPanel", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_COLOR_KEY, "ColorKey", OPTV_INTEGER, {0}, FALSE},
    {LX_OPTION_EXA_SCRATCH_BFRSZ, "ExaScratch", OPTV_INTEGER, {0}, FALSE},
    {LX_OPTION_FBSIZE, "FBSize", OPTV_INTEGER, {0}, FALSE },
    {LX_OPTION_PANEL_GEOMETRY, "PanelGeometry", OPTV_STRING, {0}, FALSE },
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

#endif

#ifdef HAVE_GX
void GXSetupChipsetFPtr(ScrnInfoPtr pScrni);

OptionInfoRec GX_GeodeOptions[] = {
    {GX_OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {GX_OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {GX_OPTION_NOCOMPRESSION, "NoCompression", OPTV_BOOLEAN, {0}, FALSE},
    {GX_OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN, {0}, FALSE},
    {GX_OPTION_ACCEL_METHOD, "AccelMethod", OPTV_STRING, {0}, FALSE},
    {GX_OPTION_TV_SUPPORT, "TV", OPTV_ANYSTR, {0}, FALSE},
    {GX_OPTION_TV_OUTPUT, "TV_Output", OPTV_ANYSTR, {0}, FALSE},
    {GX_OPTION_TV_OVERSCAN, "TVOverscan", OPTV_ANYSTR, {0}, FALSE},
    {GX_OPTION_ROTATE, "Rotate", OPTV_ANYSTR, {0}, FALSE},
    {GX_OPTION_NOPANEL, "NoPanel", OPTV_BOOLEAN, {0}, FALSE},
    {GX_OPTION_COLOR_KEY, "ColorKey", OPTV_INTEGER, {0}, FALSE},
    {GX_OPTION_OSM_IMG_BUFS, "OSMImageBuffers", OPTV_INTEGER, {0}, FALSE},
    {GX_OPTION_OSM_CLR_BUFS, "OSMColorExpBuffers", OPTV_INTEGER, {0}, FALSE},
    {GX_OPTION_FBSIZE, "FBSize", OPTV_INTEGER, {0}, FALSE },
    {GX_OPTION_PANEL_GEOMETRY, "PanelGeometry", OPTV_STRING, {0}, FALSE },
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};
#endif

OptionInfoRec no_GeodeOptions[] = {
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

/* List of symbols from other modules that this module references.The purpose
* is that to avoid unresolved symbol warnings
*/
const char *amdVgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWSave",
    "vgaHWRestore",
    "vgaHWProtect",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    NULL
};

const char *amdVbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
    "vbeFree",
    NULL
};

const char *amdInt10Symbols[] = {
    "xf86ExecX86int10",
    "xf86InitInt10",
    "xf86Int10AllocPages",
    "xf86Int10Addr",
    NULL
};

const char *amdFbSymbols[] = {
    "fbScreenInit",
    "fbPictureInit",
    NULL
};

const char *amdXaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAAInit",
    "XAAScreenIndex",
    NULL
};

const char *amdExaSymbols[] = {
    "exaGetVersion",
    "exaDriverInit",
    "exaDriverFini",
    "exaOffscreenAlloc",
    "exaOffscreenFree",
    NULL
};

const char *amdRamdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

#ifdef XFree86LOADER

/* Module loader interface */

static MODULESETUPPROTO(AmdSetup);

static XF86ModuleVersionInfo AmdVersionRec = {
    "amd",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    GET_MODULE_MAJOR_VERSION(AMD_VERSION_CURRENT),
    GET_MODULE_MINOR_VERSION(AMD_VERSION_CURRENT),
    (GET_MODULE_PATCHLEVEL(AMD_VERSION_CURRENT) >> 8) * 100 +
        (GET_MODULE_PATCHLEVEL(AMD_VERSION_CURRENT) & 0xff),
    ABI_CLASS_VIDEODRV,                /* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

/*
 * This data is accessed by the loader.  The name must be the module name
 * followed by "ModuleInit".
 */
_X_EXPORT XF86ModuleData amdModuleData = { &AmdVersionRec, AmdSetup, NULL };

/*-------------------------------------------------------------------------
 * AmdSetup.
 *
 * Description	:This function sets up the driver in X list and load the
 *               module symbols through xf86loader routines..
 *
 * Parameters.
 *    Module	:Pointer to the geode  module
 *    options	:Driver module options.
 *    ErrorMajor:Major no
 *    ErrorMinor:Minor no.
 *
 * Returns		:NULL on success
 *
 * Comments     :Module setup is done by this function
 *
 *-------------------------------------------------------------------------
*/
static pointer
AmdSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised) {
        Initialised = TRUE;
        xf86AddDriver(&AMD, Module, 0);
        /* Tell the loader about symbols from other modules that this
         * module might refer to.
         */
        LoaderRefSymLists(amdVgahwSymbols, amdVbeSymbols,
            amdFbSymbols, amdXaaSymbols,
            amdInt10Symbols, amdRamdacSymbols, NULL);
        return (pointer) TRUE;
    }

    /*The return value must be non-NULL on success */
    if (ErrorMajor)
        *ErrorMajor = LDR_ONCEONLY;
    return NULL;
}
#endif /*End of XFree86Loader */

/*-------------------------------------------------------------------------
 * AmdIdentify.
 *
 * Description  :	This function identify an Amdfamily version.
 *
 *
 * Parameters.
 *    flags		:	flags may be used in PreInit*
 *
 * Returns		: 	none
 *
 * Comments     : 	none
 *
*------------------------------------------------------------------------
*/
static void
AmdIdentify(int flags)
{
    xf86PrintChipsets(AMD_NAME, AMD_VERSION " for chipsets ", GeodeChipsets);
}

/*----------------------------------------------------------------------------
 * AmdAvailableOptions.
 *
 * Description	:This function returns the geodeoptions set geodeoption
 *
 * Parameters.
 *    chipid	:This will identify the chipset.
 *    busid     :This will identify the PCI busid
 *
 * Returns		:ptr to GeodeOptions.
 *
 * Comments     :none
 *
*----------------------------------------------------------------------------
*/
static const OptionInfoRec *
AmdAvailableOptions(int chipid, int busid)
{
    switch (chipid) {
#ifdef HAVE_LX
    case PCI_CHIP_GEODELX:
        return LX_GeodeOptions;
#endif
#ifdef HAVE_GX
    case PCI_CHIP_REDCLOUD:
        return GX_GeodeOptions;
#endif
    }
    return no_GeodeOptions;
}

/*----------------------------------------------------------------------------
 * AmdProbe.
 *
 * Description	:This is to find that hardware is claimed by another
 *		 driver if not claim the slot & allocate ScreenInfoRec.
 *
 * Parameters.
 *     drv	:a pointer to the geode driver
 *     flags    :flags may passed to check the config and probe detect
 * 												
 * Returns	:TRUE on success and FALSE on failure.
 *
 * Comments     :This should ne minimal probe and it should under no
 *               circumstances change the state of the hardware.Don't do
 *               any intiallizations other than the required
 *               ScreenInforec.
*----------------------------------------------------------------------------
*/

static Bool
AmdProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections = NULL;
    int *usedChips = NULL;
    int i;
    void (*drvr_setup) (ScrnInfoPtr pScrni) = NULL;

    DEBUGMSG(1, (0, X_INFO, "AmdProbe: Probing for supported devices!\n"));
    /*
     * * Find the config file Device sections that match this
     * * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(AMD_NAME, &devSections)) <= 0) {
        DEBUGMSG(1, (0, X_INFO, "AmdProbe: failed 1!\n"));
        return FALSE;
    }
    DEBUGMSG(1, (0, X_INFO, "AmdProbe: Before MatchPciInstances!\n"));
    /* PCI BUS */
    if (xf86GetPciVideoInfo()) {
        numUsed = xf86MatchPciInstances(AMD_NAME, PCI_VENDOR_ID_NS,
            GeodeChipsets, GeodePCIchipsets,
            devSections, numDevSections, drv, &usedChips);

        if (numUsed <= 0)
            numUsed = xf86MatchPciInstances(AMD_NAME, PCI_VENDOR_ID_AMD,
                GeodeChipsets, GeodePCIchipsets,
                devSections, numDevSections, drv, &usedChips);

        DEBUGMSG(1, (0, X_INFO, "AmdProbe: MatchPCI (%d)!\n", numUsed));

        if (numUsed > 0) {
            if (flags & PROBE_DETECT)
                foundScreen = TRUE;
            else {
                /* Durango only supports one instance, */
                /* so take the first one */
                for (i = 0; i < numUsed; i++) {
                    /* Allocate a ScrnInfoRec  */
                    ScrnInfoPtr pScrni = xf86AllocateScreen(drv, 0);

                    EntityInfoPtr pEnt = xf86GetEntityInfo(usedChips[i]);
                    PciChipsets *p_id;

                    for (p_id = GeodePCIchipsets; p_id->numChipset != -1;
                        p_id++) {
                        if (pEnt->chipset == p_id->numChipset) {
                            switch (pEnt->chipset) {
#ifdef HAVE_LX
                            case PCI_CHIP_GEODELX:
                                CPUDetected = LX;
                                drvr_setup = &LXSetupChipsetFPtr;
                                break;
#endif
#ifdef HAVE_GX
                            case PCI_CHIP_REDCLOUD:
                                CPUDetected = GX2;
                                drvr_setup = &GXSetupChipsetFPtr;
                                break;
#endif
                            default:
				break;
                            }
                            break;
                        }
                    }
                    xfree(pEnt);
                    if (drvr_setup == NULL)
                    	return FALSE;

                    DEBUGMSG(1, (0, X_INFO, "AmdProbe: CPUDetected %d!\n",
                            CPUDetected));

                    pScrni->driverName = AMD_DRIVER_NAME;
                    pScrni->name = AMD_NAME;
                    pScrni->Probe = AmdProbe;
                    drvr_setup(pScrni);

                    foundScreen = TRUE;
                    xf86ConfigActivePciEntity(pScrni, usedChips[i],
                        GeodePCIchipsets, NULL, NULL, NULL, NULL, NULL);
                }
            }
        }
    }

    if (usedChips)
        xfree(usedChips);
    if (devSections)
        xfree(devSections);
    DEBUGMSG(1, (0, X_INFO, "AmdProbe: result (%d)!\n", foundScreen));
    return foundScreen;
}
