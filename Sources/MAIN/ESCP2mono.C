/*
    PROforma, PROGS FOnt & Raster MAnager

    Printer drivers for all Epson ESC\P2 (compatible) printers
    Mono version - 180, 360, 720 and 1440 dpi

    VERSION: 1.05

    (c) RWAP Software
    written by Rich Mellor
    with thanks to Joachim Van der Auwera of Progs for writing the original
    RLE compression modes (180 and 360 dpi only)

    this file is based on PROforma v1.0? DeskJet external driver
    02.11.1995 for PROforma v1.10 - different driver structure
    02.11.1995 finally introduced skip_lines in driver - better speed etc.
    29.06.1996 fixed a bug in CompressMode2
    16.09.1999 TIFF compression added and empty lines jumped - quicker printing!
               720 dpi mode added
    20.01.2000 Stoppage altered to try and ensure page is always ejected at end
               of print.
    30.09.2000 Monochrome print mode added - speeds up printer.
               MOVX 0,0 and MOVY 0,0 are now ignored.
               Microweave added to 360 dpi and 720 dpi
               Delta Row compression added for 720dpi
    07.11.2000 CompressMode2 and CompressMode3 re-written as m/c module
               Added 1440 dpi test mode * NOT YET WORKING *
    01.01.2002 Flush Buffers and microweave added for all modes.
    12.01.2005 Updated the end code for switching off microweave - was sending
               extra char
*/

/*
---------------  SUGGESTED IMPROVEMENTS ----------------------------
1. Add economy print mode - ignore every even line in picture.

2. Add small dot size for 720+ dpi - may also need it for economy print
*/

#include "io_h"
#include "ESCP2Module_h"
#include "driv_escp2_h"

/* this driver allows from 180 to 1440dpi resolution, */
/* Normally printer driver assumes x and y dpi are the same */

/* some macros for generating output */
#define printb(b,s) IOPutRawBytes(chid,s,(char *)b,NULL)
#define prints(s)   printb(s,strlen(s))
#define printc(c)   { char s=c; IOPutRawBytes(chid,1,&s,NULL); }

/* forward references */
static Move_Down_Page(Gstate gstate, int skip_lines);
static Set_Horizontal(Gstate gstate, int offset);
static Output_Line(Gstate gstate, char *start, char *end);

/* initialise the printer */

Error InitPage(Gstate gstate)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;

    /* put printer in known state (reset) */
    prints("\x1b@");

    /* select monochrome printing mode - should speed up driver */
    prints("\x1b(K\x02");
    printc(0); printc(0); printc(1);

    /* set graphics mode */
    prints("\x1b(G\1");
    printc(0); printc(1);

    /* enable microweave */
    prints("\x1b(i\1");
    printc(0); printc(1);

    /* Select resolution and line spacing */

    switch (gstate->printer->xdpi)
    {
        case 180:
                prints("\x1b(U\1");
                printc(0); printc(20);      /* 180 dpi resolution */
                break;
    
        case 360:
                prints("\x1b(U\1");
                printc(0); printc(10);      /* 360 dpi resolution */
                break;
    
        case 720:
                prints("\x1b(U\1");
                printc(0); printc(5);       /* 720 dpi resolution */

                /* Clear out previous line buffer */
                MEMClear(gstate->raster->linc,di->prev_line);

                break;
    
        case 1440:
                prints("\x1b(U\1");
                printc(0); printc(5);       /* 720 dpi resolution */

/* Stylus 600 printer ONLY

                prints("\x1b(e\2");         smallest dot size
                printc(0); printc(0); printc(2);
*/
                /* All other printers */
                prints("\x1b(e\2");         /* smallest dot size */
                printc(0); printc(0); printc(1);

                prints("\x1b(s\1");         /* slowest print speed */
                printc(0); printc(2);

                break;
    }

    /* Select appropriate raster graphics (TIFF or Delta Row) mode for current dpi */
    /* Note once in TIFF mode, cannot use anything other than the binary */
    /* commands */

    switch (gstate->printer->xdpi)
    {
        case 180:
            prints("\x1b.\2\x14\x14\1");
            printc(0); printc(0);
            break;

        case 360:
            prints("\x1b.\2\x0a\x0a\1");
            printc(0); printc(0);
            break;

        case 720:
            prints("\x1b.\3\x05\x05\1");    /* Delta row compression */
            printc(0); printc(0);
            break;

    }
    if (gstate->printer->xdpi!=1440) printc (MOVXBYTE);
    di->skip_lines=0;
    return ERR_OK;
}


