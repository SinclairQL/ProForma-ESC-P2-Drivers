* ESC/P2 Module Routines (c) Rich Mellor 2000
* v1.00 Last Updated 7/11/00
* Contains routines called by ProForma ESC/P2 driver to speed up printing

          section   TEXT

          xdef      _CompressMode3
          xdef      _CompressMode2
          xdef      _SplitPixels

MOVX      equ       64
XFER      equ       32

* A routine to call _CompressMode2 from within another machine code routine as
* opposed to from the C function
* entry : a4=temporary buffer (for output)
*         A1=end line pointer
*         D2=run_start pointer
* return: D0=length of output
*         a4 unchanged - points to start of output

CallCompressMode2
          MOVEM.L   d1-d5/a0-a4,-(SP)             Save context (10 longwords)
          MOVE.L    A1,A2                         Grab pointer to end of run
          MOVE.L    D2,A1                         Grab pointer to start of run
          MOVE.L    a4,A3                         Grab pointer to output buffer
          MOVE.L    A3,D4                         Store original output buffer
          MOVEQ     #0,D0                         Set identical flag to FALSE
          BRA.S     tiff_loop

* Prototype: int CompressMode2(char *line, char *end, char *output);

* mode 2 compression routine or TIFF (rev. 4.0) encoding
* variation on run length encoding
* theoretically, the output can be at most twice as long as the input
* however - the CompressMode2 routine produces an output with maximum
* length size+size/127+1

_CompressMode2
          MOVEM.L   d1-d5/a0-a4,-(SP)             Save context (10 longwords)
          LEA       44(SP),A0                     Pointer on first parameter (10+1 longwords)
          MOVE.L    (a0)+,a1                      Grab current line pointer
          MOVE.L    (a0)+,a2                      Grab end of current line pointer
          MOVE.L    (a0),a3                       Grab pointer to output buffer
          MOVE.L    A3,D4                         Store original output buffer
          MOVEQ     #0,D0                         Set identical flag to FALSE
tiff_loop
          CMPA.L    a1,a2                         Is line<end?
          BLS       end_this_line                 No - end run
          MOVE.L    A1,D2                         Store current line pointer
          TST.W     D0                            Is identical flag set?
          BEQ.S     not_same                      No - we cannot use compression!
          MOVE.B    (A1)+,D1                      Grab current character
do_tiff
          CMPA.L    a1,a2                         Is line<end?
          BLS.S     end_tiff
          CMP.B     (A1),D1                       Is character the same as previous one?
          BNE.S     end_tiff
          ADDQ.L    #1,A1                         Increase line pointer
          BRA.S     do_tiff
end_tiff
          MOVE.L    A1,D3                         Work out length of continuous run
          SUB.L     D2,D3
large_counter
          CMP.L     #128,D3                       If counter>128 do following
          BLS.S     small_counter
          SUB.L     #128,D3
          MOVE.B    #-127,(a3)+                   Output character (-127)
          MOVE.B    D1,(a3)+                      Followed by previous
          BRA.S     large_counter                 This is the max number of characters can be sent in one go
small_counter
          CMPI.L    #1,D3                         Is Counter>1?
          BLS.S     no_multiple                   If not ignore compression
          NEG.L     D3                            Negate counter
          ADDQ.L    #1,D3
          MOVE.B    D3,(a3)+                      Output negated counter
          MOVE.B    D1,(a3)+                      Followed by previous
          BRA.S     sent_compress
no_multiple
          SUBQ.L    #1,A1                         Decrease line pointer
          BRA.S     sent_compress

* need three identical bytes for compression to stay optimal
* I could also request another identical byte, this would
* - make longer runs - might be more efficient for the printer
*   but then again, sending data to the printer is so slow
* - longer runs makes it easier to get a break because you can
*   run out of counter bits, therefore, the compression can
*   be BETTER by only going for three identical bytes

not_same
          LEA.L     2(A1),A4
          CMPA.L    A4,A2                         Is line+2<end?
          BLS.S     end_simple                    Yes so do not do anything else
          MOVE.B    (A1),D3
          CMP.B     1(A1),D3                      Is there 2 identical bytes out of 3?
          BNE.S     more2_bytes                   If 2 identical bytes, end this run
          CMP.B     2(A1),D3
          BEQ.S     end_simple
more2_bytes
          ADDQ.L    #1,A1                         Find the end of dissimilar bytes
          BRA.S     not_same
end_simple
          LEA.L     2(A1),A4
          CMPA.L    A4,A2                         Is line+2>=end?
          BHI.S     not_ended                     No, so check for identical bytes
          MOVE.L    A2,A1                         If it is past the end, make line=end
not_ended
          MOVE.L    A1,D3                         Work out length of run
          SUB.L     D2,D3
          BEQ.S     sent_uncompress               Ignore when no bytes to send out
          MOVE.L    D2,A0                         Grab pointer to run start
large_counter2
          CMP.L     #128,D3                       If counter>128 do following
          BLS.S     small_counter2
          SUB.L     #128,D3
          MOVE.B    #127,(A3)+                    Output character (127)
          MOVEQ     #31,D5                        Print first 128 characters of string
