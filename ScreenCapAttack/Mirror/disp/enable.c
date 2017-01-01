/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\**************************************************************************/
#define DBG 1

#include "driver.h"

// The driver function table with all function index/address pairs


static DRVFN gadrvfn[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvNotify,                (PFN) DrvNotify             },
#if 0
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
#endif	
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo             },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvStrokeAndFillPath,     (PFN) DrvStrokeAndFillPath  },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
    {   INDEX_DrvAlphaBlend,            (PFN) DrvAlphaBlend         },
    {   INDEX_DrvTransparentBlt,        (PFN) DrvTransparentBlt     },
    {   INDEX_DrvGradientFill,          (PFN) DrvGradientFill       },
    {   INDEX_DrvPlgBlt,                (PFN) DrvPlgBlt             },
    {   INDEX_DrvStretchBltROP,         (PFN) DrvStretchBltROP      },
#if (NTDDI_VERSION >= NTDDI_VISTA)
    {   INDEX_DrvRenderHint,            (PFN) DrvRenderHint         },
#endif
    {   INDEX_DrvEscape,                (PFN) DrvEscape             }
};

//
// always hook these routines to ensure the mirrored driver
// is called for our surfaces
//

#define flGlobalHooks   HOOK_BITBLT|HOOK_TEXTOUT|HOOK_COPYBITS|HOOK_STROKEPATH|HOOK_LINETO|HOOK_FILLPATH|HOOK_STROKEANDFILLPATH|HOOK_STRETCHBLT|HOOK_ALPHABLEND|HOOK_TRANSPARENTBLT|HOOK_GRADIENTFILL|HOOK_PLGBLT|HOOK_STRETCHBLTROP

// Define the functions you want to hook for 8/16/24/32 pel formats

#define HOOKS_BMF8BPP 0

#define HOOKS_BMF16BPP 0

#define HOOKS_BMF24BPP 0

#define HOOKS_BMF32BPP 0

/******************************Public*Routine******************************\
* DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    DISPDBG((0,"DrvEnableDriver:\n"));

// Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

// DDI version this driver was targeted for is passed back to engine.
// Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
	// DDI_DRIVER_VERSION is now out-dated. See winddi.h
	// DDI_DRIVER_VERSION_NT4 is equivalent to the old DDI_DRIVER_VERSION
        pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvEnablePDEV
*
* DDI function, Enables the Physical Device.
*
* Return Value: device handle to pdev.
*
\**************************************************************************/

DHPDEV
DrvEnablePDEV(
    __in     DEVMODEW   *pDevmode,                // Pointer to DEVMODE
    __in_opt PWSTR       pwszLogAddress,          // Logical address
    __in     ULONG       cPatterns,               // number of patterns
    __in_opt HSURF      *ahsurfPatterns,          // return standard patterns
    __in     ULONG       cjGdiInfo,               // Length of memory pointed to by pGdiInfo
    __out_bcount(cjGdiInfo)  ULONG    *pGdiInfo,  // Pointer to GdiInfo structure
    __in     ULONG       cjDevInfo,               // Length of following PDEVINFO structure
    __out_bcount(cjDevInfo)  DEVINFO  *pDevInfo,  // physical device information structure
    __in_opt HDEV        hdev,                    // HDEV, used for callbacks
    __in_opt PWSTR       pwszDeviceName,          // DeviceName - not used
    __in     HANDLE      hDriver                  // Handle to base driver
    )
{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    PPDEV   ppdev = (PPDEV) NULL;

    DISPDBG((0,"DrvEnablePDEV:\n"));

    // Allocate a physical device structure.

    ppdev = (PPDEV) EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);

    if (ppdev == (PPDEV) NULL)
    {
        RIP("DISP DrvEnablePDEV failed EngAllocMem\n");
        return((DHPDEV) 0);
    }

    // Save the screen handle in the PDEV.

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and devinfo.

    if (!bInitPDEV(ppdev, pDevmode, &GdiInfo, &DevInfo))
    {
        DISPDBG((0,"DISP DrvEnablePDEV failed\n"));
        goto error_free;
    }
    
    // Copy the devinfo into the engine buffer.

    // memcpy(pDevInfo, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));
    if (sizeof(DEVINFO) > cjDevInfo)
    {
        DISPDBG((0,"DISP DrvEnablePDEV failed: insufficient pDevInfo memory\n"));
        goto error_free;
    }
	RtlCopyMemory(pDevInfo, &DevInfo, sizeof(DEVINFO));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.

    //memcpy(pGdiInfo, &GdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));
    if (sizeof(GDIINFO) > cjGdiInfo)
    {
        DISPDBG((0,"DISP DrvEnablePDEV failed: insufficient pDevInfo memory\n"));
        goto error_free;
    }
	RtlCopyMemory(pGdiInfo, &GdiInfo, sizeof(GDIINFO));

    return((DHPDEV) ppdev);

    // Error case for failure.
