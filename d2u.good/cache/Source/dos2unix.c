/* // Line 1
 *  Name: dos2unix // Line 2
 *  Documentation: // Line 3
 *    Remove cr ('\x0d') characters from a file. // Line 4
 * // Line 5
 *  The dos2unix package is distributed under FreeBSD style license. // Line 6
 *  See also https://www.freebsd.org/copyright/freebsd-license.html // Line 7
 *  -------- // Line 8
 * // Line 9
 *  Copyright (C) 2009-2024 Erwin Waterlander // Line 10
 *  Copyright (C) 1998 Christian Wurll // Line 11
 *  Copyright (C) 1998 Bernd Johannes Wuebben // Line 12
 *  Copyright (C) 1994-1995 Benjamin Lin. // Line 13
 *  All rights reserved. // Line 14
 * // Line 15
 *  Redistribution and use in source and binary forms, with or without // Line 16
 *  modification, are permitted provided that the following conditions // Line 17
 *  are met: // Line 18
 *  1. Redistributions of source code must retain the above copyright // Line 19
 *     notice, this list of conditions and the following disclaimer. // Line 20
 *  2. Redistributions in binary form must reproduce the above copyright // Line 21
 *     notice in the documentation and/or other materials provided with // Line 22
 *     the distribution. // Line 23
 * // Line 24
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY // Line 25
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE // Line 26
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR // Line 27
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE // Line 28
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR // Line 29
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT // Line 30
 *  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR // Line 31
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, // Line 32
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE // Line 33
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN // Line 34
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. // Line 35
 * // Line 36
 *  == 1.0 == 1989.10.04 == John Birchfield (jb@koko.csustan.edu) // Line 37
 *  == 1.1 == 1994.12.20 == Benjamin Lin (blin@socs.uts.edu.au) // Line 38
 *     Cleaned up for Borland C/C++ 4.02 // Line 39
 *  == 1.2 == 1995.03.16 == Benjamin Lin (blin@socs.uts.edu.au) // Line 40
 *     Modified to more conform to UNIX style. // Line 41
 *  == 2.0 == 1995.03.19 == Benjamin Lin (blin@socs.uts.edu.au) // Line 42
 *     Rewritten from scratch. // Line 43
 *  == 2.1 == 1995.03.29 == Benjamin Lin (blin@socs.uts.edu.au) // Line 44
 *     Conversion to SunOS charset implemented. // Line 45
 *  == 2.2 == 1995.03.30 == Benjamin Lin (blin@socs.uts.edu.au) // Line 46
 *     Fixed a bug in 2.1 where in new-file mode, if outfile already exists // Line 47
 *     conversion can not be completed properly. // Line 48
 * // Line 49
 * Added Mac text file translation, i.e. \r to \n conversion // Line 50
 * Bernd Johannes Wuebben, wuebben@kde.org // Line 51
 * Wed Feb  4 19:12:58 EST 1998 // Line 52
 * // Line 53
 * Added extra newline if ^M occurs // Line 54
 * Christian Wurll, wurll@ira.uka.de // Line 55
 * Thu Nov 19 1998 // Line 56
 * // Line 57
 *  See ChangeLog.txt for complete version history. // Line 58
 * // Line 59
 */ // Line 60


/* #define DEBUG 1 */ // Line 63
#define __DOS2UNIX_C

#include "common.h"
#include "dos2unix.h"
# if (defined(_WIN32) && !defined(__CYGWIN__))
#include <windows.h>
#endif
#ifdef D2U_UNICODE
#if !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__)  /* Unix, Cygwin */
# include <langinfo.h>
#endif
#endif

void PrintLicense(void) // Line 77
{ // Line 78
  D2U_ANSI_FPRINTF(stdout,_("\
Copyright (C) 2009-%d Erwin Waterlander\n\
Copyright (C) 1998      Christian Wurll (Version 3.1)\n\
Copyright (C) 1998      Bernd Johannes Wuebben (Version 3.0)\n\
Copyright (C) 1994-1995 Benjamin Lin\n\
All rights reserved.\n\n"),2024); // Line 84
  PrintBSDLicense(); // Line 85
} // Line 86

