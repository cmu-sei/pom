/* querycp.c is in the public domain */ // Line 1

#if (defined(__WATCOMC__) && defined(__NT__))
#  define _WIN32 1
#endif

#ifdef __DJGPP__

#include <dpmi.h>
#include <go32.h>
#include <stdio.h>

/* // Line 13
 ---------------------------------------------------------------------- // Line 14
 Tuesday, May 5, 2009    1:40pm // Line 15
 rugxulo _AT_ gmail _DOT_ com // Line 16

 This file is (obviously?) not copyrighted, "nenies proprajxo" !! // Line 18

 Tested successfully on DR-DOS 7.03, FreeDOS 1.0++, and MS-DOS 6.22. // Line 20
 (Doesn't work on XP or Vista, though.) // Line 21
 ---------------------------------------------------------------------- // Line 22

 unsigned short query_con_codepage(void); // Line 24

 gets currently selected display CON codepage // Line 26

 int 21h, 6601h ("chcp") needs NLSFUNC.EXE + COUNTRY.SYS, but many // Line 28
    obscure codepages (e.g. FD's cp853 from EGA.CPX (CPIX.ZIP) or // Line 29
    Kosta Kostis' cp913 from ISOLATIN.CPI (ISOCP101.ZIP) have no // Line 30
    relevant data inside COUNTRY.SYS. // Line 31

 int 21h, 440Ch 6Ah only works in MS-DOS and DR-DOS (not FreeDOS) because // Line 33
    FreeDOS DISPLAY is an .EXE TSR, not a device driver, and hence doesn't // Line 34
    fully support IOCTL, so they use the undocumented int 2Fh, 0AD02h // Line 35
    (which doesn't work in DR-DOS!). But DR-DOS' DISPLAY doesn't respond // Line 36
    to the typical install check i.d. anyways. FreeDOS currently only // Line 37
    supports COUNTRY.SYS in their "unstable" kernel 2037, but at least // Line 38
    their KEYB, "gxoje", supports cp853 too (thanks, Henrique!). // Line 39

 P.S. For MS or DR: ren ega.cpx *.com ; upx -d ega.com ; ren ega.com *.cpi // Line 41

 ADDENDUM (2011): // Line 43
 Latest "stable" FreeDOS kernel is 2040, it now includes COUNTRY.SYS // Line 44
 support by default, but NLSFUNC (CHCP) 'system code page' support is // Line 45
 partially unimplemented (lacking some int 2Fh calls, yet Eduardo // Line 46
 Casino didn't seem too worried, so I dunno, nag him if necessary, // Line 47
 heh). // Line 48
 ---------------------------------------------------------------------- // Line 49
*/ // Line 50

unsigned short query_con_codepage(void) { // Line 52
   __dpmi_regs regs; // Line 53

   unsigned short param_block[2] = { 0, 437 }; // Line 55

   regs.d.eax = 0x440C;                /* GENERIC IO FOR HANDLES */ // Line 57
   regs.d.ebx = 1;                     /* STDOUT */ // Line 58
   regs.d.ecx = 0x036A;                /* 3 = CON, 0x6A = QUERY SELECTED CP */ // Line 59
   regs.x.ds = __tb >> 4;              /* using transfer buffer for low mem. */ // Line 60
   regs.x.dx = __tb & 0x0F;            /* (suggested by DJGPP FAQ, hi Eli!) */ // Line 61
   regs.x.flags |= 1;                  /* preset carry for potential failure */ // Line 62
   __dpmi_int (0x21, &regs); // Line 63

   if (!(regs.x.flags & 1))            /* if succeed (carry flag not set) */ // Line 65
     dosmemget( __tb, 4, param_block); // Line 66
   else {                              /* (undocumented method) */ // Line 67
     regs.x.ax = 0xAD02;               /* 440C -> MS-DOS or DR-DOS only */ // Line 68
     regs.x.bx = 0xFFFE;               /* AD02 -> MS-DOS or FreeDOS only */ // Line 69
     regs.x.flags |= 1; // Line 70
     __dpmi_int(0x2F, &regs); // Line 71

     if ((!(regs.x.flags & 1)) && (regs.x.bx < 0xFFFE)) // Line 73
       param_block[1] = regs.x.bx; // Line 74
   } // Line 75

   return param_block[1]; // Line 77
} // Line 78
#elif defined(__WATCOMC__) && defined(__I86__) /* Watcom C, 16 bit Intel */

/* rugxulo _AT_ gmail _DOT_ com */ // Line 81

#include <stdio.h>
#include <dos.h>
#include <i86.h>

