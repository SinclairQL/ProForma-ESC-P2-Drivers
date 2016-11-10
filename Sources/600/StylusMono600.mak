# ProWeSs make file for Stylus Monochrome 600 drivers
# (c) Rich Mellor 1999-2000

# LRUN win1_pws_pf_boot
# before trying to Execute this file.

DRIVER_PARTS = driv_Stylusrouts_o driv_escp2mono600_o driv_StylusMono600_o core-dll_o
DRIVER_NAME  = driv_StylusMono600_pfd

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