#ifdef D2U_UNICODE
wint_t StripDelimiterW(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, wint_t CurChar, unsigned int *converted, const char *progname) // Line 89
{ // Line 90
  wint_t TempNextChar; // Line 91
  /* CurChar is always CR (x0d) */ // Line 92
  /* In normal dos2unix mode put nothing (skip CR). */ // Line 93
  /* Don't modify Mac files when in dos2unix mode. */ // Line 94
  if ( (TempNextChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 95
    if (d2u_ungetwc( TempNextChar, ipInF, ipFlag->bomtype) == WEOF) {  /* put back peek char */ // Line 96
        d2u_getc_error(ipFlag,progname); // Line 97
        return WEOF; // Line 98
    } // Line 99
    if ( TempNextChar != 0x0a ) { // Line 100
      if (d2u_putwc(CurChar, ipOutF, ipFlag, progname) == WEOF) {  /* Mac line, put CR */ // Line 101
          d2u_putwc_error(ipFlag,progname); // Line 102
          return WEOF; // Line 103
      } // Line 104
    } else { // Line 105
      (*converted)++; // Line 106
      if (ipFlag->NewLine) {  /* add additional LF? */ // Line 107
        if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 108
            d2u_putwc_error(ipFlag,progname); // Line 109
            return WEOF; // Line 110
        } // Line 111
      } // Line 112
    } // Line 113
  } else { // Line 114
    if (ferror(ipInF)) { // Line 115
        d2u_getc_error(ipFlag,progname); // Line 116
        return WEOF; // Line 117
    } // Line 118
    if ( CurChar == 0x0d ) {  /* EOF: last Mac line delimiter (CR)? */ // Line 119
        if (d2u_putwc(CurChar, ipOutF, ipFlag, progname) == WEOF) { // Line 120
            d2u_putwc_error(ipFlag,progname); // Line 121
            return WEOF; // Line 122
        } // Line 123
    } // Line 124
  } // Line 125
  return CurChar; // Line 126
} // Line 127
#endif

/* CUR        NEXT // Line 130
   0xd(CR)    0xa(LF)  => put LF if option -l was used // Line 131
   0xd(CR)  ! 0xa(LF)  => put CR // Line 132
   0xd(CR)    EOF      => put CR // Line 133
 */ // Line 134
int StripDelimiter(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, int CurChar, unsigned int *converted, const char *progname) // Line 135
{ // Line 136
  int TempNextChar; // Line 137
  /* CurChar is always CR (x0d) */ // Line 138
  /* In normal dos2unix mode put nothing (skip CR). */ // Line 139
  /* Don't modify Mac files when in dos2unix mode. */ // Line 140
  if ( (TempNextChar = fgetc(ipInF)) != EOF) { // Line 141
    if (ungetc( TempNextChar, ipInF ) == EOF) { /* put back peek char */ // Line 142
        d2u_getc_error(ipFlag,progname); // Line 143
        return EOF; // Line 144
    } // Line 145
    if ( TempNextChar != '\x0a' ) { // Line 146
      if (fputc( CurChar, ipOutF ) == EOF) { /* Mac line, put CR */ // Line 147
          d2u_putc_error(ipFlag,progname); // Line 148
          return EOF; // Line 149
      } // Line 150
    } else { // Line 151
      (*converted)++; // Line 152
      if (ipFlag->NewLine) {  /* add additional LF? */ // Line 153
        if (fputc('\x0a', ipOutF) == EOF) { // Line 154
            d2u_putc_error(ipFlag,progname); // Line 155
            return EOF; // Line 156
        } // Line 157
      } // Line 158
    } // Line 159
  } else { // Line 160
    if (ferror(ipInF)) { // Line 161
        d2u_getc_error(ipFlag,progname); // Line 162
        return EOF; // Line 163
    } // Line 164
    if ( CurChar == '\x0d' ) {  /* EOF: last Mac line delimiter (CR)? */ // Line 165
        if (fputc( CurChar, ipOutF ) == EOF) { // Line 166
            d2u_putc_error(ipFlag,progname); // Line 167
            return EOF; // Line 168
        } // Line 169
    } // Line 170
  } // Line 171
  return CurChar; // Line 172
} // Line 173

/* converts stream ipInF to UNIX format text and write to stream ipOutF // Line 175
 * RetVal: 0  if success // Line 176
 *         -1  otherwise // Line 177
 */ // Line 178
