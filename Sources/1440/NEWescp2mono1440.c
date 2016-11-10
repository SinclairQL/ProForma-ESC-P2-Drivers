/*
    PROforma, PROGS FOnt & Raster MAnager

    Printer drivers for all Epson ESC\P2 (compatible) printers
    Mono version - 1440 dpi only (test version)

    VERSION: 1.02

    (c) RWAP Software
    written by Rich Mellor

    with thanks to Joachim Van der Auwera of Progs for writing the original
    RLE compression modes

    NOTE: this does not support TIFF/Delta Row Compression at present

*/

#include "io_h"
#include "driv_escp2A_h"         /* 1440dpi version */

/* Forward references */
static int CompressMode2(char *, char *, char *);
static int CompressMode3(int size, char *, char *, char *, char *);

/* some macros for generating output */
#define printb(b,s) IOPutRawBytes(chid,s,(char *)b,NULL)
#define prints(s)   printb(s,strlen(s))
#define printc(c)   { char s=c; IOPutRawBytes(chid,1,&s,NULL); }

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

    /* Select resolution and line spacing */

    switch (gstate->printer->xdpi)
    {
        case 1440:

/* Stylus 600 printer ONLY

                prints("\x1b(e\2");         smallest dot size
                printc(0); printc(0); printc(0); printc(2);
*/
                /* All other printers */
                prints("\x1b(e\2");         /* smallest dot size */
                printc(0); printc(0); printc(1);

                prints("\x1b(s\1");         /* slowest print speed */
                printc(0); printc(2);

                prints("\x1b(i\1");         /* microweave enabled  */
                printc(0); printc(1);
                prints("\x1b(U\1");
                printc(0); printc(5);       /* 720 dpi resolution */
                break;
    }

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
    int out_len, i, pixcount, emptyflag;
    int databyte, evenbyte, oddbyte, bitoffset, j;
    int skip_lines=di->skip_lines;


    if (di->yskip>0)
    {
        prints("\x1b(V\02");
        printc(0); printc(di->yskip%256); printc(di->yskip/256);
    }

    for (i=gstate->pagebbox.ysiz ; i ; i--, data=data+(line_size*2))
    {
        end_data=data+line_size;

        end_even_dots=even_dots;
        end_odd_dots=even_dots;
        emptyflag=1;

        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_data>data && !end_data[-1]) end_data--;

        while (end_data>data)
        {
            databyte=*data++;
            evenbyte=0;
            oddbyte=0;
            bitoffset=7;
            if (databyte)
            {
                for (j=7; j>0 ; j-=2)
                {
                    if (databyte&2^j)       evenbyte+=2^bitoffset;
                    if (databyte&2^(j-1))   oddbyte +=2^bitoffset;
                    bitoffset--;
                }
            }

            databyte=*data++;
            if (databyte)
            {
                for (j=7; j>0 ; j-=2)
                {
                    if (databyte&2^j)       evenbyte+=2^bitoffset;
                    if (databyte&2^(j-1))   oddbyte +=2^bitoffset;
                    bitoffset--;
                }
            }
            *end_even_dots++=evenbyte;
            *end_odd_dots++=oddbyte;
        }

        /* Deal with the even pixels first */

        pixcount=(end_even_dots-even_dots)*8;
        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_even_dots>even_dots && !end_even_dots[-1]) end_even_dots--, pixcount-=8;

        if (end_even_dots!=even_dots)
        {
            emptyflag=0;
            /* move to next printable line down */
            if (skip_lines)
            {
                prints("\x1b(v\02");
                printc(0); printc(skip_lines%256); printc(skip_lines/256);
            }
            skip_lines=0;

            /* set horizontal start position */
            if (di->xskip>0)
            {
                prints("\x1b$");
                printc(di->xskip%256); printc(di->xskip/256);
            }
            
            /* compress the line */
            out_len=CompressMode2(even_dots, end_even_dots, out_data);

            /* set raster graphics mode */
            prints("\x1b.\1\5");
            printc(5); printc(1);
            printc(pixcount%256);
            printc(pixcount/256);

            printb (out_data,out_len);

            /* Return to left hand margin */
            printc(13);

        }
        /* Now to deal with the odd pixels */

        pixcount=(end_odd_dots-odd_dots)*8;
        /* remove trailing zeros - printing "nothing" is useless ! */
        while (end_odd_dots>odd_dots && !end_odd_dots[-1]) end_odd_dots--, pixcount-=8;

        if (end_odd_dots!=odd_dots)
        {
            emptyflag=0;
            /* move to next printable line down */
            if (skip_lines)
            {
                prints("\x1b(v\02");
                printc(0); printc(skip_lines%256); printc(skip_lines/256);
            }
            skip_lines=0;

            /* set horizontal start position */
            if (di->xskip>0)
            {
                prints("\x1b$");
                printc(di->xskip%256); printc(di->xskip/256);
            }

            /* We now need to move the start of the line across 1/1440 inches */
            prints("\x1b(\5c");
            printc(4); printc(0); printc(-96);
            printc(5); printc(1); printc(0);
            
            /* compress the line */
            out_len=CompressMode2(odd_dots, end_odd_dots, out_data);

            /* set raster graphics mode */
            prints("\x1b.\1\5");
            printc(5); printc(1);
            printc(pixcount%256);
            printc(pixcount/256);

            printb (out_data,out_len);

            /* Return to left hand margin */
            printc(13);

        }

        /* We can now move onto the next line */
        if (emptyflag)
        {
            skip_lines++;
        }
        else
        {
            printc(10);
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

    /* eject page and end raster graphics */
    prints("\x1b(i\1\0\48");    /* Turn off any microweave */

    /* v1.02 - changed to include carriage return - reset printer now at end */
    prints("x0d\x0c");          /* carriage return and form feed */

    /* v1.02 - formerly only changed loading to bin 1 (ESC,EM,49). */
    prints("\x1b\x19R");        /* Eject paper in printer */

    prints("\x1b@");            /* Reset printer */

    return ERR_OK;
}

