#ifndef _ESCP2module_H
#define _ESCP2module_H
/*
    Define references for m/c routines called by ESC/P2 drivers (ESCP2Module_a)

    (c) Rich Mellor 1/11/2000
*/

int CompressMode2 (char * line,char *end,char *output);
int CompressMode3 (int size,char * line,char *prev,char *output,char *temp_data);
int SplitPixels(char *data,char *end_data,char *odd_dots,char *even_dots);

#endif  /* _ESCP2module_H */