#ifdef D2U_UNICODE
int ConvertDosToUnixW(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, const char *progname) // Line 180
{ // Line 181
    int RetVal = 0; // Line 182
    wint_t PrevChar = WEOF; // Line 183
    wint_t TempChar; // Line 184
    wint_t TempNextChar; // Line 185
    unsigned int line_nr = 1; // Line 186
    unsigned int converted = 0; // Line 187

    ipFlag->status = 0; // Line 189

    /* CR-LF -> LF */ // Line 191
    /* LF    -> LF, in case the input file is a Unix text file */ // Line 192
    /* CR    -> CR, in dos2unix mode (don't modify Mac file) */ // Line 193
    /* CR    -> LF, in Mac mode */ // Line 194
    /* \x0a = Newline/Line Feed (LF) */ // Line 195
    /* \x0d = Carriage Return (CR) */ // Line 196

    switch (ipFlag->FromToMode) // Line 198
    { // Line 199
      case FROMTO_DOS2UNIX: /* dos2unix */ // Line 200
        while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) {  /* get character */ // Line 201
          if ((ipFlag->Force == 0) && // Line 202
              (TempChar < 32) && // Line 203
              (TempChar != 0x0a) &&  /* Not an LF */ // Line 204
              (TempChar != 0x0d) &&  /* Not a CR */ // Line 205
              (TempChar != 0x09) &&  /* Not a TAB */ // Line 206
              (TempChar != 0x0c)) {  /* Not a form feed */ // Line 207
            RetVal = -1; // Line 208
            ipFlag->status |= BINARY_FILE ; // Line 209
            if (ipFlag->verbose) { // Line 210
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 211
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 212
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x00%02X found at line %u\n"),TempChar, line_nr); // Line 213
            } // Line 214
            break; // Line 215
          } // Line 216
          if (TempChar != 0x0d) { // Line 217
            if (TempChar == 0x0a) /* Count all DOS and Unix line breaks */ // Line 218
              ++line_nr; // Line 219
            if (d2u_putwc(TempChar, ipOutF, ipFlag, progname) == WEOF) { // Line 220
              RetVal = -1; // Line 221
              d2u_putwc_error(ipFlag,progname); // Line 222
              break; // Line 223
            } // Line 224
          } else { // Line 225
            if (StripDelimiterW( ipInF, ipOutF, ipFlag, TempChar, &converted, progname) == WEOF) { // Line 226
              RetVal = -1; // Line 227
              break; // Line 228
            } // Line 229
          } // Line 230
          PrevChar = TempChar; // Line 231
        } // Line 232
        if (TempChar == WEOF && ipFlag->add_eol && PrevChar != WEOF && PrevChar != 0x0a) { // Line 233
          /* Add missing line break at the last line. */ // Line 234
            if (ipFlag->verbose > 1) { // Line 235
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 236
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 237
            } // Line 238
            if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 239
              RetVal = -1; // Line 240
              d2u_putwc_error(ipFlag,progname); // Line 241
            } // Line 242
        } // Line 243
        if ((TempChar == WEOF) && ferror(ipInF)) { // Line 244
          RetVal = -1; // Line 245
          d2u_getc_error(ipFlag,progname); // Line 246
        } // Line 247
        break; // Line 248
      case FROMTO_MAC2UNIX: /* mac2unix */ // Line 249
        while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 250
          if ((ipFlag->Force == 0) && // Line 251
              (TempChar < 32) && // Line 252
              (TempChar != 0x0a) &&  /* Not an LF */ // Line 253
              (TempChar != 0x0d) &&  /* Not a CR */ // Line 254
              (TempChar != 0x09) &&  /* Not a TAB */ // Line 255
              (TempChar != 0x0c)) {  /* Not a form feed */ // Line 256
            RetVal = -1; // Line 257
            ipFlag->status |= BINARY_FILE ; // Line 258
            if (ipFlag->verbose) { // Line 259
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 260
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 261
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x00%02X found at line %u\n"), TempChar, line_nr); // Line 262
            } // Line 263
            break; // Line 264
          } // Line 265
          if ((TempChar != 0x0d)) { // Line 266
            if (TempChar == 0x0a) /* Count all DOS and Unix line breaks */ // Line 267
              ++line_nr; // Line 268
            if(d2u_putwc(TempChar, ipOutF, ipFlag, progname) == WEOF) { // Line 269
              RetVal = -1; // Line 270
              d2u_putwc_error(ipFlag,progname); // Line 271
              break; // Line 272
            } // Line 273
          } else{ // Line 274
            /* TempChar is a CR */ // Line 275
            if ( (TempNextChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 276
              if (d2u_ungetwc( TempNextChar, ipInF, ipFlag->bomtype) == WEOF) {  /* put back peek char */ // Line 277
                d2u_getc_error(ipFlag,progname); // Line 278
                RetVal = -1; // Line 279
                break; // Line 280
              } // Line 281
              /* Don't touch this delimiter if it's a CR,LF pair. */ // Line 282
              if ( TempNextChar == 0x0a ) { // Line 283
                if (d2u_putwc(0x0d, ipOutF, ipFlag, progname) == WEOF) { /* put CR, part of DOS CR-LF */ // Line 284
                  d2u_putwc_error(ipFlag,progname); // Line 285
                  RetVal = -1; // Line 286
                  break; // Line 287
                } // Line 288
                PrevChar = TempChar; // Line 289
                continue; // Line 290
              } // Line 291
            } // Line 292
            if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { /* MAC line end (CR). Put LF */ // Line 293
              RetVal = -1; // Line 294
              d2u_putwc_error(ipFlag,progname); // Line 295
              break; // Line 296
            } // Line 297
            converted++; // Line 298
            line_nr++; /* Count all Mac line breaks */ // Line 299
            if (ipFlag->NewLine) {  /* add additional LF? */ // Line 300
              if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 301
                RetVal = -1; // Line 302
                d2u_putwc_error(ipFlag,progname); // Line 303
                break; // Line 304
              } // Line 305
            } // Line 306
          } // Line 307
          PrevChar = TempChar; // Line 308
        } // Line 309
        if (TempChar == WEOF && ipFlag->add_eol && PrevChar != WEOF && !(PrevChar == 0x0a || PrevChar == 0x0d)) { // Line 310
          /* Add missing line break at the last line. */ // Line 311
            if (ipFlag->verbose > 1) { // Line 312
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 313
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 314
            } // Line 315
            if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 316
              RetVal = -1; // Line 317
              d2u_putwc_error(ipFlag,progname); // Line 318
            } // Line 319
        } // Line 320
        if ((TempChar == WEOF) && ferror(ipInF)) { // Line 321
          RetVal = -1; // Line 322
          d2u_getc_error(ipFlag,progname); // Line 323
        } // Line 324
        break; // Line 325
      default: /* unknown FromToMode */ // Line 326
      ; // Line 327
