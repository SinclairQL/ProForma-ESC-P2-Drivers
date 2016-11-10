/*
    PROforma, PROGS Font & Raster Manager

    general header file for ESC/P2 printer drivers

    (c) PROGS, Professional & Graphical Software
    written by Joachim Van der Auwera
    started 02 November 1995, based on pcl_h

    Updated by Rich Mellor 14/10/99 to include binary commands
    for enhanced compression modes

    Changes 7/11/00 to allow 1440 dpi
*/

#include "PFmodule.h"

/* Define binary drawing commands supported by ESC/P2 */
#define MOVXDOT    -27      /* Size to move x by is specified as one dot (229) */
/* binary 1110 0101         */
#define MOVXBYTE   -28      /* Size to move x by is specified as bytes (228) */
/* binary 1110 0100         */
#define MOVX        82      /* Amount to move x by relative */
/* binary 0101 0010         */
#define MOVY        96      /* Amount to move y by relative */
/* binary 0110 0000         */
#define P_COLR     -128     /* Set colour command (128) */
/* binary 1000 0000         */
#define CLR        -31      /* Clear buffer for Delta Row (225) */
/* binary 1110 0001         */
#define EXIT       -29      /* Exit compression mode (227) */
/* binary 1110 0011         */
#define XFER        32      /* Transfer bytes to printer */
/* binary 0010 0000         */
#define CR         -30      /* Carriage Return (226) */
/* binary 1110 0010         */

/* Set up some constants for the routines */
#define RELATIVE   1
#define ABSOLUTE   0

typedef struct _DriverInfo {

    Channel channel;        /* the output channel for this driver */
    int compression;        /* current compression method */
    int compression2,compression3,compression4;
    int skip_lines;         /* lines to skip before printing next data */
    int skip_yellow;        /* Lines of each colour plane to skip */
    int skip_cyan;
    int skip_magenta;
    int skip_black;

    int xskip, yskip;       /* skip values to end up at the topleft */
    char *out_line;         /* place to compress a byteline */
    char *prev_line;        /* Store last line in each pass */
    char *temp_line;        /* place to store delta row compression */
    char *odd_dots;         /* storage for odd pixels */
    char *even_dots;        /* storage for even pixels */

} DriverInfo;