/* mode 2 compression routine or TIFF (rev. 4.0) encoding */
/* variation on run length encoding */
/* theoretically, the output can be at most twice as long as the input */
/* however - the CompressMode2 routine produces an output with maximum */
/* length size+size/127+1 */

static int CompressMode2(char *line, char *end, char *output)
{
    char *org_output=output;
    char *run_start;
    char previous;
    char identical=FALSE;
    int counter;

    while (line<end)
    {
        run_start=line;
        if (identical)
        {
            previous=*line++;
            while (line<end && *line==previous) line++;
            for (counter=line-run_start ; counter>128 ; counter-=128)
            {
                *output++=-127;
                *output++=previous;
            }
            if (counter>1)
            {
                *output++=-counter+1;
                *output++=previous;
            }
            else
                line--;
            identical=FALSE;
        }
        else
        {
            /* need three identical bytes for compression to stay optimal */
            /* I could also request another identical byte, this would */
            /*  - make longer runs - might be more efficient for the printer */
            /*    but then again, sending data to the printer is so slow */
            /*  - longer runs makes it easier to get a break because you can */
            /*    run out of counter bits, therefore, the compression can */
            /*    be BETTER by only going for three identical bytes */

            while (line+2<end && (line[1]!=line[0] || line[2]!=line[0])) line++;

            if (line+2>=end) line=end;

            for (counter=line-run_start ; counter>128 ; counter-=128)
            {
                *output++=127;
                MEMCopy(128,run_start,output);
                output+=128;
                run_start+=128;
            }
            if (counter)
            {
                *output++=counter-1;
                MEMCopy(counter,run_start,output);
                output+=counter;
            }
            identical=TRUE;
        }
    }
    return output-org_output;
}

/* mode 3 compression routine or delta row compression */
/* similar in spirit to mode2 compression, but instead of bytes */
/* identical to the previous (horizontal), it uses bytes identical to */
/* the byte just above */
/* the output can have a maximum length of size+size/8+1 */
/* However, for Epson printers this is different - we need to include */
/* all of the control characters as well as the compression in the ouput */
/* before we send it to the printer */

static CompressMode3(int size, char *line, char *prev, char *output, char *temp_data)
{
    char *org_output=output;
    char *run_start;
    int counter,diff_bytes;
    int orig_MOVX=MOVX-(16+2);

    while (size)
    {
        /* skip identical bytes */
        run_start=line;
        while (*line==*prev && size)
        {
            line++;
            prev++;
            size--;
        }

        counter=line-run_start;

        /* Wherever there is a byte which is same as the byte above, simply use MOVX
        in the output string to ignore these bytes!! */

        if (counter && size)
        {
            if (counter<8)
            {
                *output++=(orig_MOVX+counter);
            }
            else
            {
                if (counter<128)
                {
                    *output++=(orig_MOVX+16+1);
                    *output++=(counter);
                }
                else
                {
                    *output++=(orig_MOVX+16+2);
                    *output++=(counter%256);
                    *output++=(counter/256);
                }
            }
        }
        /* Now copy across the bytes which do not match - we need to use
        TIFF compression on these bytes first! */

        run_start=line;
        while (*line!=*prev && size)
        {
            line++;
            prev++;
            size--;
        }

        counter=line-run_start;
        if (counter)
        {
            diff_bytes=CompressMode2(run_start, line, temp_data);
            if (diff_bytes)
            {
                if (diff_bytes<16)
                {
                    *output++=(XFER+diff_bytes);
                }
                else
                {
                    if (diff_bytes<256)
                    {
                        *output++=(XFER+16+1);
                        *output++=(diff_bytes);
                    }
                    else
                    {
                        *output++=(XFER+16+2);
                        *output++=(diff_bytes%256);
                        *output++=(diff_bytes/256);
                    }
                }
                MEMCopy(diff_bytes,temp_data,output);
                output+=diff_bytes;
            }
        }
    }
    return output-org_output;
}
