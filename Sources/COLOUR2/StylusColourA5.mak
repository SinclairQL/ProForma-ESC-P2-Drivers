# ProWeSs make file for Stylus Colour A5 replacement drivers
# (c) Rich Mellor 1999-2002

# LRUN win1_pws_pf_boot
# before trying to Execute this file.

DRIVER_PARTS = driv_escp2colour_o driv_StylusColourA5_o core-dll_o
DRIVER_NAME  = driv_StylusColourA5_pfd

# Directories:

T = ram1_
P = win1_C68_

# Program names:

CC  = cc
AS  = as68
ASM = qmac
LD  = ld

# Program flags:

CCFLAGS  = -tmp$(T) -v -warn=3 -O -Y$(P) -I$(P)include_
ASFLAGS  = -V
ASMFLAGS = -nolist
LDFLAGS  = -v -L$(P)lib_ -bufp100K\

$(DRIVER_NAME) : $(DRIVER_PARTS)
        $(LD) -ms -o$(DRIVER_NAME) $(DRIVER_PARTS) -lsms -lESCP2Module -sxmod
        mkxmod $(DRIVER_NAME) \"PROforma external driver\"

# Construction rules:

_c_o :
    $(CC) -c $(CCFLAGS) $<

_s_o :
    $(AS) $(ASFLAGS) $< $@

_asm_rel :
    $(ASM) $< $(ASMFLAGS)

#end
