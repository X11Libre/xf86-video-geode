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
 * File Contents: This is the main module configures the interfacing
 *                with the X server. The individual modules will be
 *                loaded based upon the options selected from the
 *                XF86Config. This file also has modules for finding
 *                supported modes, turning on the modes based on options.
 *
 * Project:       Amd Xfree Frame buffer device driver.
 *
 */
#include "config.h"

/* Includes that are used by all drivers */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Pci.h"
#include "xf86cmap.h"

#include "geode.h"

#define RC_MAX_DEPTH 24

#include "fb.h"

/* Machine independent stuff */
#include "mipointer.h"
#include "micmap.h"
#include "vgaHW.h"
#include "vbe.h"

#ifdef DPMSExtension
#include "globals.h"
#include <X11/extensions/dpmsconst.h>

#endif                          /* DPMSExtension */

/* A few things all drivers should have */
#define GEODE_NAME        "GEODE"
#define GEODE_DRIVER_NAME "geode"
#define GEODE_VERSION       4000
#define GEODE_VERSION_MAJOR PACKAGE_VERSION_MAJOR
#define GEODE_VERSION_MINOR PACKAGE_VERSION_MINOR
#define GEODE_VERSION_PATCH PACKAGE_VERSION_PATCHLEVEL

/* Forward definitions */
static const OptionInfoRec *AmdAvailableOptions(int chipid, int busid);
static void AmdIdentify(int);

static Bool AmdPciProbe(DriverPtr, int, struct pci_device *, intptr_t);

static const struct pci_id_match amdDeviceMatch[] = {
    {PCI_VENDOR_ID_NS, PCI_CHIP_GEODEGX, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0,
     0},
    {PCI_VENDOR_ID_AMD, PCI_CHIP_GEODELX, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0,
     0},
    {0, 0, 0}
};

/* driver record contains the functions needed by the server after loading
 * the driver module.
 */
_X_EXPORT DriverRec AMD = {
    GEODE_VERSION,
    "amd",
    AmdIdentify,
    NULL,
    AmdAvailableOptions,
    NULL,
    0,
    NULL,
    amdDeviceMatch,
    AmdPciProbe
};

_X_EXPORT DriverRec GEODE = {
    GEODE_VERSION,
    "geode",
    AmdIdentify,
    NULL,
    AmdAvailableOptions,
    NULL,
    0,
    NULL,
    amdDeviceMatch,
    AmdPciProbe
};

/* Advanced Micro Devices Chip Models */
typedef struct _DEVICE_MODEL {
    int DeviceId;
    int Model;
} DeviceModel;

DeviceModel ChipModel[] = {
#ifdef HAVE_LX
    {PCI_CHIP_GEODELX, LX},
#endif
#ifdef HAVE_GX
    {PCI_CHIP_GEODEGX, GX},
#endif
    {-1, 0}
};

/* Supported chipsets */
SymTabRec GeodeChipsets[] = {
#ifdef HAVE_LX
    {PCI_CHIP_GEODELX, "Geode LX"},
#endif
#ifdef HAVE_GX
    {PCI_CHIP_GEODEGX, "Geode GX"},
#endif
    {-1, NULL}
};

PciChipsets GeodePCIchipsets[] = {
#ifdef HAVE_LX
    {PCI_CHIP_GEODELX, PCI_CHIP_GEODELX, RES_SHARED_VGA},
#endif
#ifdef HAVE_GX
    {PCI_CHIP_GEODEGX, PCI_CHIP_GEODEGX, RES_SHARED_VGA},
#endif
    {-1, -1, RES_UNDEFINED},
};

#ifdef HAVE_LX