#if DEBUG
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 329
      D2U_UTF8_FPRINTF(stderr, _("program error, invalid conversion mode %d\n"),ipFlag->FromToMode); // Line 330
      exit(1); // Line 331
#endif
    } // Line 333
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) // Line 334
        ipFlag->line_nr = line_nr; // Line 335
    if ((RetVal == 0) && (ipFlag->verbose > 1)) { // Line 336
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 337
      D2U_UTF8_FPRINTF(stderr, _("Converted %u out of %u line breaks.\n"), converted, line_nr -1); // Line 338
    } // Line 339
    return RetVal; // Line 340
} // Line 341
#endif

/* converts stream ipInF to UNIX format text and write to stream ipOutF // Line 344
 * RetVal: 0  if success // Line 345
 *         -1  otherwise // Line 346
 */ // Line 347
int ConvertDosToUnix(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, const char *progname) // Line 348
{ // Line 349
    int RetVal = 0; // Line 350
    int PrevChar = EOF; // Line 351
    int TempChar; // Line 352
    int TempNextChar; // Line 353
    int *ConvTable; // Line 354
    unsigned int line_nr = 1; // Line 355
    unsigned int converted = 0; // Line 356

    ipFlag->status = 0; // Line 358

    switch (ipFlag->ConvMode) { // Line 360
      case CONVMODE_ASCII: /* ascii */ // Line 361
      case CONVMODE_UTF16LE: /* Assume UTF-16LE, bomtype = FILE_UTF8 or GB18030 */ // Line 362
      case CONVMODE_UTF16BE: /* Assume UTF-16BE, bomtype = FILE_UTF8 or GB18030 */ // Line 363
        ConvTable = D2UAsciiTable; // Line 364
        break; // Line 365
      case CONVMODE_7BIT: /* 7bit */ // Line 366
        ConvTable = D2U7BitTable; // Line 367
        break; // Line 368
      case CONVMODE_437: /* iso */ // Line 369
        ConvTable = D2UIso437Table; // Line 370
        break; // Line 371
      case CONVMODE_850: /* iso */ // Line 372
        ConvTable = D2UIso850Table; // Line 373
        break; // Line 374
      case CONVMODE_860: /* iso */ // Line 375
        ConvTable = D2UIso860Table; // Line 376
        break; // Line 377
      case CONVMODE_863: /* iso */ // Line 378
        ConvTable = D2UIso863Table; // Line 379
        break; // Line 380
      case CONVMODE_865: /* iso */ // Line 381
        ConvTable = D2UIso865Table; // Line 382
        break; // Line 383
      case CONVMODE_1252: /* iso */ // Line 384
        ConvTable = D2UIso1252Table; // Line 385
        break; // Line 386
      default: /* unknown convmode */ // Line 387
        ipFlag->status |= WRONG_CODEPAGE ; // Line 388
        return(-1); // Line 389
    } // Line 390
    /* Turn off ISO and 7-bit conversion for Unicode text files */ // Line 391
    if (ipFlag->bomtype > 0) // Line 392
      ConvTable = D2UAsciiTable; // Line 393

    if ((ipFlag->ConvMode > CONVMODE_7BIT) && (ipFlag->verbose)) { /* not ascii or 7bit */ // Line 395
       D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 396
       D2U_UTF8_FPRINTF(stderr, _("using code page %d.\n"), ipFlag->ConvMode); // Line 397
    } // Line 398

    /* CR-LF -> LF */ // Line 400
    /* LF    -> LF, in case the input file is a Unix text file */ // Line 401
    /* CR    -> CR, in dos2unix mode (don't modify Mac file) */ // Line 402
    /* CR    -> LF, in Mac mode */ // Line 403
    /* \x0a = Newline/Line Feed (LF) */ // Line 404
    /* \x0d = Carriage Return (CR) */ // Line 405

    switch (ipFlag->FromToMode) { // Line 407
      case FROMTO_DOS2UNIX: /* dos2unix */ // Line 408
        while ((TempChar = fgetc(ipInF)) != EOF) {  /* get character */ // Line 409
          if ((ipFlag->Force == 0) && // Line 410
              (TempChar < 32) && // Line 411
              (TempChar != '\x0a') &&  /* Not an LF */ // Line 412
              (TempChar != '\x0d') &&  /* Not a CR */ // Line 413
              (TempChar != '\x09') &&  /* Not a TAB */ // Line 414
              (TempChar != '\x0c')) {  /* Not a form feed */ // Line 415
            RetVal = -1; // Line 416
            ipFlag->status |= BINARY_FILE ; // Line 417
            if (ipFlag->verbose) { // Line 418
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 419
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 420
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x%02X found at line %u\n"),TempChar, line_nr); // Line 421
            } // Line 422
            break; // Line 423
          } // Line 424
          if (TempChar != '\x0d') { // Line 425
            if (TempChar == '\x0a') /* Count all DOS and Unix line breaks */ // Line 426
              ++line_nr; // Line 427
            if (fputc(ConvTable[TempChar], ipOutF) == EOF) { // Line 428
              RetVal = -1; // Line 429
              d2u_putc_error(ipFlag,progname); // Line 430
              break; // Line 431
            } // Line 432
          } else { // Line 433
            if (StripDelimiter( ipInF, ipOutF, ipFlag, TempChar, &converted, progname) == EOF) { // Line 434
              RetVal = -1; // Line 435
              break; // Line 436
            } // Line 437
          } // Line 438
          PrevChar = TempChar; // Line 439
        } // Line 440
        if (TempChar == EOF && ipFlag->add_eol && PrevChar != EOF && PrevChar != '\x0a') { // Line 441
          /* Add missing line break at the last line. */ // Line 442
            if (ipFlag->verbose > 1) { // Line 443
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 444
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 445
            } // Line 446
           if (fputc('\x0a', ipOutF) == EOF) { // Line 447
              RetVal = -1; // Line 448
              d2u_putc_error(ipFlag,progname); // Line 449
            } // Line 450
        } // Line 451
        if ((TempChar == EOF) && ferror(ipInF)) { // Line 452
          RetVal = -1; // Line 453
          d2u_getc_error(ipFlag,progname); // Line 454
        } // Line 455
        break; // Line 456
      case FROMTO_MAC2UNIX: /* mac2unix */ // Line 457
        while ((TempChar = fgetc(ipInF)) != EOF) { // Line 458
          if ((ipFlag->Force == 0) && // Line 459
              (TempChar < 32) && // Line 460
              (TempChar != '\x0a') &&  /* Not an LF */ // Line 461
              (TempChar != '\x0d') &&  /* Not a CR */ // Line 462
              (TempChar != '\x09') &&  /* Not a TAB */ // Line 463
              (TempChar != '\x0c')) {  /* Not a form feed */ // Line 464
            RetVal = -1; // Line 465
            ipFlag->status |= BINARY_FILE ; // Line 466
            if (ipFlag->verbose) { // Line 467
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 468
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 469
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x%02X found at line %u\n"),TempChar, line_nr); // Line 470
            } // Line 471
            break; // Line 472
          } // Line 473
          if ((TempChar != '\x0d')) { // Line 474
            if (TempChar == '\x0a') /* Count all DOS and Unix line breaks */ // Line 475
              ++line_nr; // Line 476
            if(fputc(ConvTable[TempChar], ipOutF) == EOF) { // Line 477
              RetVal = -1; // Line 478
              d2u_putc_error(ipFlag,progname); // Line 479
              break; // Line 480
            } // Line 481
          } else{ // Line 482
            /* TempChar is a CR */ // Line 483
            if ( (TempNextChar = fgetc(ipInF)) != EOF) { // Line 484
              if (ungetc( TempNextChar, ipInF ) == EOF) {  /* put back peek char */ // Line 485
                d2u_getc_error(ipFlag,progname); // Line 486
                RetVal = -1; // Line 487
                break; // Line 488
              } // Line 489
              /* Don't touch this delimiter if it's a CR,LF pair. */ // Line 490
              if ( TempNextChar == '\x0a' ) { // Line 491
                if (fputc('\x0d', ipOutF) == EOF) { /* put CR, part of DOS CR-LF */ // Line 492
                  RetVal = -1; // Line 493
                  d2u_putc_error(ipFlag,progname); // Line 494
                  break; // Line 495
                } // Line 496
                PrevChar = TempChar; // Line 497
                continue; // Line 498
              } // Line 499
            } // Line 500
            if (fputc('\x0a', ipOutF) == EOF) { /* MAC line end (CR). Put LF */ // Line 501
              RetVal = -1; // Line 502
              d2u_putc_error(ipFlag,progname); // Line 503
              break; // Line 504
            } // Line 505
            converted++; // Line 506
            line_nr++; /* Count all Mac line breaks */ // Line 507
            if (ipFlag->NewLine) {  /* add additional LF? */ // Line 508
              if (fputc('\x0a', ipOutF) == EOF) { // Line 509
                RetVal = -1; // Line 510
                d2u_putc_error(ipFlag,progname); // Line 511
                break; // Line 512
              } // Line 513
            } // Line 514
          } // Line 515
          PrevChar = TempChar; // Line 516
        } // Line 517
        if (TempChar == EOF && ipFlag->add_eol && PrevChar != EOF && !(PrevChar == '\x0a' || PrevChar == '\x0d')) { // Line 518
          /* Add missing line break at the last line. */ // Line 519
            if (ipFlag->verbose > 1) { // Line 520
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 521
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 522
            } // Line 523
            if (fputc('\x0a', ipOutF) == EOF) { // Line 524
              RetVal = -1; // Line 525
              d2u_putc_error(ipFlag,progname); // Line 526
            } // Line 527
        } // Line 528
        if ((TempChar == EOF) && ferror(ipInF)) { // Line 529
          RetVal = -1; // Line 530
          d2u_getc_error(ipFlag,progname); // Line 531
        } // Line 532
        break; // Line 533
      default: /* unknown FromToMode */ // Line 534
      ; // Line 535
#if DEBUG
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 537
      D2U_UTF8_FPRINTF(stderr, _("program error, invalid conversion mode %d\n"),ipFlag->FromToMode); // Line 538
      exit(1); // Line 539
#endif
    } // Line 541
    if ((RetVal == 0) && (ipFlag->verbose > 1)) { // Line 542
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 543
      D2U_UTF8_FPRINTF(stderr, _("Converted %u out of %u line breaks.\n"),converted, line_nr -1); // Line 544
    } // Line 545
    return RetVal; // Line 546
} // Line 547