error_free:
    EngFreeMem(ppdev);
    return((DHPDEV) 0);
}

/******************************Public*Routine******************************\
* DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    DISPDBG((1,"DrvCompletePDEV:\n"));
    ((PPDEV) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV dhpdev)
{
   PPDEV ppdev = (PPDEV) dhpdev;
   
   EngDeletePalette(ppdev->hpalDefault);

   EngFreeMem(dhpdev);
   /*    Add              */
   if(ppdev->hMem) EngUnmapFile(ppdev->hMem);   
}

/******************************Public*Routine******************************\
* DrvEnableSurface
*
* Enable the surface for the device.  Hook the calls this driver supports.
*
* Return: Handle to the surface if successful, 0 for failure.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PPDEV ppdev;
    HSURF hsurf;
    SIZEL sizl;
    ULONG ulBitmapType;
    FLONG flHooks;
    ULONG PerPixel;
    MIRRSURF *mirrsurf;

    // Create engine bitmap around frame buffer.

    DISPDBG((0,"DrvEnableSurface:\n"));

    ppdev = (PPDEV) dhpdev;

    ppdev->ptlOrg.x = 0;
    ppdev->ptlOrg.y = 0;

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (ppdev->ulBitCount == 16)
    {
        ulBitmapType = BMF_16BPP;
        flHooks = HOOKS_BMF16BPP;
		PerPixel = 2;
    }
    else if (ppdev->ulBitCount == 24)
    {
        ulBitmapType = BMF_24BPP;
        flHooks = HOOKS_BMF24BPP;
		PerPixel = 3;
    }
    else
    {
        ulBitmapType = BMF_32BPP;
        flHooks = HOOKS_BMF32BPP;
		PerPixel = 4;
    }
    
    flHooks |= flGlobalHooks;

    hsurf = EngCreateDeviceSurface((DHSURF)ppdev,
                                   sizl,
                                   ulBitmapType);

    if (hsurf == (HSURF) 0)
    {
        RIP("DISP DrvEnableSurface failed EngCreateBitmap\n");
        return(FALSE);
    }
	/*       Add                */
	//==================================
	ppdev->pVideoMemory = EngMapFile(
	L"\\??\\c:\\video.dat",
	ppdev->cxScreen*ppdev->cyScreen*PerPixel,
	&ppdev->hMem);
		
	EngModifySurface(hsurf,ppdev->hdevEng,flHooks,0,
	(DHSURF)ppdev,ppdev->pVideoMemory,ppdev->cxScreen*PerPixel,
	NULL);
	//==================================	
    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, flHooks))
    {
        RIP("DISP DrvEnableSurface failed EngAssociateSurface\n");
        EngDeleteSurface(hsurf);
        return(FALSE);
    }
	
    ppdev->hsurfEng = (HSURF) hsurf;
 
    return(hsurf);
}

/******************************Public*Routine******************************\
* DrvNotify
*
* Receives notification on where the mirror driver is positioned.
* Also gets notified before drawing happens 
*
\**************************************************************************/

