1 REMark Create Module for C68 Library
2 REMark (c) Rich Mellor 2000
3 REMark v1.00
4 :
50 CLS
60 PRINT 'Creating SROFF file..'
70 DELETE ram1_temp
100 EX win1_asm_qmac_qmac;'win1_c68_LIB_ESCP2Module_asm -list ram1_temp'
104 PRINT 'Press a Key to Proceed...'
105 PAUSE
107 PRINT 'Creating Module...'
108 PROG_USE win1_c68_
109 DATA_USE win1_pws_pf_
110 EX win1_c68_utils_slb;'-v -o -c -r win1_C68_LIB_libESCP2Module_a win1_C68_LIB_ESCP2Module_rel'
120 QUIT
