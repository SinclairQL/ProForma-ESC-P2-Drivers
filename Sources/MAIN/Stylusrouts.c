/*
    PROforma, PROGS Font & Raster Manager

    routines to control the Epson Stylus printer drivers

    (c) PROGS, Professional & Graphical Software
    written by Joachim Van der Auwera
    started 02 November 1995, based on LaserJet4routs_c
    updated 27/9/00 to include Delta Row Compression
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
    di->out_line=(char *)gstate->raster->base;

    /* Change to allow a seed row to be stored for black */
    di->prev_line=di->out_line+linc;
    di->temp_line=di->prev_line+linc*4;
    di->odd_dots=di->temp_line+linc;
    di->even_dots=di->odd_dots+linc;
    gstate->raster->base=(unsigned char *)(di->even_dots+linc);

    di->xskip=pt2rshort(gstate->pagebbox.xporg-fixmul(PrintableArea.xorg,
                               gstate->todev.x));
    di->yskip=pt2rshort(gstate->pagebbox.yporg-fixmul(PrintableArea.yorg,
                               gstate->todev.y));

    return ERR_OK;
}

Error BufferSize(Gstate gstate, int xsiz, int ysiz, Size *size)
{
    return gstate->bitmap->BufferSize(xsiz,ysiz+16,size);
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

Error ColourNearest(Gstate gstate, pt *colour, int *result)
{
    *result=0;                      /* assume white */
    if (*colour>pt_hundred/2)
        *result=1;                  /* make it black */
    return ERR_OK;
}

Error ColourDelta(Gstate gstate, pt *colour, pt *delta)
{
    *delta=*colour;
    if (*colour>pt_hundred/2)
        *delta-=pt_hundred;
    return ERR_OK;
}