VOID DrvNotify(
SURFOBJ *pso,
ULONG iType,
PVOID pvData)
{
    UNREFERENCED_PARAMETER(pso);
    UNREFERENCED_PARAMETER(pvData);

    switch(iType)
    {
        case DN_DEVICE_ORIGIN:
            DISPDBG((0,"DrvNotify: DN_DEVICE_ORIGIN (%d,%d)\n", ((POINTL*)pvData)->x, ((POINTL*)pvData)->y));
            break;
        case DN_DRAWING_BEGIN:
            DISPDBG((0,"DrvNotify: DN_DRAWING_BEGIN\n"));
            break;
    }
}

/******************************Public*Routine******************************\
* DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV) dhpdev;

    DISPDBG((0,"DrvDisableSurface:\n"));

    EngDeleteSurface( ppdev->hsurfEng );
    
    // deallocate MIRRSURF structure.

    EngFreeMem( ppdev->pvTmpBuffer );
}

/******************************Public*Routine******************************\
* DrvCopyBits
*
\**************************************************************************/

BOOL DrvCopyBits(
   OUT SURFOBJ *psoDst,
   IN SURFOBJ *psoSrc,
   IN CLIPOBJ *pco,
   IN XLATEOBJ *pxlo,
   IN RECTL *prclDst,
   IN POINTL *pptlSrc
   )
{

   DISPDBG((0,"DrvCopyBits:(%d,%d,%d,%d)\n", prclDst->bottom, prclDst->left,prclDst->right, prclDst->top));

	//return EngCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);
	return DrvBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc, 
                        NULL, NULL, NULL, 0xCCCC);

}

/******************************Public*Routine******************************\
* DrvBitBlt
*
\**************************************************************************/

BOOL DrvBitBlt(
   IN SURFOBJ *psoDst,
   IN SURFOBJ *psoSrc,
   IN SURFOBJ *psoMask,
   IN CLIPOBJ *pco,
   IN XLATEOBJ *pxlo,
   IN RECTL *prclDst,
   IN POINTL *pptlSrc,
   IN POINTL *pptlMask,
   IN BRUSHOBJ *pbo,
   IN POINTL *pptlBrush,
   IN ROP4 rop4
   )
{

   DISPDBG((0,"DrvBitBlt:(%d,%d,%d,%d)\n", prclDst->bottom, prclDst->left,prclDst->right, prclDst->top));
	return EngBitBlt(psoDst, psoSrc, psoMask, pco, pxlo, prclDst, pptlSrc, pptlMask, pbo, pptlBrush, rop4);

}

BOOL DrvTextOut(
   IN SURFOBJ *psoDst,
   IN STROBJ *pstro,
   IN FONTOBJ *pfo,
   IN CLIPOBJ *pco,
   IN RECTL *prclExtra,
   IN RECTL *prclOpaque,
   IN BRUSHOBJ *pboFore,
   IN BRUSHOBJ *pboOpaque,
   IN POINTL *pptlOrg,
   IN MIX mix
   )
{

   DISPDBG((1,
            "Mirror Driver DrvTextOut: pwstr=%08x\n",
            pstro ? pstro->pwszOrg : (WCHAR*)-1));

   return EngTextOut(psoDst, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);

}

BOOL
DrvStrokePath(SURFOBJ*   pso,
              PATHOBJ*   ppo,
              CLIPOBJ*   pco,
              XFORMOBJ*  pxo,
              BRUSHOBJ*  pbo,
              POINTL*    pptlBrush,
              LINEATTRS* pLineAttrs,
              MIX        mix)
{

   DISPDBG((1,"Mirror Driver DrvStrokePath:\n"));

   return EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pLineAttrs, mix);

}

BOOL DrvLineTo(
SURFOBJ   *pso,
CLIPOBJ   *pco,
BRUSHOBJ  *pbo,
LONG       x1,
LONG       y1,
LONG       x2,
LONG       y2,
RECTL     *prclBounds,
MIX        mix)
{

    DISPDBG((1,"Mirror Driver DrvLineTo: \n"));
	return EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);

}