copy_128string
          MOVE.L    (A0)+,(A3)+                   Copy across 32 longwords (128 chars)
          DBF       D5,copy_128string
          BRA.S     large_counter2                This is the max number of characters can be sent in one go
small_counter2
          TST.L     D3                            Is Counter>=1?
          BEQ.S     sent_uncompress               If not ignore compression
          SUBQ.L    #1,D3
          MOVE.B    D3,(a3)+                      Output counter-1
copy_lenstring
          MOVE.B    (a0)+,(a3)+                   Followed by the characters
          DBF       D3,copy_lenstring
sent_uncompress
          MOVEQ     #1,D0                         Set identical flag to TRUE
          BRA       tiff_loop
sent_compress
          MOVEQ     #0,D0                         Set identical flag to FALSE
          BRA       tiff_loop
end_this_line
          MOVE.L    A3,D0
          SUB.L     D4,D0                         Work out the length of the output.
          MOVEM.L   (SP)+,D1-D5/A0-A4             Restore the context.
          RTS

* Prototype: int CompressMode3(int size, char *line, char *prev, char *output, char *temp_data);

* mode 3 compression routine or delta row compression
* similar in spirit to mode2 compression, but instead of bytes
* identical to the previous (horizontal), it uses bytes identical to
* the byte just above
* the output can have a maximum length of size+size/8+1
* However, for Epson printers this is different - we need to include
* all of the control characters as well as the compression in the ouput
* before we send it to the printer

_CompressMode3
          MOVEM.L   d1-d5/a0-a4,-(SP)             Save context (10 longwords)
          LEA       44(SP),A0                     Pointer on first parameter (10+1 longwords)
          MOVE.L    (a0)+,d5                      Grab size
          MOVE.L    (a0)+,a1                      Grab current line pointer
          MOVE.L    (a0)+,a2                      Grab previous line pointer
          MOVE.L    (a0)+,a3                      Grab pointer to output buffer
          MOVE.L    (a0),a4                       Grab pointer to temporary buffer
          MOVE.L    A3,D4                         Store original output buffer
          ADD.L     A1,D5                         Work out end of line pointer
compare_loop
          CMP.L     A1,D5                         Is line pointer past end?
          BLS       end_line                      Yes, exit loop
          MOVE.L    A1,D2                         Store current line pointer
same_char
          MOVE.B    (a2),D1
          CMP.B     (a1),D1                       Is character same as on previous line?
          BNE.S     diff_char                     No, we need to print out
          ADDQ.L    #1,A1
          ADDQ.L    #1,A2
          CMP.L     A1,D5
          BLS       end_line                      Reached end of line?
          BRA.S     same_char
diff_char
          MOVE.L    A1,D1
          SUB.L     D2,D1                         Find out number of repeat characters
          BEQ.S     nxt_char
          CMP.L     A1,D5                         Are we at the end of the line?
          BLS.S     nxt_char

          CMPI.L    #8,D1                         Can we fit movement in a byte?
          BCC.S     check_word
          ADD.B     #MOVX,D1
          MOVE.B    D1,(a3)+                      Send output command (MOVX+counter)
          BRA.S     nxt_char

check_word
          CMP.L     #128,D1                       Can we fit movement in a word?
          BCC.S     check_longword
          MOVE.B    #MOVX+17,(a3)+                Send output command (MOVX+16+1)
          MOVE.B    D1,(a3)+                      Send counter
          BRA.S     nxt_char

check_longword
          MOVE.B    #MOVX+18,(a3)+                Send output command (MOVX+16+2)
          MOVE.B    D1,(A3)+                      Send counter MOD 256
          LSR.L     #8,D1
          MOVE.B    D1,(a3)+                      Send counter DIV 256

nxt_char
          MOVE.L    A1,D2                         Store current line pointer
          CMP.L     A1,D5                         Is line>end
          BLS.S     match_char                    Yes, ignore
alt_char
          MOVE.B    (a2),D1
          CMP.B     (a1),D1                       Is character same as on previous line?
          BEQ.S     match_char                    Yes, we need to print out
          ADDQ.L    #1,A1
          ADDQ.L    #1,A2
          CMP.L     A1,D5                         Is line>end?
          BLS.S     match_char                    If so, end of line reached
          BRA.S     alt_char
match_char
          CMP.L     D2,A1                         Is current line=end pointer?
          BEQ       compare_loop                  Any non matching characters to print?

* Call up TIFF compression mode - we need to set up the stack in reverse order
* D1=CompressMode2(run_start,line,temp_data)
* which gives us D1=CompressMode2(D2.L,A1,a4)

          BSR       CallCompressMode2             Call sub-routine from inside m/c
          MOVE.L    D0,D1                         Grab the length returned
          BEQ       compare_loop                  Nothing to print!
          CMP.L     #16,D1                        Can we fit characters in a byte?
          BCC.S     print_word
          MOVE.B    D1,D3
          ADD.B     #XFER,D3
          MOVE.B    D3,(a3)+                      Send output command (XFER+counter)
          BRA.S     print_bytes

