/*
    PROforma, PROGS FOnt & Raster MAnager

    Printer drivers for all Epson ESC\P2 (compatible) printers
    Colour version - 180, 360, and 720 dpi only

    VERSION: 1.05

    (c) RWAP Software
    written by Rich Mellor
    with thanks to Joachim Van der Auwera of Progs for writing the original
    RLE compression modes (180 and 360 dpi only)

    this file is based on PROforma v1.0? DeskJet external driver
    02.11.1995 for PROforma v1.10 - different driver structure
    02.11.1995 finally introduced skip_lines in driver - better speed etc.
    29.06.1996 fixed a bug in CompressMode2
    20.07.1999 first attempt at 720 dpi
    21.07.1999 CASE function added instead of the IF... tests for dpi
    16.09.1999 TIFF compression added and empty lines jumped - quicker printing!
    20.01.2000 StopPage altered to try and ensure page is always ejected.
    29.09.2000 Added Delta Row Compression for 720 dpi
    13.03.2001 Fixed bug which meant black got out of sinc when printing <720dpi.
               Microweave added for 360 and 720 dpi
               Binary command mode now entered in InitPage() and left in StopPage().
    01.01.2002 Added flush buffers to end of graphics run
               Overcame problems with Delta Row Compression on loss of previous line
               between passes
    11.01.2005 Code for switching off microweave at end updated - was sending an extra
               character
*/

/*
---------------  SUGGESTED IMPROVEMENTS ----------------------------
1. Add economy print mode - ignore every even line in picture.
   May need to change escp2_h header and also styluscolour2_c to add an option
   to the calling menu??

2. Add small dot size for 720+ dpi - may also need it for economy print

*/

#include "io_h"
#include "ESCP2Module_h"
#include "driv_escp2_h"

/* this driver allows from 180 to 720dpi resolution, */
/* x and y resolution must be the same */

/* some macros for generating output */
#define printb(b,s) IOPutRawBytes(chid,s,(char *)b,NULL)
#define prints(s)   printb(s,strlen(s))
#define printc(c)   { char s=c; IOPutRawBytes(chid,1,&s,NULL); }

/* forward references */
static Move_Down_Page(Gstate gstate, int skip_lines);
static Set_Horizontal(Gstate gstate, int offset);
static Output_Line(Gstate gstate, char *start, char *end, int Colour);

/* initialise the printer */

Error InitPage(Gstate gstate)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;

    /* put printer in known state (reset) */
    prints("\x1b@");

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

                /* Clear out previous line buffer for all 4 seed rows */
                MEMClear(4*gstate->raster->linc,di->prev_line);

                break;

    
        case 1440:
                printc(0); printc(5);       /* 720 dpi resolution for time being*/
                break;
    }

    di->skip_lines=0;

    /* Select appropriate raster graphics (TIFF) mode for current dpi */
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

        case 1440:
            /* It is not clear what parameters to pass for 1440 dpi */
            prints("\x1b.\2\x05\x05\1");
            printc(0); printc(0);
            break;
    }

    printc (MOVXBYTE);

    return ERR_OK;
}


/* Send buffer to printer - use compression to speed up data transfer */