/* Send buffer to printer - use compression to speed up data transfer */

Error ShowPage(Gstate gstate)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;
    int line_size=gstate->raster->linc;
    char *end_data, *end_odd_dots, *end_even_dots;
    char *out_data=di->out_line;
    char *temp_data=di->temp_line;
    char *prev_data=di->prev_line;
    char *odd_dots=di->odd_dots;
    char *even_dots=di->even_dots;
    char *data=(char *)gstate->raster->base;
    int out_len, i;
    int odd_diff, movement;
    int skip_lines=di->skip_lines;

    Move_Down_Page(gstate,di->yskip);

    for (i=gstate->pagebbox.ysiz ; i ; i--, data+=line_size)
    {
        /* Printer driver will only work with same x and y dpi */
        /* Therefore for 1440x720dpi we need to ignore every second line */

        if (gstate->printer->xdpi==1440 && i%2) continue;

        end_data=data+line_size;

        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_data>data && !end_data[-1]) end_data--;

        if (end_data==data)
        {
            if (gstate->printer->xdpi==720)
            {
                if (skip_lines==0)
                {
                    /* Clear seed row data - nothing at all to print */
                    MEMClear(line_size, prev_data);
                    printc(CLR);
                }
            }
            skip_lines++;
            continue;
        }
        else
        {
            /* Set vertical start position */
            Move_Down_Page(gstate,skip_lines);
            skip_lines=0;

            Set_Horizontal(gstate, di->xskip);
            
            /* compress the line */
            if (gstate->printer->xdpi==720)     /* Use Delta Row compression */
            {
                out_len=CompressMode3(line_size, data, prev_data, out_data, temp_data);
                if (out_len>0)
                {
                    MEMCopy(line_size,data,prev_data);
                    printb (out_data,out_len);
                }
            }
            else
            {

                /* This is VERY rough code for 1440 dpi printing - NOT YET IMPLEMENTED!! */
                if (gstate->printer->xdpi==1440)
                {
                    /* Split the line into odd and even pixels */
                    odd_diff=SplitPixels(data,end_data,odd_dots,even_dots);

                    end_odd_dots=odd_dots+odd_diff;

                    /* Deal with the odd pixels first */
                    /* remove trailing zeros - printing "nothing" is useless ! */
                    while (end_odd_dots>odd_dots && !end_odd_dots[-1]) end_odd_dots--;

                    if (end_odd_dots!=odd_dots)
                    {
                        /* compress the line and print it*/
                        Output_Line(gstate, odd_dots, end_odd_dots);
                    }

                    /* Now to deal with the even pixels */

                    end_even_dots=even_dots+odd_diff;

                    /* remove trailing zeros - printing "nothing" is useless ! */
                    while (end_even_dots>even_dots && !end_even_dots[-1]) end_even_dots--;

                    if (end_even_dots!=even_dots)
                    {
                        /* Move to the correct indent margin if required in 1/1440 inch units */
                        /* di->xskip is in units of 8 pixels ie. bytes */
                        /* therefore convert this to steps of 1/1440" units */
                        /* If di->xskip=90, this gives 90*8=720 pixels (one inch) */
                        /* Equivalent to 16*90=1440 1/1440" units */

                        movement=di->xskip*16;

                        /* Flush buffers on later printers - ignored if not available */
                        prints("\x1b\x36");

                        printc(13);

                        /* Offset by 1/1440" for even pixels */
                        movement++;

                        /* We now need to move to the start of the line plus 1/1440 inches */
                        prints("\x1b(\x5c");
                        printc(4); printc(0); printc(-96);
                        printc(5);
                        printc(-movement%256);
                        printc(-movement/256);

                        /* This next line causes the problems - all further lines cause */
                        /* the print head to move correctly, but nothing is printed */

                        Output_Line(gstate, even_dots, end_even_dots);
                    }
                    /* Print carriage return and line feed */
                    printc(13);
                    Move_Down_Page(gstate,1);
                }
                else
                {
                    Output_Line(gstate, data, end_data);
                }
            }
            if (gstate->printer->xdpi!=1440) printc (MOVY+1);
        }
    }
    /* Update number of skipped lines ready for the next buffer */
    di->skip_lines=skip_lines;

    return ERR_OK;
}