BOOL DrvFillPath(
 SURFOBJ  *pso,
 PATHOBJ  *ppo,
 CLIPOBJ  *pco,
 BRUSHOBJ *pbo,
 PPOINTL   pptlBrushOrg,
 MIX       mix,
 FLONG     flOptions)
{

    DISPDBG((1,"Mirror Driver DrvFillPath: \n"));  
	return EngFillPath(pso, ppo,pco, pbo, pptlBrushOrg, mix, flOptions);

}

BOOL DrvStrokeAndFillPath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pboStroke,
LINEATTRS* plineattrs,
BRUSHOBJ*  pboFill,
POINTL*    pptlBrushOrg,
MIX        mixFill,
FLONG      flOptions)
{

    DISPDBG((1,"Mirror Driver DrvStrokeAndFillPath: \n"));
	return EngStrokeAndFillPath(pso,ppo,pco,pxo,pboStroke,plineattrs,pboFill,pptlBrushOrg,mixFill,flOptions);
}

BOOL DrvTransparentBlt(
SURFOBJ*    psoDst,
SURFOBJ*    psoSrc,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
RECTL*      prclSrc,
ULONG       iTransColor,
ULONG       ulReserved)
{

    DISPDBG((1,"Mirror Driver DrvTransparentBlt: \n"));
	return EngTransparentBlt(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, iTransColor, ulReserved);

}


BOOL DrvAlphaBlend(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
RECTL*              prclDst,
RECTL*              prclSrc,
BLENDOBJ*           pBlendObj)
{

    DISPDBG((1,"Mirror Driver DrvAlphaBlend: \n"));
	return EngAlphaBlend(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, pBlendObj);

}

BOOL DrvGradientFill(
SURFOBJ*            pso,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
TRIVERTEX*          pVertex,
ULONG               nVertex,
PVOID               pMesh,
ULONG               nMesh,
RECTL*              prclExtents,
POINTL*             pptlDitherOrg,
ULONG               ulMode)
{

    DISPDBG((1,"Mirror Driver DrvGradientFill: \n"));
	return EngGradientFill(pso, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);

}

BOOL DrvStretchBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMsk,
ULONG               iMode)
{

    DISPDBG((1,"Mirror Driver DrvStretchBlt: \n"));
	return EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode);

}

BOOL DrvStretchBltROP(
SURFOBJ         *psoTrg,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlBrushOrg,
RECTL           *prclTrg,
RECTL           *prclSrc,
POINTL          *pptlMask,
ULONG            iMode,
BRUSHOBJ        *pbo,
ROP4            rop4)
{
    DISPDBG((1,"Mirror Driver DrvStretchBltROP: \n"));
	return EngStretchBltROP(psoTrg,psoSrc,psoMask,pco,pxlo,pca,pptlBrushOrg,prclTrg,prclSrc,pptlMask,iMode,pbo,rop4);

}