unsigned short query_con_codepage(void) { // Line 87
   union REGS regs; // Line 88
   unsigned short param_block[2] = { 0, 437 }; // Line 89

   regs.x.ax = 0x440C;           /* GENERIC IO FOR HANDLES */ // Line 91
   regs.x.bx = 1;                /* STDOUT */ // Line 92
   regs.x.cx = 0x036A;           /* 3 = CON, 0x6A = QUERY SELECTED CP */ // Line 93
   regs.x.dx = (unsigned short)param_block; // Line 94
   regs.x.cflag |= 1;            /* preset carry for potential failure */ // Line 95
   int86(0x21, &regs, &regs); // Line 96

   if (regs.x.cflag)             /* if not succeed (carry flag set) */ // Line 98
   { // Line 99
     regs.x.ax = 0xAD02;         /* 440C -> MS-DOS or DR-DOS only */ // Line 100
     regs.x.bx = 0xFFFE;         /* AD02 -> MS-DOS or FreeDOS only */ // Line 101
     regs.x.cflag |= 1; // Line 102
     int86(0x2F, &regs, &regs); // Line 103
   } // Line 104

     if ((!(regs.x.cflag)) && (regs.x.bx < 0xFFFE)) // Line 106
       param_block[1] = regs.x.bx; // Line 107

   return param_block[1]; // Line 109

} // Line 111

#elif defined(__WATCOMC__) && defined(__DOS__) /* Watcom C, 32 bit DOS */

/* rugxulo _AT_ gmail _DOT_ com */ // Line 115

#include <stdio.h>
#include <dos.h>
#include <i86.h>

unsigned short query_con_codepage(void) { // Line 121
   union REGS regs; // Line 122
   unsigned short param_block[2] = { 0, 437 }; // Line 123

   regs.x.eax = 0x440C;           /* GENERIC IO FOR HANDLES */ // Line 125
   regs.x.ebx = 1;                /* STDOUT */ // Line 126
   regs.x.ecx = 0x036A;           /* 3 = CON, 0x6A = QUERY SELECTED CP */ // Line 127
   regs.x.edx = (unsigned short)param_block; // Line 128
   regs.x.cflag |= 1;             /* preset carry for potential failure */ // Line 129
   int386(0x21, &regs, &regs); // Line 130

   if (regs.x.cflag)              /* if not succeed (carry flag set) */ // Line 132
   { // Line 133
     regs.x.eax = 0xAD02;         /* 440C -> MS-DOS or DR-DOS only */ // Line 134
     regs.x.ebx = 0xFFFE;         /* AD02 -> MS-DOS or FreeDOS only */ // Line 135
     regs.x.cflag |= 1; // Line 136
     int386(0x2F, &regs, &regs); // Line 137
   } // Line 138

     if ((!(regs.x.cflag)) && (regs.x.ebx < 0xFFFE)) // Line 140
       param_block[1] = regs.x.ebx; // Line 141

   return param_block[1]; // Line 143

} // Line 145


#elif defined (_WIN32) && !defined(__CYGWIN__) /* Windows, not Cygwin */

/* Erwin Waterlander */ // Line 150

#include <windows.h>
unsigned short query_con_codepage(void) { // Line 153

  /* Dos2unix is modelled after dos2unix under SunOS/Solaris. // Line 155
   * The original dos2unix ISO mode on SunOS supported code // Line 156
   * pages CP437, CP850, CP860, CP863, and CP865, which // Line 157
   * are DOS code pages. Therefore we request here the DOS // Line 158
   * code page of the Console. The DOS code page is used // Line 159
   * by DOS programs, for instance text editor 'edit'. // Line 160
   */ // Line 161

  /* Get the console's DOS code page */ // Line 163
   return((unsigned short)GetConsoleOutputCP()); // Line 164

   /* Get the system's ANSI code page */ // Line 166
   /* return((unsigned short)GetACP()); */ // Line 167

} // Line 169

#elif defined (__OS2__) /* OS/2 Warp */

#define INCL_DOS
#include <os2.h>

unsigned short query_con_codepage(void) { // Line 176
  ULONG cp[3]; // Line 177
  ULONG cplen; // Line 178

  DosQueryCp(sizeof(cp), cp, &cplen); // Line 180
  return((unsigned short)cp[0]); // Line 181
} // Line 182

#else  /* Unix, other */
unsigned short query_con_codepage(void) { // Line 185
   return(0); // Line 186
} // Line 187
#endif

#ifdef TEST
int main() { // Line 191
  printf("\nCP%u\n",query_con_codepage() );  /* should be same result as */ // Line 192
  return 0;                                  /*   "mode con cp /status" */ // Line 193
} // Line 194
#endif

