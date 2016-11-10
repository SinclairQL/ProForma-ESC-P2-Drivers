/*
    PROforma, PROGS Font & Raster Manager

    Epson Stylus Colour driver - with colour replacement for LINEdesign

    (c) PROGS, Professional & Graphical Software
    written by Joachim Van der Auwera
    started 02 November 1995, based on LaserJet4routs_c
    Updated 27 July 1999 by Rich Mellor to include 720 and 1440 dpi print modes
    Updated 29 December 2001 by Rich Mellor to include flush buffers
*/

#include <mem.h>
#include "win1_pws_pf_driv_escp2_h"

Bbox PrintableArea={
    double2pt(575.6), double2pt(780.0), /* size */
    double2pt(  8.6), double2pt(  8.6)  /* origin */
};

char myDefaultDevice[IO_MAXFULLNAME]="pard";

Error Define(char *name, char *value)
{
    double x,y;

    if (STRSameCD(name,"DEFAULT-DEVICE"))
    {
        STRCopySafe(value,myDefaultDevice,IO_MAXFULLNAME);
        return ERR_OK;
    }
    if (STRSameCD(name,"PRINTABLE-AREA-SIZE"))
    {
        STRFormatGet(value,NULL,"%f %f",&x,&y);
        PrintableArea.xsiz=double2pt(x);
        PrintableArea.ysiz=double2pt(y);
        return ERR_OK;
    }
    if (STRSameCD(name,"PRINTABLE-AREA-ORIGIN"))
    {
        STRFormatGet(value,NULL,"%f %f",&x,&y);
        PrintableArea.xorg=double2pt(x);
        PrintableArea.yorg=double2pt(y);
        return ERR_OK;
    }
    return ERR_IPAR;
}


Error InitInit(Gstate gstate, char *device, PBbox *area)
{
    Error err;
    DriverInfo *di;
    DRIVdriver *prt=gstate->printer;

    err=MEMAllocate(sizeof(DriverInfo),&di);
    if (err) return err;

    if (device && *device)
        err=IOOpen(device,OPEN_OVER,&di->channel);
    else
        err=IOOpen(myDefaultDevice,OPEN_OVER,&di->channel);
    if (err) { MEMRelease(di); return err; };

    gstate->PrinterExtra=di;
    gstate->todev.x = pt_one * prt->xdpi / 72;
    gstate->todev.y = pt_one * prt->ydpi / 72;
    gstate->ppp.x = pt_one * 72 / prt->xdpi;
    gstate->ppp.y = pt_one * 72 / prt->ydpi;

    area->xsiz=pt2rshort(fixmul(PrintableArea.xsiz,gstate->todev.x));
    area->ysiz=pt2rshort(fixmul(PrintableArea.ysiz,gstate->todev.y));
    area->xorg=pt2rshort(fixmul(PrintableArea.xorg,gstate->todev.x));
    area->yorg=pt2rshort(fixmul(PrintableArea.yorg,gstate->todev.y));

    return err;
}

Error InitDone(Gstate gstate)
{
    DriverInfo *di=gstate->PrinterExtra;
    Size linc;

    gstate->bitmap->BufferSize(gstate->pagebbox.xsiz,2,&linc);

    /* complete the initialisation of the DriverInfo */
    /* Changed to allow a seed row to be stored for each colour */

    di->out_line=(char *)gstate->raster->base;
    di->prev_line=di->out_line+linc;

    /* prev_line needs to store all 4 colour planes, therefore, make it linc*4 */
    di->temp_line=di->prev_line+linc*4;

    /* Specify start of picture data */
    gstate->raster->base=(unsigned char *)(di->temp_line+linc);

    di->xskip=pt2rshort(gstate->pagebbox.xporg-fixmul(PrintableArea.xorg,
                               gstate->todev.x));
    di->yskip=pt2rshort(gstate->pagebbox.yporg-fixmul(PrintableArea.yorg,
                               gstate->todev.y));

    return ERR_OK;
}

Error BufferSize(Gstate gstate, int xsiz, int ysiz, Size *size)
{
    /* Specify how much memory is required to store a page (ysiz plus enough
       lines for the out_line and prev_line for each stored line *2 - prev_line is 4*linc)
    */

    return gstate->bitmap->BufferSize(xsiz,ysiz+12,size);
}


Error CleanUp(Gstate gstate)
{
    DriverInfo *di=gstate->PrinterExtra;

    if (di)
    {
        IOClose(di->channel);
        MEMRelease(gstate->PrinterExtra);
    }
    return ERR_OK;
}

Error ColourNearest(Gstate gstate, ColourCMYK *colour, int *result)
{
    *result=0;                      /* assume white */
    if (colour->cyan>pt_hundred/2)
        *result+=8;
    if (colour->magenta>pt_hundred/2)
        *result+=4;
    if (colour->yellow>pt_hundred/2)
        *result+=2;
    if (colour->black>pt_hundred/2)
        *result+=1;
    return ERR_OK;
}

Error ColourDelta(Gstate gstate, ColourCMYK *colour, ColourCMYK *delta)
{
    *delta=*colour;
    if (colour->cyan>pt_hundred/2)
        delta->cyan-=pt_hundred;
    if (colour->magenta>pt_hundred/2)
        delta->magenta-=pt_hundred;
    if (colour->yellow>pt_hundred/2)
        delta->yellow-=pt_hundred;
    if (colour->black>pt_hundred/2)
        delta->black-=pt_hundred;
    return ERR_OK;
}