/* handles everything to end the printing */

Error StopPage(Gstate gstate)
{
    Channel chid=gstate->PrinterExtra->channel;

    /* Exit binary command mode */
    if (gstate->printer->xdpi!=1440) printc (EXIT);

    /* v1.05 - updated to split the code into two lines. */
    /* eject page and end raster graphics */
    prints("\x1b(i");    /* Turn off any microweave */
    printc(1); printc(0); printc(48);

    /* v1.05 - was missing the escape character */
    prints("\x0d\x0c");          /* carriage return and form feed */

    /* v1.02 - formerly only changed loading to bin 1 (ESC,EM,49). */
    prints("\x1b\x19R");        /* Eject paper in printer */

    /* v1.04 - added to try and help problems on Epson 900 */
    prints("\x1b\x06");         /* Flush Out buffers - needed for post 850 */

    prints("\x1b@");            /* Reset printer */

    return ERR_OK;
}

/* Routine to move the printer down as many lines as requied on the page */

Error Move_Down_Page(Gstate gstate, int skip_lines)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;

    /* move to next printable line down */
    if (skip_lines)
    {
        if (gstate->printer->xdpi==1440)
        {
            prints("\x1b(v\2");
            printc(0);
            printc (skip_lines%256);
            printc (skip_lines/256);

        }
        else
        {
            if (skip_lines<16)
            {
                printc (MOVY+skip_lines);
            }
            else
            {
                if (skip_lines<256)
                {
                    printc (MOVY+16+1);
                    printc (skip_lines);
                }
                else
                {
                    printc (MOVY+16+2);
                    printc (skip_lines%256);
                    printc (skip_lines/256);
                }
            }
        }
    }
}

/* Set a relative horizontal position in TIFF compression mode */

Error Set_Horizontal(Gstate gstate, int offset)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;

    /* Now move to the given relative position */
    if (offset>0)
    {
        if (gstate->printer->xdpi==1440)
        {
            prints("\x1b\x5c");
            printc (offset%256);
            printc (offset/256);
        }
        else
        {
            printc (MOVX);
            printc (offset%256);            /* xskip MOD 256 */
            printc (offset/256);            /* xskip DIV 256 */
        }
    }

}

/* Output any given line to printer */
Error Output_Line(Gstate gstate, char *start, char *end)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;
    int out_len, pixcount;
    char *out_data=di->out_line;

    out_len=CompressMode2(start,end,out_data);

    if (out_len>0)
    {
        if (gstate->printer->xdpi==1440)
        {
            /* Work out number of pixels being printed */
            pixcount=(end-start)*8;

            prints("\x1b.\1");
            printc(5);
            printc(5);
            printc(1);
            printc (pixcount%256);
            printc (pixcount/256);
        }
        else
        {
            if (out_len<16)
            {
                printc (XFER+out_len);
            }
            else
            {
                if (out_len<256)
                {
                    printc (XFER+16+1);
                    printc (out_len);
                }
                else
                {
                    printc (XFER+16+2);
                    printc (out_len%256);
                    printc (out_len/256);
                }
            }
        }
        printb (out_data,out_len);
    }
}
