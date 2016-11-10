/*
    PROforma, PROGS Font & Raster Manager

    routines to control the Epson Stylus printer drivers

    (c) PROGS, Professional & Graphical Software
    written by Joachim Van der Auwera
    started 02 November 1995, based on LaserJet4routs_c
    updated 29/9/00 to allow Delta Row Compression
    updated 01/01/02 to allow Flush Buffers and overcome pass printing problems
    updated 18/01/05 to correct some problems with end of page
*/

#include "mem_h"

#include "win1_pws_pf_driv_escp2_h"

Bbox PrintableArea={
    double2pt(575.6), double2pt(780.0), /* size */
    double2pt(  8.6), double2pt(  8.6)  /* origin */
    };

char DefaultDevice[IO_MAXFULLNAME]="pard";

Error Define(char *name, char *value)
{
    double x,y;

    if (STRSameCD(name,"DEFAULT-DEVICE"))
    {
        STRCopySafe(value,DefaultDevice,IO_MAXFULLNAME);
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
        err=IOOpen(DefaultDevice,OPEN_OVER,&di->channel);
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
