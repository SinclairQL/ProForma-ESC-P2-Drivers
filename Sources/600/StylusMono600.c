/*
    PROforma, PROgs FOnt & Raster MAnager

    Epson Slylus 600 - aka ESC/P2 mono printer driver
    This printer reports an error if try to enter Fast Monochrome mode

    (c) PROGS, Professional & Graphical Software
    written by Joachim Van der Auwera
    started 2 November 1995

    Updated by Rich Mellor (RWAP Software)   16/10/99
    Updated to include Delta Row Compression 27/9/00
    Updated 29/12/01 to allow Flush Buffers
*/

#include "PFmodule.h"

static Error nimp()     { return ERR_NIMP; }
static Error nothing()  { return ERR_OK; }

Error InitDriver();
Error Define(char *name, char *value);

Error InitInit(Gstate, char *device, PBbox *area);
Error BufferSize(Gstate, int xsiz, int ysiz, Size *size);
Error InitDone(Gstate);
Error CleanUp(Gstate);

Error InitPage(Gstate);
Error ShowPage(Gstate);
Error StopPage(Gstate);

Error SetCopies(Gstate, int count);
Error SetDisplayMode(Gstate, int mode);
Error WindowMove(Gstate, int dx, int dy);
Error BufferUpdate(Gstate);

Error ColourAdjust(Gstate, pt *);
Error ColourNearest(Gstate, pt *, int *);
Error ColourDelta(Gstate, pt *, pt *);

Error GetFullPageBbox(Gstate, Bbox *box);
Error DefaultDevice(Gstate, char *device);

#define InitDriver      nothing
#define WindowMove      nothing
#define ColourAdjust    nothing
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
    "Epson Stylus 600 mono v1.05 180dpi", /* name */
    180,180,                    /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        1, 1,                   /* 1 plane, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_NO_PLANES,       /* no planes */
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

    PF_COLOURSPACE_GRAYSHADE,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};

DRIVdriver dpi360={
    &dpi180, PF_PRINTERDRIVER,
    "Epson Stylus 600 mono v1.05 360dpi", /* name */
    360,360,                    /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        1, 1,                   /* 1 plane, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_NO_PLANES,       /* no planes */
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

    PF_COLOURSPACE_GRAYSHADE,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};

/* init indicates the last driver */

DRIVdriver init={
    &dpi360, PF_PRINTERDRIVER,
    "Epson Stylus 600 mono v1.05 720dpi", /* name */
    720,720,                    /* xdpi, ydpi */
    {                           /* mode 4 bitmap */
        1, 1,                   /* 1 plane, 1 pixel deep */
        BITMAP_NO_BYTEGROUPING, /* no byte grouping */
        BITMAP_NO_PLANES,       /* no planes */
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

    PF_COLOURSPACE_GRAYSHADE,
    {
        ColourAdjust,
        ColourNearest,
        ColourDelta
    },

    GetFullPageBbox,
    DefaultDevice,

    Handle
};