Error ShowPage(Gstate gstate)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;
    int line_size=gstate->raster->linc;
    int pinc=gstate->raster->pinc;
    char *data, *end_data, *prev_seed;
    char *out_data=di->out_line;
    char *temp_data=di->temp_line;
    char *prev_data=di->prev_line;
    char *buff=(char *)gstate->raster->base;
    int out_len, i, pixcount, emptyflag;
    int skip_lines=di->skip_lines;
    int skip_yellow=di->skip_yellow;
    int skip_cyan=di->skip_cyan;
    int skip_magenta=di->skip_magenta;
    int skip_black=di->skip_black;

    Move_Down_Page(gstate,di->yskip);

    for (i=gstate->pagebbox.ysiz ; i ; i--, buff+=line_size)
    {
        /* Presume that each line is empty */
        emptyflag=1;

        /* YELLOW PLANE */
        data=buff+2*pinc;
        prev_seed=prev_data+2*line_size;
        end_data=data+line_size;
        pixcount=line_size*8;

        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_data>data && !end_data[-1]) end_data--, pixcount-=8;

        if (end_data==data)
        {
            if (skip_yellow==0)
            {
                if (gstate->printer->xdpi==720)
                {
                    /* Clear seed row data - nothing at all to print */
                    MEMClear(line_size, prev_seed);
                    /* set printing colour */
                    printc (P_COLR+4);
                    printc(CLR);
                }
            }
        skip_yellow++;
        }
        else
        {
            /* Ensure this line is not counted as empty */
            emptyflag=0;
            skip_yellow=0;

            /* Set vertical start position */
            Move_Down_Page(gstate,skip_lines);
            skip_lines=0;

            Set_Horizontal(gstate, di->xskip);
            
            /* compress the line */
            if (gstate->printer->xdpi==720)     /* Use Delta Row compression */
            {
                out_len=CompressMode3(line_size, data, prev_seed, out_data, temp_data);
                if (out_len!=0)
                {
                    MEMCopy(line_size,data,prev_seed);

                    /* set printing colour */
                    printc (P_COLR+4);
                    printb (out_data,out_len);
                    printc (CR);
                }
            }
            else                                /* Use TIFF compression */
            {
                Output_Line(gstate, data, end_data, P_COLR+4);
            }
        }

        /* CYAN PLANE */
        data=buff;
        prev_seed=prev_data;
        end_data=data+line_size;
        pixcount=line_size*8;

        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_data>data && !end_data[-1]) end_data--, pixcount-=8;

        if (end_data==data)
        {
            if (skip_cyan==0)
            {
                if (gstate->printer->xdpi==720)
                {
                    /* Clear seed row data - nothing at all to print */
                    MEMClear(line_size, prev_seed);
                    /* set printing colour */
                    printc (P_COLR+2);
                    printc (CLR);
                }
            }
        skip_cyan++;
        }
        else
        {
            /* Ensure this line is not counted as empty */
            emptyflag=0;
            skip_cyan=0;

            /* Set vertical start position */
            Move_Down_Page(gstate,skip_lines);
            skip_lines=0;

            Set_Horizontal(gstate, di->xskip);
            
            /* compress the line */
            if (gstate->printer->xdpi==720)     /* Use Delta Row compression */
            {
                out_len=CompressMode3(line_size, data, prev_seed, out_data, temp_data);
                if (out_len!=0)
                {
                    MEMCopy(line_size,data,prev_seed);

                    /* set printing colour */
                    printc (P_COLR+2);
                    printb (out_data,out_len);
                    printc (CR);

                }
            }
            else                                /* Use TIFF compression */
            {
                Output_Line(gstate, data, end_data, P_COLR+2);
            }
        }

        /* MAGENTA */
        data=buff+pinc;
        prev_seed=prev_data+line_size;
        end_data=data+line_size;
        pixcount=line_size*8;

        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_data>data && !end_data[-1]) end_data--, pixcount-=8;

        if (end_data==data)
        {
            if (skip_magenta==0)
            {
                if (gstate->printer->xdpi==720)
                {
                    /* Clear seed row data - nothing at all to print */
                    MEMClear(line_size, prev_seed);
                    /* set printing colour */
                    printc (P_COLR+1);
                    printc (CLR);
                }
            }
        skip_magenta++;
        }
        else
        {
            /* Ensure this line is not counted as empty */
            emptyflag=0;
            skip_magenta=0;

            /* Set vertical start position */
            Move_Down_Page(gstate,skip_lines);
            skip_lines=0;

            Set_Horizontal(gstate, di->xskip);

            /* compress the line */
            if (gstate->printer->xdpi==720)     /* Use Delta Row compression */
            {
                out_len=CompressMode3(line_size, data, prev_seed, out_data, temp_data);
                if (out_len!=0)
                {
                    MEMCopy(line_size,data,prev_seed);

                    /* set printing colour */
                    printc (P_COLR+1);
                    printb (out_data,out_len);
                    printc (CR);
                }
            }
            else                                /* Use TIFF compression */
            {
                Output_Line(gstate, data, end_data, P_COLR+1);
            }
        }

        /* BLACK */
        data=buff+3*pinc;
        prev_seed=prev_data+3*line_size;
        end_data=data+line_size;
        pixcount=line_size*8;

        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_data>data && !end_data[-1]) end_data--, pixcount-=8;

        if (end_data==data)
        {
            if (skip_black==0)
            {
                if (gstate->printer->xdpi==720)
                {
                    /* Clear seed row data - nothing at all to print */
                    MEMClear(line_size, prev_seed);
                    /* set printing colour */
                    printc (P_COLR+0);
                    printc (CLR);
                }
            }
        skip_black++;
        }
        else
        {
            /* Ensure this line is not counted as empty */
            emptyflag=0;
            skip_black=0;

            /* Set vertical start position */
            Move_Down_Page(gstate,skip_lines);
            skip_lines=0;

            Set_Horizontal(gstate, di->xskip);

            /* compress the line */
            if (gstate->printer->xdpi==720)     /* Use Delta Row compression */
            {
                out_len=CompressMode3(line_size, data, prev_seed, out_data, temp_data);
                if (out_len!=0)
                {
                    MEMCopy(line_size,data,prev_seed);

                    /* set printing colour */
                    printc (P_COLR+0);

                    printb (out_data,out_len);
                    printc (CR);
                }
            }
            else                                /* Use TIFF compression */
            {
                Output_Line(gstate, data, end_data, P_COLR+0);
            }
        }

        /* Move down one line on page (if not an empty line) */

        if (emptyflag==1)
        {
            /* Remark no graphics on this line */
            skip_lines++;
        }
        else
        {
            /* Move to next line down */
            printc(MOVY+1);
        }
    }

    /* Store number of skipped lines ready for next pass */

    di->skip_lines=skip_lines;
    di->skip_yellow=skip_yellow;
    di->skip_cyan=skip_cyan;
    di->skip_magenta=skip_magenta;
    di->skip_black=skip_black;

    return ERR_OK;
}