print_word
          CMP.L     #256,D1                       Can we fit movement in a word?
          BCC.S     print_longword
          MOVE.B    #XFER+17,(a3)+                Send output command (XFER+16+1)
          MOVE.B    D1,(a3)+                      Send counter
          BRA.S     print_bytes

print_longword
          MOVE.B    #XFER+18,(a3)+                Send output command (XFER+16+2)
          MOVE.B    D1,(A3)+                      Send counter MOD 256
          MOVE.L    D1,D3
          LSR.L     #8,D3
          MOVE.B    D3,(a3)+                      Send counter DIV 256
print_bytes
          MOVE.L    a4,A0                         Grab pointer to temporary buffer
          SUBQ.L    #1,D1                         How many characters to print?
print_chars
          MOVE.B    (A0)+,(A3)+                   Copy the characters across from the temporary buffer to the output buffer
          DBF       D1,print_chars
          BRA       compare_loop
end_line
          MOVE.L    A3,D0
          SUB.L     D4,D0                         Work out the length of the output.
          MOVEM.L   (SP)+,D1-D5/A0-a4             Restore the context.
          RTS

* Prototype: int SplitPixels(char *line, char *end, char *odds, char *evens);

* For 1440 dpi printing - take a specified line of pixels
* and slit it into two - line of odd pixels
* and a line of even pixels.
* This is required because the printer can only print a maximum of 720 dpi.
* We therefore need to print all of the odd pixels, then use <CR> and move the
* print head 1/1440" to the right and print all of the even pixels.
* The routine returns the number of odd/even pixels

_SplitPixels
          MOVEM.L   d1-d3/a0-a5,-(SP)             Save context (9 longwords)
          LEA       40(SP),A0                     Pointer on first parameter (9+1 longwords)
          MOVE.L    (a0)+,a1                      Grab current line pointer
          MOVE.L    (a0)+,a2                      Grab end of current line pointer
          MOVE.L    (a0)+,a3                      Grab pointer to odd pixel store
          MOVE.L    (a0),a4                       Grab pointer to even pixel store
          MOVE.L    a3,a5                         Store pointer to odd pixels
split_loop
          CMPA.L    A1,A2                         Have we reached the end of the line?
          BLS       end_split
          MOVEQ     #0,D2
          MOVEQ     #0,D3
          MOVE.B    (A1)+,D1                      Grab a two bytes (16 pixels) at a time
          BEQ.S     TEST_O5
          BTST      #7,D1                         We now work through the bits of the
          BEQ.S     TEST_E1                        word (one bit = one pixel) to see whether
          BSET      #7,D2                          we need to set corresponding bit in
TEST_E1   BTST      #6,D1                         either the odd or even line of pixels
          BEQ.S     TEST_O2
          BSET      #7,D3
TEST_O2   BTST      #5,D1
          BEQ.S     TEST_E2
          BSET      #6,D2
TEST_E2   BTST      #4,D1
          BEQ.S     TEST_O3
          BSET      #6,D3
TEST_O3   BTST      #3,D1
          BEQ.S     TEST_E3
          BSET      #5,D2
TEST_E3   BTST      #2,D1
          BEQ.S     TEST_O4
          BSET      #5,D3
TEST_O4   BTST      #1,D1
          BEQ.S     TEST_E4
          BSET      #4,D2
TEST_E4   BTST      #0,D1
          BEQ.S     TEST_O5
          BSET      #4,D3
TEST_O5   CMPA.L    A1,A2                         We need to test for the end of the line
          BLS.S     end_split                     After splitting the first byte of (A1)
          MOVE.B    (A1)+,D1
          BTST      #7,D1
          BEQ.S     TEST_E5
          BSET      #3,D2
TEST_E5   BTST      #6,D1
          BEQ.S     TEST_O6
          BSET      #3,D3
TEST_O6   BTST      #5,D1
          BEQ.S     TEST_E6
          BSET      #2,D2
TEST_E6   BTST      #4,D1
          BEQ.S     TEST_O7
          BSET      #2,D3
TEST_O7   BTST      #3,D1
          BEQ.S     TEST_E7
          BSET      #1,D2
TEST_E7   BTST      #2,D1
          BEQ.S     TEST_O8
          BSET      #1,D3
TEST_O8   BTST      #1,D1
          BEQ.S     TEST_E8
          BSET      #0,D2
TEST_E8   BTST      #0,D1
          BEQ.S     split_end
          BSET      #0,D3
split_end
          MOVE.B    D3,(a4)+                      Store 8 even pixels
          MOVE.B    D2,(a3)+                      Store 8 odd pixels
          BRA       split_loop                    Move onto next 16 pixels.
end_split
          MOVE.L    A3,D0                         Work out number of odd pixels
          SUB.L     A5,D0
          MOVEM.L   (SP)+,d1-d3/a0-a5             Restore context (9 longwords)
          RTS

          end