int main (int argc, char *argv[]) // Line 550
{ // Line 551
  /* variable declarations */ // Line 552
  char progname[9]; // Line 553
  CFlag *pFlag; // Line 554
  char *ptr; // Line 555
  char localedir[1024]; // Line 556
  int ret; // Line 557
# ifdef __MINGW64__
  int _dowildcard = -1; /* enable wildcard expansion for Win64 */ // Line 559
# endif
  int  argc_new; // Line 561
  char **argv_new; // Line 562
#ifdef D2U_UNIFILE
  wchar_t **wargv; // Line 564
  char ***argv_glob; // Line 565
# endif

  progname[8] = '\0'; // Line 568
  strcpy(progname,"dos2unix"); // Line 569

#ifdef ENABLE_NLS
   ptr = getenv("DOS2UNIX_LOCALEDIR"); // Line 572
   if (ptr == NULL) // Line 573
      d2u_strncpy(localedir,LOCALEDIR,sizeof(localedir)); // Line 574
   else { // Line 575
      if (strlen(ptr) < sizeof(localedir)) // Line 576
         d2u_strncpy(localedir,ptr,sizeof(localedir)); // Line 577
      else { // Line 578
         D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 579
         D2U_ANSI_FPRINTF(stderr, "%s", _("error: Value of environment variable DOS2UNIX_LOCALEDIR is too long.\n")); // Line 580
         d2u_strncpy(localedir,LOCALEDIR,sizeof(localedir)); // Line 581
      } // Line 582
   } // Line 583
#endif

#if defined(ENABLE_NLS) || (defined(D2U_UNICODE) && !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__))
/* setlocale() is also needed for nl_langinfo() */ // Line 587
#if (defined(_WIN32) && !defined(__CYGWIN__))
/* When the locale is set to "" on Windows all East-Asian multi-byte ANSI encoded text is printed // Line 589
   wrongly when you use standard printf(). Also UTF-8 code is printed wrongly. See also test/setlocale.c. // Line 590
   When we set the locale to "C" gettext still translates the messages on Windows. On Unix this would disable // Line 591
   gettext. */ // Line 592
   setlocale (LC_ALL, "C"); // Line 593
#else
   setlocale (LC_ALL, ""); // Line 595
#endif
#endif

#ifdef ENABLE_NLS
   bindtextdomain (PACKAGE, localedir); // Line 600
   textdomain (PACKAGE); // Line 601
#endif


  /* variable initialisations */ // Line 605
  pFlag = (CFlag*)malloc(sizeof(CFlag)); // Line 606
  if (pFlag == NULL) { // Line 607
    D2U_UTF8_FPRINTF(stderr, "dos2unix:"); // Line 608
    D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno)); // Line 609
    return errno; // Line 610
  } // Line 611
  pFlag->FromToMode = FROMTO_DOS2UNIX;  /* default dos2unix */ // Line 612
  pFlag->keep_bom = 0; // Line 613

  if ( ((ptr=strrchr(argv[0],'/')) == NULL) && ((ptr=strrchr(argv[0],'\\')) == NULL) ) // Line 615
    ptr = argv[0]; // Line 616
  else // Line 617
    ptr++; // Line 618

  if ((strcmpi("mac2unix", ptr) == 0) || (strcmpi("mac2unix.exe", ptr) == 0)) { // Line 620
    pFlag->FromToMode = FROMTO_MAC2UNIX; // Line 621
    strcpy(progname,"mac2unix"); // Line 622
  } // Line 623

#ifdef D2U_UNIFILE
  /* Get arguments in wide Unicode format in the Windows Command Prompt */ // Line 626

  /* This does not support wildcard expansion (globbing) */ // Line 628
  wargv = CommandLineToArgvW(GetCommandLineW(), &argc); // Line 629

  argv_glob = (char ***)malloc(sizeof(char***)); // Line 631
  if (argv_glob == NULL) { // Line 632
    D2U_UTF8_FPRINTF(stderr, "%s:", progname); // Line 633
    D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno)); // Line 634
    free(pFlag); // Line 635
    return errno; // Line 636
  } // Line 637
  /* Glob the arguments and convert them to UTF-8 */ // Line 638
  argc_new = glob_warg(argc, wargv, argv_glob, pFlag, progname); // Line 639
  argv_new = *argv_glob; // Line 640
#else  
  argc_new = argc; // Line 642
  argv_new = argv; // Line 643
#endif

#ifdef D2U_UNICODE
  ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense, ConvertDosToUnix, ConvertDosToUnixW); // Line 647
#else
  ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense, ConvertDosToUnix); // Line 649
#endif
  free(pFlag); // Line 651
  return ret; // Line 652
} // Line 653