OptionInfoRec LX_GeodeOptions[] = {
    {LX_OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_NOCOMPRESSION, "NoCompression", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_TV_SUPPORT, "TV", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_TV_OUTPUT, "TV_Output", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_TV_OVERSCAN, "TVOverscan", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_ROTATE, "Rotate", OPTV_ANYSTR, {0}, FALSE},
    {LX_OPTION_NOPANEL, "NoPanel", OPTV_BOOLEAN, {0}, FALSE},
    {LX_OPTION_EXA_SCRATCH_BFRSZ, "ExaScratch", OPTV_INTEGER, {0}, FALSE},
    {LX_OPTION_FBSIZE, "FBSize", OPTV_INTEGER, {0}, FALSE},
    {LX_OPTION_PANEL_MODE, "PanelMode", OPTV_STRING, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

#endif

#ifdef HAVE_GX

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
    {GX_OPTION_OSM_IMG_BUFS, "OSMImageBuffers", OPTV_INTEGER, {0}, FALSE},
    {GX_OPTION_OSM_CLR_BUFS, "OSMColorExpBuffers", OPTV_INTEGER, {0}, FALSE},
    {GX_OPTION_FBSIZE, "FBSize", OPTV_INTEGER, {0}, FALSE},
    {GX_OPTION_PANEL_GEOMETRY, "PanelGeometry", OPTV_STRING, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};
#endif

OptionInfoRec no_GeodeOptions[] = {
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

/* Module loader interface */

static MODULESETUPPROTO(AmdSetup);

static XF86ModuleVersionInfo AmdVersionRec = {
    "amd",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    GEODE_VERSION_MAJOR, GEODE_VERSION_MINOR, GEODE_VERSION_PATCH,
    ABI_CLASS_VIDEODRV,         /* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

static XF86ModuleVersionInfo GeodeVersionRec = {
    "geode",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    GEODE_VERSION_MAJOR, GEODE_VERSION_MINOR, GEODE_VERSION_PATCH,
    ABI_CLASS_VIDEODRV,         /* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

static pointer
GeodeSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool init = FALSE;
    int flag = 0;

    flag = HaveDriverFuncs;
    if (init) {
        *ErrorMajor = LDR_ONCEONLY;
        return (pointer) NULL;
    }

    init = TRUE;
    xf86AddDriver(&GEODE, Module, flag);

    return (pointer) TRUE;
}

static pointer
AmdSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised) {
        Initialised = TRUE;
        xf86AddDriver(&AMD, Module,
                      HaveDriverFuncs
            );

        return (pointer) TRUE;
    }

    /*The return value must be non-NULL on success */
    if (ErrorMajor)
        *ErrorMajor = LDR_ONCEONLY;
    return NULL;
}

_X_EXPORT XF86ModuleData amdModuleData = { &AmdVersionRec, AmdSetup, NULL };
_X_EXPORT XF86ModuleData geodeModuleData =
    { &GeodeVersionRec, GeodeSetup, NULL };

/*-------------------------------------------------------------------------
 * AmdIdentify.
 *
 * Description  :	This function identify an Amdfamily version.
 *
 *
 * Parameters.
 *    flags		:	flags may be used in PreInit*
 *
 * Returns		:	none
 *
 * Comments     :	none
 *
*------------------------------------------------------------------------
*/
static void
AmdIdentify(int flags)
{
    xf86PrintChipsets(GEODE_NAME, "Driver for AMD Geode Chipsets",
                      GeodeChipsets);
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
    case PCI_CHIP_GEODEGX:
        return GX_GeodeOptions;
#endif
    }
    return no_GeodeOptions;
}

static Bool
AmdPciProbe(DriverPtr driver,
            int entity_num, struct pci_device *device, intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;

    ErrorF("AmdPciProbe: Probing for supported devices!\n");

    scrn = xf86ConfigPciEntity(scrn, 0, entity_num, GeodePCIchipsets,
                               NULL, NULL, NULL, NULL, NULL);

    if (scrn != NULL) {
        scrn->driverName = GEODE_DRIVER_NAME;
        scrn->driverVersion = GEODE_VERSION;
        scrn->name = GEODE_NAME;
        scrn->Probe = NULL;

        switch (device->device_id) {
#ifdef HAVE_LX
        case PCI_CHIP_GEODELX:
            LXSetupChipsetFPtr(scrn);
            break;
#endif
#ifdef HAVE_GX
        case PCI_CHIP_GEODEGX:
            GXSetupChipsetFPtr(scrn);
            break;
#endif
        default:
            ErrorF("AmdPciProbe: unknown device ID\n");
            return FALSE;
        }
    }
    return scrn != NULL;
}