BOOL DrvPlgBlt(
SURFOBJ         *psoTrg,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMsk,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlBrushOrg,
POINTFIX        *pptfx,
RECTL           *prcl,
POINTL          *pptl,
ULONG            iMode)
{
    DISPDBG((1,"Mirror Driver DrvPlgBlt: \n"));
	return EngPlgBlt(psoTrg,psoSrc,psoMsk,pco,pxlo,pca,pptlBrushOrg,pptfx,prcl,pptl,iMode);

}
#if 0
HBITMAP DrvCreateDeviceBitmap(
   IN DHPDEV dhpdev,
   IN SIZEL sizl,
   IN ULONG iFormat
   )
{
   MIRRSURF *mirrsurf;
   ULONG mirrorsize;
   DHSURF dhsurf;
   ULONG stride;
   HSURF hsurf;

   PPDEV ppdev = (PPDEV) dhpdev;
   
   DISPDBG((1,"CreateDeviceBitmap:\n"));
   
   if (iFormat == BMF_1BPP || iFormat == BMF_4BPP)
   {
      return NULL;
   }

   // DWORD align each stride
   stride = (sizl.cx*(iFormat/8)+3);
   stride -= stride % 4;
   
   mirrorsize = (int)(sizeof(MIRRSURF) + stride * sizl.cy);

   mirrsurf = (MIRRSURF *) EngAllocMem(FL_ZERO_MEMORY,
                                       mirrorsize,
                                       0x4D495252);
   if (!mirrsurf) {
        RIP("DISP DrvCreateDeviceBitmap failed EngAllocMem\n");
        return(FALSE);
   }
                                       
   dhsurf = (DHSURF) mirrsurf;

   hsurf = (HSURF) EngCreateDeviceBitmap(dhsurf,
                                 sizl,
                                 iFormat);

   if (hsurf == (HSURF) 0)
   {
       RIP("DISP DrvCreateDeviceBitmap failed EngCreateBitmap\n");
       return(FALSE);
   }

   if (!EngAssociateSurface(hsurf, 
                            ppdev->hdevEng, 
                            flGlobalHooks))
   {
       RIP("DISP DrvCreateDeviceBitmap failed EngAssociateSurface\n");
       EngDeleteSurface(hsurf);
       return(FALSE);
   }
  
   mirrsurf->cx = sizl.cx;
   mirrsurf->cy = sizl.cy;
   mirrsurf->lDelta = stride;
   mirrsurf->ulBitCount = iFormat;
   mirrsurf->bIsScreen = FALSE;
  
   return((HBITMAP)hsurf);
}

VOID DrvDeleteDeviceBitmap(
   IN DHSURF dhsurf
   )
{
   MIRRSURF *mirrsurf;
   
   DISPDBG((1, "DeleteDeviceBitmap:\n"));

   mirrsurf = (MIRRSURF *) dhsurf;

   EngFreeMem((PVOID) mirrsurf);
}
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
LONG
DrvRenderHint(DHPDEV dhpdev,
              ULONG  NotifyCode,
              SIZE_T Length,
              PVOID  Data)
{
    PPDEV ppdev = (PPDEV) dhpdev;
    PDRH_APIBITMAPDATA pData = (PDRH_APIBITMAPDATA)Data;

    UNREFERENCED_PARAMETER(ppdev);

    if (NotifyCode == DRH_APIBITMAP && Length && Data)
    {
        DISPDBG((1, "DrvRenderHint(API Render: %08x, %lx)\n", pData->pso, pData->b));
    }

    return TRUE;
}
#endif

/******************************Public*Routine******************************\
* DrvAssertMode
*
* Enable/Disable the given device.
*
\**************************************************************************/

DrvAssertMode(DHPDEV  dhpdev,
              BOOL    bEnable)
{
    PPDEV ppdev = (PPDEV) dhpdev;

    UNREFERENCED_PARAMETER(bEnable);
    UNREFERENCED_PARAMETER(ppdev);

    DISPDBG((0, "DrvAssertMode(%lx, %lx)\n", dhpdev, bEnable));

    return TRUE;

}// DrvAssertMode()

/******************************Public*Routine******************************\
* DrvEscape
*
* We only handle WNDOBJ_SETUP escape. 
*
\**************************************************************************/

typedef struct _WndObjENUMRECTS
{
  ULONG c;
  RECTL arcl[100];
} WNDOBJENUMRECTS;

VOID
vDumpWndObjRgn(WNDOBJ *pwo)
{
    ULONG ulRet;

    ulRet = WNDOBJ_cEnumStart(pwo, CT_RECTANGLES, CD_RIGHTDOWN, 100);

    if (ulRet != 0xFFFFFFFF)
    {
        BOOL bMore;
        ULONG i;
        WNDOBJENUMRECTS enumRects;

        do
        {
          bMore = WNDOBJ_bEnum(pwo, sizeof(enumRects), &enumRects.c);

          for (i = 0; i < enumRects.c; i++)
          {
              DISPDBG((0,"\nWNDOBJ_rect:[%d,%d][%d,%d]",
                          enumRects.arcl[i].left,
                          enumRects.arcl[i].top,
                          enumRects.arcl[i].right,
                          enumRects.arcl[i].bottom));

          }
        } while (bMore);
    }
}