/* handles everything to end the printing */

Error StopPage(Gstate gstate)
{
    Channel chid=gstate->PrinterExtra->channel;

    /* Leave binary compression mode */
    printc (EXIT);

    /* v1.05 - updated to split the code into two lines. */
    /* eject page and end raster graphics */
    prints("\x1b(i");    /* Turn off any microweave */
    printc(1); printc(0); printc(48);

    /* v1.02 - changed to include carriage return - reset printer now at end */
    /* v1.05 - was missing the escape character */
    prints("\x0d\x0c");          /* carriage return and form feed */

    /* v1.02 - formerly only changed loading to bin 1 (ESC,EM,49). */
    prints("\x1b\x19R");        /* Eject paper in printer */

    /* v1.04 - added to try and help problems on Epson 900 */
    prints("\x1b\x06");         /* Flush Out buffers - needed for post 850 */

    prints("\x1b@");            /* Reset printer - exit graphics mode */

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

/* Set a relative horizontal position in TIFF compression mode */

Error Set_Horizontal(Gstate gstate, int offset)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;

    /* Now move to the given relative position */
    if (offset>0)
    {
        printc (MOVX);
        printc (offset%256);            /* xskip MOD 256 */
        printc (offset/256);            /* xskip DIV 256 */
    }

}

/* Output any given line to printer */
Error Output_Line(Gstate gstate, char *start, char *end, int Colour)
{
    DriverInfo *di=gstate->PrinterExtra;
    Channel chid=di->channel;
    int out_len, pixcount;
    char *out_data=di->out_line;

    out_len=CompressMode2(start,end,out_data);

    if (out_len>0)
    {
        printc (Colour);
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
        printb (out_data,out_len);
    }
}
