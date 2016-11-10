                Epson (ESC/P2) Drivers for ProWesS (and ProForma)
                -------------------------------------------------

These drivers are all copyright RWAP Software 1999-2000, released as freeware as part of 
the 30th Anniversary of RWAP Software.  

They are based on the original drivers provided by PROGS with their ProWeSs package for the Sinclair QL.  

However, they have been much improved to allow 720 dpi printing and more speed.  In particular the
binary TIFF compression mode is now used to ensure that the minimum time possible
is spent sending control codes to the printer.

These drivers should work on all Epson ESC/P2 drivers, although we understand that
the Epson Stylus 200 does not support the binary commands used in these drivers.

There are three different drivers supplied:

Stylus_pfd              A mono printer driver
StylusColour_pfd        A colour printer driver
StylusColour2_pfd       A colour replacement printer driver for LineDesign
StylusMono600_pfd       A mono printer driver for the Epson Stylus 600
StylusColourA5_pfd      A second copy of StylusColour2_pfd

Under ProForma (the print program supplied with ProWesS, programs can use these
drivers to output in all the colours supplied by your printer (unless the mono
printer driver is used).  However, LineDesign does not currently support colour
(except on bitmaps).

To overcome this, colour replacement drivers are supplied which change some of
the greyshades output by LineDesign to create colours.

ColourSample_ldp is a sample LineDesign file which allows you to see the various
colours and their values under the colour replacement drivers.  Remember that 0
is white and 100 is black.  Anything other than the values listed will produce a
grey shade based on the percentage of black.

StylusColourA5_pfd is supplied with the drivers renamed to:

Epson Stylus Colour 360dpi colrepA5 (etc)

- this is so that you can have two versions of the colour replacement driver
loaded at the same time, for example for different paper sizes.

INSTALLING THE DRIVERS
----------------------
If you have already installed Stylus Colour Drivers on your system, you will
need to replace the existing _pfd files with those supplied.  These are normally
installed in the sub-directory win1_pws_pf_driv_

If you have not done this before, then copy the files to this sub-directory.

You should then amend the file PROforma_cfg - normally in the sub-directory
win1_pws_mine_

You can amend this in a text editor or selecting the Configure ProWesS option
from the UTILITIES button.  Select PROforma_cfg as the file to configure.

Once you have loaded PROforma_cfg, look for the line which says:

; load drivers (mixed screen, printer, bitmap & picture)


and then add the following three lines to this section (if not already existing):

D StylusColour_pfd
D StylusColour2_pfd
D Stylus_pfd

Each line will ensure that PROForma loads the relevent printer driver on startup.

You will need to reset your system for the new settings to take effect.

IDEAL PAPER SIZES
-----------------
If you have found out the best settings for your printer, please let us know.

These are the paper sizes found so far (based on 180 dpi)

Epson Stylus 850 Colour
-----------------------
A4 -    Paper Width     204.04mm
        Paper Height    274.46mm
        Left Margin     2.68mm
        Top Margin      8.61mm


Updates
-------
v1.00 First Release version
v1.01 ColourSample_ldp added
v1.02 Fast Monochrome mode added to the Mono drivers (this causes an error on the
      Epson Stylus 600 and possibly others, hence the separate driver)
      Improved Compression techniques used for 720dpi (Delta Row Compression)
      Generally increased speed.
v1.03 Corrected a bug in the 180 and 360 dpi colour printer drivers which meant
      that pages could get corrupted.
v1.04 Flush Buffers command added for later series Epson printers to ensure that
      end of page is printed.
      Memory allocation re-done to ensure that information is not corrupted
      between passes when using Delta Row compression.
      Machine code compression routines added to speed up transfer of data
v1.05 Updated the code for switching off microweave - for some reason PRINTS was
      adding an extra character