VOID
WndObjCallback(WNDOBJ *pwo,
               FLONG fl)
{
#if (NTDDI_VERSION < NTDDI_VISTA)
    UNREFERENCED_PARAMETER(pwo);
#endif

    if (fl & (WOC_RGN_CLIENT_DELTA |
              WOC_RGN_CLIENT |
              WOC_RGN_SURFACE_DELTA |
              WOC_RGN_SURFACE |
              WOC_CHANGED |
              WOC_DELETE |
              WOC_DRAWN |
              WOC_SPRITE_OVERLAP |
              WOC_SPRITE_NO_OVERLAP
#if (NTDDI_VERSION >= NTDDI_VISTA)
              | WOC_RGN_SPRITE
#endif 
              ))
    {
        DISPDBG((0,"WndObjCallback: "));

        if (fl & WOC_RGN_CLIENT_DELTA) 
            DISPDBG((0,"WOC_RGN_CLIENT_DELTA "));
        if (fl & WOC_RGN_CLIENT) 
            DISPDBG((0,"WOC_RGN_CLIENT "));
        if (fl & WOC_RGN_SURFACE_DELTA) 
            DISPDBG((1,"WOC_RGN_SURFACE_DELTA "));
        if (fl & WOC_RGN_SURFACE) 
            DISPDBG((1,"WOC_RGN_SURFACE "));
        if (fl & WOC_CHANGED) 
            DISPDBG((1,"WOC_CHANGED "));
        if (fl & WOC_DELETE) 
            DISPDBG((0,"WOC_DELETE "));
        if (fl & WOC_DRAWN) 
            DISPDBG((1,"WOC_DRAWN "));
        if (fl & WOC_SPRITE_OVERLAP) 
            DISPDBG((0,"WOC_SPRITE_OVERLAP "));
        if (fl & WOC_SPRITE_NO_OVERLAP)
            DISPDBG((0,"WOC_SPRITE_NO_OVERLAP "));
#if (NTDDI_VERSION >= NTDDI_VISTA)
        if (fl & WOC_RGN_SPRITE)
        {
            DISPDBG((0,"WOC_RGN_SPRITE "));
            vDumpWndObjRgn(pwo);
        }
#endif
        DISPDBG((0,"\n"));
    }
}

ULONG
DrvEscape(SURFOBJ *pso,
          ULONG iEsc,
          ULONG cjIn,
          PVOID pvIn,
          ULONG cjOut,
          PVOID pvOut)
{
    ULONG ulRet = 0;

    UNREFERENCED_PARAMETER(cjIn);
    UNREFERENCED_PARAMETER(pvIn);
    UNREFERENCED_PARAMETER(cjOut);
    UNREFERENCED_PARAMETER(pvOut);

    if (pso->dhsurf)
    {

        if (iEsc == WNDOBJ_SETUP)
        {
            WNDOBJ *pwo = NULL;

            DISPDBG((0,"Attempt to create WndObj\n"));

            pwo = EngCreateWnd(pso,
                               *(HWND*)pvIn,
                               WndObjCallback,
                               WO_DRAW_NOTIFY |
                               WO_RGN_CLIENT |
                               WO_RGN_CLIENT_DELTA |
                               WO_RGN_WINDOW |
                               WO_SPRITE_NOTIFY
#if (NTDDI_VERSION >= NTDDI_VISTA)
                               | WO_RGN_SPRITE
#endif
                               ,
                               0);
            if (pwo != NULL)
            {
                DISPDBG((0,"WndObj creat success\n"));
                ulRet = 1;
            }
        }
    }

    return ulRet;
}