#define p100 short2pt(100)
#define p050 short2pt( 50)
#define p000 short2pt(  0)

/* Define the colour settings */

ColourCMYK replace[18]={
    { p100,p000,p000,short2pt( 1) },        /* Cyan */
    { p000,p100,p000,short2pt( 7) },        /* Magenta */
    { p000,p000,p100,short2pt(11) },        /* Yellow */
    { p100,p100,p000,short2pt(17) },        /* Dark Blue */
    { p000,p100,p100,short2pt(21) },        /* Orange */
    { p100,p000,p100,short2pt(27) },        /* Green */
    { p100,p050,p050,short2pt(31) },        /* Dark Green */
    { p050,p100,p050,short2pt(37) },        /* Charcoal */
    { p050,p050,p100,short2pt(41) },        /* Grey-Green */
    { p100,p100,p050,short2pt(47) },        /* Grey-Blue */
    { p050,p100,p100,short2pt(51) },        /* Brown */
    { p100,p050,p100,short2pt(57) },        /* Grey-Green (more green) */
    { p050,p000,p000,short2pt(61) },        /* Light Cyan */
    { p000,p050,p000,short2pt(67) },        /* Light Magenta */
    { p000,p000,p050,short2pt(71) },        /* Light Yellow */
    { p050,p050,p000,short2pt(77) },        /* Light Blue */
    { p000,p050,p050,short2pt(81) },        /* Light Orange */
    { p050,p000,p050,short2pt(87) },        /* Light Orange */
};

Error ColourAdjust(Gstate gstate, ColourCMYK *col)
{
    int i;
    if (col->cyan==0 && col->magenta==0 && col->yellow==0)
    {
        for (i=0 ; i<18 ; i++)
        {
            if (replace[i].black==col->black)
            {
                col->black=0;
                col->cyan   =replace[i].cyan;
                col->magenta=replace[i].magenta;
                col->yellow =replace[i].yellow;
            }
        }
    }
    return ERR_OK;
}

static Error nimp()     { return ERR_NIMP; }
static Error nothing()  { return ERR_OK; }

Error InitDriver();

Error InitPage(Gstate);
Error ShowPage(Gstate);
Error StopPage(Gstate);

Error SetCopies(Gstate, int count);
Error SetDisplayMode(Gstate, int mode);
Error WindowMove(Gstate, int dx, int dy);
Error BufferUpdate(Gstate);

Error GetFullPageBbox(Gstate, Bbox *box);
Error DefaultDevice(Gstate, char *device);

#define InitDriver      nothing
#define WindowMove      nothing
#define SetDisplayMode  nimp
#define GetFullPageBbox nimp
#define DefaultDevice   nimp
#define SetCopies       nimp
#define BufferUpdate    (Error (*)(Gstate))nimp
#define BufferSave      (Error (*)(Gstate))nimp
#define BufferRestore   (Error (*)(Gstate))nimp
#define Handle          nimp

DRIVdriver dpi180={
    NULL, PF_PRINTERDRIVER,
    "Epson Stylus Colour v1.05 180dpi colrepA5", /* name */
    180,180,                    /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        4, 1,                   /* 4 plane, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_SEPARATE_PLANES, /* no planes */
    },

    InitDriver,
    Define,

    InitInit,
    BufferSize,
    InitDone,
    CleanUp,

    InitPage,
    ShowPage,
    StopPage,

    SetCopies,
    SetDisplayMode,
    WindowMove,

    BufferUpdate,
    BufferSave,
    BufferRestore,

    PF_COLOURSPACE_CMYK,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta,
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};

DRIVdriver dpi360={
    &dpi180, PF_PRINTERDRIVER,
    "Epson Stylus Colour v1.05 360dpi colrepA5", /* name */
    360,360,                    /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        4, 1,                   /* 1 planes, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_SEPARATE_PLANES, /* no planes */
    },

    InitDriver,
    Define,

    InitInit,
    BufferSize,
    InitDone,
    CleanUp,

    InitPage,
    ShowPage,
    StopPage,

    SetCopies,
    SetDisplayMode,
    WindowMove,

    BufferUpdate,
    BufferSave,
    BufferRestore,

    PF_COLOURSPACE_CMYK,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta,
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};

DRIVdriver init={
    &dpi360, PF_PRINTERDRIVER,
    "Epson Stylus Colour v1.05 720dpi colrepA5", /* name */
    720,720,                    /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        4, 1,                   /* 1 planes, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_SEPARATE_PLANES, /* no planes */
    },

    InitDriver,
    Define,

    InitInit,
    BufferSize,
    InitDone,
    CleanUp,

    InitPage,
    ShowPage,
    StopPage,

    SetCopies,
    SetDisplayMode,
    WindowMove,

    BufferUpdate,
    BufferSave,
    BufferRestore,

    PF_COLOURSPACE_CMYK,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta,
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};

DRIVdriver dpi1440={
    &init, PF_PRINTERDRIVER,
    "Epson Stylus Colour v1.05 1440dpi colrepA5", /* name */
    1440,720,                   /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        4, 1,                   /* 1 planes, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_SEPARATE_PLANES, /* no planes */
    },

    InitDriver,
    Define,

    InitInit,
    BufferSize,
    InitDone,
    CleanUp,

    InitPage,
    ShowPage,
    StopPage,

    SetCopies,
    SetDisplayMode,
    WindowMove,

    BufferUpdate,
    BufferSave,
    BufferRestore,

    PF_COLOURSPACE_CMYK,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta,
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};
