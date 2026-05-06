/* // Line 1
 *   Copyright (C) 2009-2024 Erwin Waterlander // Line 2
 *   All rights reserved. // Line 3
 * // Line 4
 *   Redistribution and use in source and binary forms, with or without // Line 5
 *   modification, are permitted provided that the following conditions // Line 6
 *   are met: // Line 7
 *   1. Redistributions of source code must retain the above copyright // Line 8
 *      notice, this list of conditions and the following disclaimer. // Line 9
 *   2. Redistributions in binary form must reproduce the above copyright // Line 10
 *      notice in the documentation and/or other materials provided with // Line 11
 *      the distribution. // Line 12
 * // Line 13
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY // Line 14
 *   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE // Line 15
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR // Line 16
 *   PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE // Line 17
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR // Line 18
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT // Line 19
 *   OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR // Line 20
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, // Line 21
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE // Line 22
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN // Line 23
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. // Line 24
 */ // Line 25

#include "common.h"
#include "dos2unix.h"
#include "querycp.h"

#include <stdarg.h>

#if defined(D2U_UNIFILE) || (defined(D2U_UNICODE) && defined(_WIN32))
#include <windows.h>
#endif

#if defined(D2U_UNICODE) && !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__)  /* Unix, Cygwin */
# include <langinfo.h>
#endif

#if defined(__GLIBC__)
/* on glibc, canonicalize_file_name() broken prior to 2.4 (06-Mar-2006) */ // Line 42
# if __GNUC_PREREQ (2,4)
#  define USE_CANONICALIZE_FILE_NAME 1
# endif
#elif defined(__CYGWIN__)
/* on cygwin, canonicalize_file_name() available since api 0/213 */ // Line 47
/* (1.7.0beta61, 25-Sep-09) */ // Line 48
# include <cygwin/version.h>
# if (CYGWIN_VERSION_DLL_COMBINED >= 213) && (CYGWIN_VERSION_DLL_MAJOR >= 1007)
#  define USE_CANONICALIZE_FILE_NAME 1
# endif
#endif

/* global variables */ // Line 55
#ifdef D2U_UNIFILE
int d2u_display_encoding = D2U_DISPLAY_ANSI ; // Line 57
#endif

/* Copy string src to dest, and null terminate dest. // Line 60
   dest_size must be the buffer size of dest. */ // Line 61
char *d2u_strncpy(char *dest, const char *src, size_t dest_size) // Line 62
{ // Line 63
    strncpy(dest,src,dest_size); // Line 64
    dest[dest_size-1] = '\0'; // Line 65
#ifdef DEBUG
    if(strlen(src) > (dest_size-1)) { // Line 67
        D2U_UTF8_FPRINTF(stderr, "Text %s has been truncated from %d to %d characters in %s to prevent a buffer overflow.\n", src, (int)strlen(src), (int)dest_size, "d2u_strncpy()"); // Line 68
    } // Line 69
#endif
    return dest; // Line 71
} // Line 72

int d2u_fclose (FILE *fp, const char *filename, CFlag *ipFlag, const char *m, const char *progname) // Line 74
{ // Line 75
  if (fclose(fp) != 0) { // Line 76
    if (ipFlag->verbose) { // Line 77
      ipFlag->error = errno; // Line 78
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 79
      if (m[0] == 'w') // Line 80
        D2U_UTF8_FPRINTF(stderr, _("Failed to write to temporary output file %s:"), filename); // Line 81
      else // Line 82
        D2U_UTF8_FPRINTF(stderr, _("Failed to close input file %s:"), filename); // Line 83
      D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno)); // Line 84
    } // Line 85
    return EOF; // Line 86
  } // Line 87
#if DEBUG
  else // Line 89
     fprintf(stderr, "%s: Closing file \"%s\" OK.\n", progname, filename); // Line 90
#endif
  return 0; // Line 92
} // Line 93


/* // Line 96
 * Print last system error on Windows. // Line 97
 * // Line 98
 */ // Line 99
#if (defined(_WIN32) && !defined(__CYGWIN__))
void d2u_PrintLastError(const char *progname) // Line 101
{ // Line 102
    /* Retrieve the system error message for the last-error code */ // Line 103

    LPVOID lpMsgBuf; // Line 105
    DWORD dw; // Line 106

    dw = GetLastError(); // Line 108

    FormatMessage( // Line 110
        FORMAT_MESSAGE_ALLOCATE_BUFFER | // Line 111
        FORMAT_MESSAGE_FROM_SYSTEM | // Line 112
        FORMAT_MESSAGE_IGNORE_INSERTS, // Line 113
        NULL, // Line 114
        dw, // Line 115
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Line 116
        (LPTSTR) &lpMsgBuf, // Line 117
        0, NULL ); // Line 118

    /* Display the error message */ // Line 120

    /* MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK); */ // Line 122
    D2U_UTF8_FPRINTF(stderr, "%s: ",progname); // Line 123
#ifdef _UNICODE
    fwprintf(stderr, L"%ls\n",(LPCTSTR)lpMsgBuf); // Line 125
#else
    fprintf(stderr, "%s\n",(LPCTSTR)lpMsgBuf); // Line 127
#endif

    LocalFree(lpMsgBuf); // Line 130
} // Line 131


int d2u_WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar) // Line 134
{ // Line 135
  int i; // Line 136

  if ( (i = WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar)) == 0) // Line 138
      d2u_PrintLastError("dos2unix"); // Line 139

  return i; // Line 141
} // Line 142

int d2u_MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar) // Line 144
{ // Line 145
  int i; // Line 146

  if ( (i = MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar)) == 0) // Line 148
      d2u_PrintLastError("dos2unix"); // Line 149
  return i; // Line 150
} // Line 151

#endif

#ifdef D2U_UNIFILE
/* // Line 156
 * d2u_utf8_fprintf()  : printf wrapper, print in Windows Command Prompt in Unicode // Line 157
 * mode, to have consistent output. Regardless of active code page. // Line 158
 * // Line 159
 * On Windows the file system uses always Unicode UTF-16 encoding, regardless // Line 160
 * of the system default code page. This means that files and directories can // Line 161
 * have names that can't be encoded in the default system Windows ANSI code // Line 162
 * page. // Line 163
 * // Line 164
 * Dos2unix for Windows with Unicode file name support translates all directory // Line 165
 * names to UTF-8, to be able to  work with char type strings.  This is also // Line 166
 * done to keep the code portable. // Line 167
 * // Line 168
 * Dos2unix's messages are encoded in the default Windows ANSI code page, which // Line 169
 * can be translated with gettext. Gettext/libintl recodes messages (format) to // Line 170
 * the system default ANSI code page. // Line 171
 * // Line 172
 * d2u_utf8_fprintf() on Windows assumes that: // Line 173
 * - The format string is encoded in the system default ANSI code page. // Line 174
 * - The arguments are encoded in UTF-8. // Line 175
 * // Line 176
 * There are several methods for printing Unicode in the Windows Console, but // Line 177
 * none of them is perfect. There are so many issues that I decided to go back // Line 178
 * to ANSI by default. // Line 179
 * // Line 180
 * The use of setlocale() has influence on this function when ANSI or UTF-8 is // Line 181
 * printed. See also dos2unix.c and unix2dos.c and test/setlocale.c and // Line 182
 * test/setlocale.png. // Line 183
 */ // Line 184

void d2u_utf8_fprintf( FILE *stream, const char* format, ... ) { // Line 186
   va_list args; // Line 187
   char buf[D2U_MAX_PATH]; // Line 188
   char formatmbs[D2U_MAX_PATH]; // Line 189
   wchar_t formatwcs[D2U_MAX_PATH]; // Line 190
   UINT outputCP; // Line 191
   wchar_t wstr[D2U_MAX_PATH]; // Line 192
   int prevmode; // Line 193
   static int BOM_printed = 0; // Line 194

   va_start(args, format); // Line 196

   /* The format string is encoded in the system default // Line 198
    * Windows ANSI code page. May have been translated // Line 199
    * by gettext. Convert it to wide characters. */ // Line 200
   d2u_MultiByteToWideChar(CP_ACP,0, format, -1, formatwcs, D2U_MAX_PATH); // Line 201
   /* then convert the format string to UTF-8 */ // Line 202
   d2u_WideCharToMultiByte(CP_UTF8, 0, formatwcs, -1, formatmbs, D2U_MAX_PATH, NULL, NULL); // Line 203

   /* The arguments (file names) are in UTF-8 encoding, because // Line 205
    * in dos2unix for Windows all file names are in UTF-8 format. // Line 206
    * Print to buffer (UTF-8) */ // Line 207
   vsnprintf(buf, sizeof(buf), formatmbs, args); // Line 208

   if ((d2u_display_encoding == D2U_DISPLAY_UTF8) || (d2u_display_encoding == D2U_DISPLAY_UTF8BOM)) { // Line 210

   /* A disadvantage of this method is that all non-ASCII characters are printed // Line 212
      wrongly when the console uses raster font (which is the default). // Line 213
      When I switch the system ANSI code page to 936 (Simplified Chinese) or 932 (Japanese) // Line 214
      I see lot of flickering in the console when I print UTF-8. // Line 215
      The cause could be that I have a Dutch Windows installation, and when the console is // Line 216
      switched to UTF-8 mode (CP65001) the font is switched back to Western font (Lucida Console, // Line 217
      Consolas). These are the only fonts which I can select when I set the code page in the // Line 218
      console to 65001 with chcp, while the system ANSI code is 936 or 932. // Line 219
   */ // Line 220
       /* print UTF-8 buffer to console in UTF-8 mode */ // Line 221
      outputCP = GetConsoleOutputCP(); // Line 222
      SetConsoleOutputCP(CP_UTF8); // Line 223
      if (! BOM_printed) { // Line 224
          if (d2u_display_encoding == D2U_DISPLAY_UTF8BOM) // Line 225
              fwprintf(stream, L"%S","\xEF\xBB\xBF"); // Line 226
          BOM_printed = 1; // Line 227
      } // Line 228
      fwprintf(stream,L"%S",buf); // Line 229
      fflush(stream); // Line 230
      SetConsoleOutputCP(outputCP); // Line 231

   /* The following UTF-8 method does not give correct output. I don't know why. */ // Line 233
   /*prevmode = _setmode(_fileno(stream), _O_U8TEXT); // Line 234
     fwprintf(stream,L"%S",buf); // Line 235
     fflush(stream); // Line 236
     _setmode(_fileno(stream), prevmode); */ // Line 237

   } else if ((d2u_display_encoding == D2U_DISPLAY_UNICODE) || (d2u_display_encoding == D2U_DISPLAY_UNICODEBOM)) { // Line 239

   /* Printing UTF-16 works correctly. Works also good with raster fonts. // Line 241
      No need to change the OEM code page to the system ANSI code page. // Line 242
    */ // Line 243
      d2u_MultiByteToWideChar(CP_UTF8,0, buf, -1, wstr, D2U_MAX_PATH); // Line 244
      prevmode = _setmode(_fileno(stream), _O_U16TEXT); // Line 245
      if (! BOM_printed) { // Line 246
          /* For correct redirection in PowerShell we need to print a BOM */ // Line 247
          if (d2u_display_encoding == D2U_DISPLAY_UNICODEBOM) // Line 248
              fwprintf(stream, L"\xfeff"); // Line 249
          BOM_printed = 1; // Line 250
      } // Line 251
      fwprintf(stream,L"%ls",wstr); // Line 252
      fflush(stream);  /* Flushing is required to get correct UTF-16 when stdout is redirected. */ // Line 253
      _setmode(_fileno(stream), prevmode); // Line 254

   } else {  /* ANSI */ // Line 256

      d2u_MultiByteToWideChar(CP_UTF8,0, buf, -1, wstr, D2U_MAX_PATH); // Line 258
      /* Convert the whole message to ANSI, some Unicode characters may fail to translate to ANSI. // Line 259
         They will be displayed as a question mark. */ // Line 260
      d2u_WideCharToMultiByte(CP_ACP, 0, wstr, -1, buf, D2U_MAX_PATH, NULL, NULL); // Line 261
      fprintf(stream,"%s",buf); // Line 262
   } // Line 263

   va_end( args ); // Line 265
} // Line 266

/* d2u_ansi_fprintf() // Line 268
   fprintf wrapper for Windows console. // Line 269

   Format and arguments are in ANSI format. // Line 271
   Redirect the printing to d2u_utf8_fprintf such that the output // Line 272
   format is consistent. To prevent a mix of ANSI/UTF-8/UTF-16 // Line 273
   encodings in the print output. Mixed format printing may get the whole // Line 274
   console mixed up. // Line 275
 */ // Line 276

void d2u_ansi_fprintf( FILE *stream, const char* format, ... ) { // Line 278
   va_list args; // Line 279
   char buf[D2U_MAX_PATH];        /* ANSI encoded string */ // Line 280
   char bufmbs[D2U_MAX_PATH];     /* UTF-8 encoded string */ // Line 281
   wchar_t bufwcs[D2U_MAX_PATH];  /* Wide encoded string */ // Line 282

   va_start(args, format); // Line 284

   vsnprintf(buf, sizeof(buf), format, args); // Line 286
   /* The format string and arguments are encoded in the system default // Line 287
    * Windows ANSI code page. May have been translated // Line 288
    * by gettext. Convert it to wide characters. */ // Line 289
   d2u_MultiByteToWideChar(CP_ACP,0, buf, -1, bufwcs, D2U_MAX_PATH); // Line 290
   /* then convert the format string to UTF-8 */ // Line 291
   d2u_WideCharToMultiByte(CP_UTF8, 0, bufwcs, -1, bufmbs, D2U_MAX_PATH, NULL, NULL); // Line 292

   d2u_utf8_fprintf(stream, "%s",bufmbs); // Line 294

   va_end( args ); // Line 296
} // Line 297
#endif

/*   d2u_rename // Line 300
 *   wrapper for rename(). // Line 301
 *   On Windows file names are encoded in UTF-8. // Line 302
 */ // Line 303
int d2u_rename(const char *oldname, const char *newname) // Line 304
{ // Line 305
#ifdef D2U_UNIFILE
   wchar_t oldnamew[D2U_MAX_PATH]; // Line 307
   wchar_t newnamew[D2U_MAX_PATH]; // Line 308
   d2u_MultiByteToWideChar(CP_UTF8, 0, oldname, -1, oldnamew, D2U_MAX_PATH); // Line 309
   d2u_MultiByteToWideChar(CP_UTF8, 0, newname, -1, newnamew, D2U_MAX_PATH); // Line 310
   return _wrename(oldnamew, newnamew); // Line 311
#else
   return rename(oldname, newname); // Line 313
#endif
} // Line 315

/*   d2u_unlink // Line 317
 *   wrapper for unlink(). // Line 318
 *   On Windows file names are encoded in UTF-8. // Line 319
 */ // Line 320
int d2u_unlink(const char *filename) // Line 321
{ // Line 322
#ifdef D2U_UNIFILE
   wchar_t filenamew[D2U_MAX_PATH]; // Line 324
   d2u_MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenamew, D2U_MAX_PATH); // Line 325
   return _wunlink(filenamew); // Line 326
#else
   return unlink(filename); // Line 328
#endif
} // Line 330

/****************************************************************** // Line 332
 * // Line 333
 * int symbolic_link(char *path) // Line 334
 * // Line 335
 * test if *path points to a file that exists and is a symbolic link // Line 336
 * // Line 337
 * returns 1 on success, 0 when it fails. // Line 338
 * // Line 339
 ******************************************************************/ // Line 340

#ifdef D2U_UNIFILE

int symbolic_link(const char *path) // Line 344
{ // Line 345
   DWORD attrs; // Line 346
   wchar_t pathw[D2U_MAX_PATH]; // Line 347

   d2u_MultiByteToWideChar(CP_UTF8, 0, path, -1, pathw, D2U_MAX_PATH); // Line 349
   attrs = GetFileAttributesW(pathw); // Line 350

   if (attrs == INVALID_FILE_ATTRIBUTES) // Line 352
      return(0); // Line 353

   return ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0); // Line 355
} // Line 356

#elif(defined(_WIN32) && !defined(__CYGWIN__))

int symbolic_link(const char *path) // Line 360
{ // Line 361
   DWORD attrs; // Line 362

   attrs = GetFileAttributes(path); // Line 364

   if (attrs == INVALID_FILE_ATTRIBUTES) // Line 366
      return(0); // Line 367

   return ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0); // Line 369
} // Line 370

#else
int symbolic_link(const char *path) // Line 373
{ // Line 374
#ifdef S_ISLNK
   struct stat buf; // Line 376

   if (STAT(path, &buf) == 0) { // Line 378
      if (S_ISLNK(buf.st_mode)) // Line 379
         return(1); // Line 380
   } // Line 381
#endif
   return(0); // Line 383
} // Line 384
#endif

/****************************************************************** // Line 387
 * // Line 388
 * int regfile(char *path, int allowSymlinks) // Line 389
 * // Line 390
 * test if *path points to a regular file (or is a symbolic link, // Line 391
 * if allowSymlinks != 0). // Line 392
 * // Line 393
 * returns 0 on success, -1 when it fails. // Line 394
 * // Line 395
 ******************************************************************/ // Line 396
int regfile(char *path, int allowSymlinks, CFlag *ipFlag, const char *progname) // Line 397
{ // Line 398
#ifdef D2U_UNIFILE
   struct _stat buf; // Line 400
   wchar_t pathw[D2U_MAX_PATH]; // Line 401
#else
   struct stat buf; // Line 403
#endif

#ifdef D2U_UNIFILE
   d2u_MultiByteToWideChar(CP_UTF8, 0, path, -1, pathw, D2U_MAX_PATH); // Line 407
   if (_wstat(pathw, &buf) == 0) { // Line 408
#else
   if (STAT(path, &buf) == 0) { // Line 410
#endif
#if DEBUG
      D2U_UTF8_FPRINTF(stderr, "%s: %s", progname, path); // Line 413
      D2U_UTF8_FPRINTF(stderr, " MODE 0%o ", buf.st_mode); // Line 414
#ifdef S_ISSOCK
      if (S_ISSOCK(buf.st_mode)) // Line 416
         D2U_UTF8_FPRINTF(stderr, " (socket)"); // Line 417
#endif
#ifdef S_ISLNK
      if (S_ISLNK(buf.st_mode)) // Line 420
         D2U_UTF8_FPRINTF(stderr, " (symbolic link)"); // Line 421
#endif
      if (S_ISREG(buf.st_mode)) // Line 423
         D2U_UTF8_FPRINTF(stderr, " (regular file)"); // Line 424
#ifdef S_ISBLK
      if (S_ISBLK(buf.st_mode)) // Line 426
         D2U_UTF8_FPRINTF(stderr, " (block device)"); // Line 427
#endif
      if (S_ISDIR(buf.st_mode)) // Line 429
         D2U_UTF8_FPRINTF(stderr, " (directory)"); // Line 430
      if (S_ISCHR(buf.st_mode)) // Line 431
         D2U_UTF8_FPRINTF(stderr, " (character device)"); // Line 432
      if (S_ISFIFO(buf.st_mode)) // Line 433
         D2U_UTF8_FPRINTF(stderr, " (FIFO)"); // Line 434
      D2U_UTF8_FPRINTF(stderr, "\n"); // Line 435
#endif
      if ((S_ISREG(buf.st_mode)) // Line 437
#ifdef S_ISLNK
          || (S_ISLNK(buf.st_mode) && allowSymlinks) // Line 439
#endif
         ) // Line 441
         return(0); // Line 442
      else // Line 443
         return(-1); // Line 444
   } // Line 445
   else { // Line 446
     if (ipFlag->verbose) { // Line 447
       const char *errstr = strerror(errno); // Line 448
       ipFlag->error = errno; // Line 449
       D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, path); // Line 450
       D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 451
     } // Line 452
     return(-1); // Line 453
   } // Line 454
} // Line 455

/****************************************************************** // Line 457
 * // Line 458
 * int regfile_target(char *path) // Line 459
 * // Line 460
 * test if *path points to a regular file (follow symbolic link) // Line 461
 * // Line 462
 * returns 0 on success, -1 when it fails. // Line 463
 * // Line 464
 ******************************************************************/ // Line 465
int regfile_target(char *path, CFlag *ipFlag, const char *progname) // Line 466
{ // Line 467
#ifdef D2U_UNIFILE
   struct _stat buf; // Line 469
   wchar_t pathw[D2U_MAX_PATH]; // Line 470
#else
   struct stat buf; // Line 472
#endif

#ifdef D2U_UNIFILE
   d2u_MultiByteToWideChar(CP_UTF8, 0, path, -1, pathw, D2U_MAX_PATH); // Line 476
   if (_wstat(pathw, &buf) == 0) { // Line 477
#else
   if (stat(path, &buf) == 0) { // Line 479
#endif
      if (S_ISREG(buf.st_mode)) // Line 481
         return(0); // Line 482
      else // Line 483
         return(-1); // Line 484
   } // Line 485
   else { // Line 486
     if (ipFlag->verbose) { // Line 487
       const char *errstr = strerror(errno); // Line 488
       ipFlag->error = errno; // Line 489
       D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, path); // Line 490
       D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 491
     } // Line 492
     return(-1); // Line 493
   } // Line 494
} // Line 495

/* // Line 497
 *   glob_warg() expands the wide command line arguments. // Line 498
 *   Input  : wide Unicode arguments. // Line 499
 *   Output : argv : expanded arguments in UTF-8 format. // Line 500
 *   Returns: new argc value. // Line 501
 *            -1 when an error occurred. // Line 502
 * // Line 503
 */ // Line 504

#ifdef D2U_UNIFILE
int glob_warg(int argc, wchar_t *wargv[], char ***argv, CFlag *ipFlag, const char *progname) // Line 507
{ // Line 508
  int i; // Line 509
  int argc_glob = 0; // Line 510
  wchar_t *warg; // Line 511
  wchar_t *path; // Line 512
  wchar_t *path_and_filename; // Line 513
  wchar_t *ptr; // Line 514
  char  *arg; // Line 515
  char  **argv_new; // Line 516
  const char *errstr; // Line 517
  size_t len; // Line 518
  int found, add_path; // Line 519
  WIN32_FIND_DATA FindFileData; // Line 520
  HANDLE hFind; // Line 521

  argv_new = (char **)malloc(sizeof(char**)); // Line 523
  if (argv_new == NULL) goto glob_failed; // Line 524

  len = (size_t)d2u_WideCharToMultiByte(CP_UTF8, 0, wargv[0], -1, NULL, 0, NULL, NULL); // Line 526
  arg = (char *)malloc(len); // Line 527
  if (arg == NULL) goto glob_failed; // Line 528
  d2u_WideCharToMultiByte(CP_UTF8, 0, wargv[argc_glob], -1, arg, (int)len, NULL, NULL); // Line 529
  argv_new[argc_glob] = arg; // Line 530

  for (i=1; i<argc; ++i) // Line 532
  { // Line 533
    warg = wargv[i]; // Line 534
    found = 0; // Line 535
    add_path = 0; // Line 536
    /* FindFileData.cFileName has the path stripped off. We need to add it again. */ // Line 537
    path = _wcsdup(warg); // Line 538
    /* replace all back slashes with slashes */ // Line 539
    while ( (ptr = wcschr(path,L'\\')) != NULL) { // Line 540
      *ptr = L'/'; // Line 541
    } // Line 542
    if ( (ptr = wcsrchr(path,L'/')) != NULL) { // Line 543
      ptr++; // Line 544
      *ptr = L'\0'; // Line 545
      add_path = 1; // Line 546
    } // Line 547

    hFind = FindFirstFileW(warg, &FindFileData); // Line 549
    while (hFind != INVALID_HANDLE_VALUE) // Line 550
    { // Line 551
      char **new_argv_new; // Line 552
      len = wcslen(path) + wcslen(FindFileData.cFileName) + 2; // Line 553
      path_and_filename = (wchar_t *)malloc(len*sizeof(wchar_t)); // Line 554
      if (path_and_filename == NULL) goto glob_failed; // Line 555
      if (add_path) { // Line 556
        wcsncpy(path_and_filename, path, wcslen(path)+1); // Line 557
        wcsncat(path_and_filename, FindFileData.cFileName, wcslen(FindFileData.cFileName)+1); // Line 558
      } else { // Line 559
        wcsncpy(path_and_filename, FindFileData.cFileName, wcslen(FindFileData.cFileName)+1); // Line 560
      } // Line 561

      found = 1; // Line 563
      ++argc_glob; // Line 564
      len =(size_t) d2u_WideCharToMultiByte(CP_UTF8, 0, path_and_filename, -1, NULL, 0, NULL, NULL); // Line 565
      arg = (char *)malloc((size_t)len); // Line 566
      if (arg == NULL) goto glob_failed; // Line 567
      d2u_WideCharToMultiByte(CP_UTF8, 0, path_and_filename, -1, arg, (int)len, NULL, NULL); // Line 568
      free(path_and_filename); // Line 569
      new_argv_new = (char **)realloc(argv_new, (size_t)(argc_glob+1)*sizeof(char**)); // Line 570
      if (new_argv_new == NULL) goto glob_failed; // Line 571
      else // Line 572
        argv_new = new_argv_new; // Line 573
      argv_new[argc_glob] = arg; // Line 574

      if (!FindNextFileW(hFind, &FindFileData)) { // Line 576
        FindClose(hFind); // Line 577
        hFind = INVALID_HANDLE_VALUE; // Line 578
      } // Line 579
    } // Line 580
    free(path); // Line 581
    if (found == 0) { // Line 582
    /* Not a file. Just copy the argument */ // Line 583
      char **new_argv_new; // Line 584
      ++argc_glob; // Line 585
      len =(size_t) d2u_WideCharToMultiByte(CP_UTF8, 0, warg, -1, NULL, 0, NULL, NULL); // Line 586
      arg = (char *)malloc((size_t)len); // Line 587
      if (arg == NULL) goto glob_failed; // Line 588
      d2u_WideCharToMultiByte(CP_UTF8, 0, warg, -1, arg, (int)len, NULL, NULL); // Line 589
      new_argv_new = (char **)realloc(argv_new, (size_t)(argc_glob+1)*sizeof(char**)); // Line 590
      if (new_argv_new == NULL) goto glob_failed; // Line 591
      else // Line 592
        argv_new = new_argv_new; // Line 593
      argv_new[argc_glob] = arg; // Line 594
    } // Line 595
  } // Line 596
  *argv = argv_new; // Line 597
  return ++argc_glob; // Line 598

  glob_failed: // Line 600
  if (ipFlag->verbose) { // Line 601
    ipFlag->error = errno; // Line 602
    errstr = strerror(errno); // Line 603
    D2U_UTF8_FPRINTF(stderr, "%s:", progname); // Line 604
    D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 605
  } // Line 606
  return -1; // Line 607
} // Line 608
#endif

void PrintBSDLicense(void) // Line 611
{ // Line 612
  D2U_ANSI_FPRINTF(stdout,"%s", _("\
Redistribution and use in source and binary forms, with or without\n\
modification, are permitted provided that the following conditions\n\
are met:\n\
1. Redistributions of source code must retain the above copyright\n\
   notice, this list of conditions and the following disclaimer.\n\
2. Redistributions in binary form must reproduce the above copyright\n\
   notice in the documentation and/or other materials provided with\n\
   the distribution.\n\n\
")); // Line 622
  D2U_ANSI_FPRINTF(stdout,"%s", _("\
THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY\n\
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n\
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE\n\
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n\
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT\n\
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR\n\
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n\
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE\n\
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN\n\
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
")); // Line 635
} // Line 636

int is_dos2unix(const char *progname) // Line 638
{ // Line 639
  if ((strncmp(progname, "dos2unix", sizeof("dos2unix")) == 0) || (strncmp(progname, "mac2unix", sizeof("mac2unix")) == 0)) // Line 640
    return 1; // Line 641
  else // Line 642
    return 0; // Line 643
} // Line 644

void PrintUsage(const char *progname) // Line 646
{ // Line 647
  D2U_ANSI_FPRINTF(stdout,_("Usage: %s [options] [file ...] [-n infile outfile ...]\n"), progname); // Line 648
#ifndef NO_CHOWN
  D2U_ANSI_FPRINTF(stdout,_(" --allow-chown         allow file ownership change\n")); // Line 650
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -ascii                default conversion mode\n")); // Line 652
  D2U_ANSI_FPRINTF(stdout,_(" -iso                  conversion between DOS and ISO-8859-1 character set\n")); // Line 653
  D2U_ANSI_FPRINTF(stdout,_("   -1252               use Windows code page 1252 (Western European)\n")); // Line 654
  D2U_ANSI_FPRINTF(stdout,_("   -437                use DOS code page 437 (US) (default)\n")); // Line 655
  D2U_ANSI_FPRINTF(stdout,_("   -850                use DOS code page 850 (Western European)\n")); // Line 656
  D2U_ANSI_FPRINTF(stdout,_("   -860                use DOS code page 860 (Portuguese)\n")); // Line 657
  D2U_ANSI_FPRINTF(stdout,_("   -863                use DOS code page 863 (French Canadian)\n")); // Line 658
  D2U_ANSI_FPRINTF(stdout,_("   -865                use DOS code page 865 (Nordic)\n")); // Line 659
  D2U_ANSI_FPRINTF(stdout,_(" -7                    convert 8 bit characters to 7 bit space\n")); // Line 660
  if (is_dos2unix(progname)) // Line 661
    D2U_ANSI_FPRINTF(stdout,_(" -b, --keep-bom        keep Byte Order Mark\n")); // Line 662
  else // Line 663
    D2U_ANSI_FPRINTF(stdout,_(" -b, --keep-bom        keep Byte Order Mark (default)\n")); // Line 664
  D2U_ANSI_FPRINTF(stdout,_(" -c, --convmode        conversion mode\n\
   convmode            ascii, 7bit, iso, mac, default to ascii\n")); // Line 666
#ifdef D2U_UNIFILE
  D2U_ANSI_FPRINTF(stdout,_(" -D, --display-enc     set encoding of displayed text messages\n\
   encoding            ansi, unicode, utf8, default to ansi\n")); // Line 669
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -e, --add-eol         add a line break to the last line if there isn't one\n")); // Line 671
  D2U_ANSI_FPRINTF(stdout,_(" -f, --force           force conversion of binary files\n")); // Line 672
#ifdef D2U_UNICODE
#if (defined(_WIN32) && !defined(__CYGWIN__))
  D2U_ANSI_FPRINTF(stdout,_(" -gb, --gb18030        convert UTF-16 to GB18030\n")); // Line 675
#endif
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -h, --help            display this help text\n")); // Line 678
  D2U_ANSI_FPRINTF(stdout,_(" -i, --info[=FLAGS]    display file information\n\
   file ...            files to analyze\n")); // Line 680
  D2U_ANSI_FPRINTF(stdout,_(" -k, --keepdate        keep output file date\n")); // Line 681
  D2U_ANSI_FPRINTF(stdout,_(" -L, --license         display software license\n")); // Line 682
  D2U_ANSI_FPRINTF(stdout,_(" -l, --newline         add additional newline\n")); // Line 683
  D2U_ANSI_FPRINTF(stdout,_(" -m, --add-bom         add Byte Order Mark (default UTF-8)\n")); // Line 684
  D2U_ANSI_FPRINTF(stdout,_(" -n, --newfile         write to new file\n\
   infile              original file in new-file mode\n\
   outfile             output file in new-file mode\n")); // Line 687
#ifndef NO_CHOWN
  D2U_ANSI_FPRINTF(stdout,_(" --no-allow-chown      don't allow file ownership change (default)\n")); // Line 689
#endif
  D2U_ANSI_FPRINTF(stdout,_(" --no-add-eol          don't add a line break to the last line if there isn't one (default)\n")); // Line 691
  D2U_ANSI_FPRINTF(stdout,_(" -O, --to-stdout       write to standard output\n")); // Line 692
  D2U_ANSI_FPRINTF(stdout,_(" -o, --oldfile         write to old file (default)\n\
   file ...            files to convert in old-file mode\n")); // Line 694
  D2U_ANSI_FPRINTF(stdout,_(" -q, --quiet           quiet mode, suppress all warnings\n")); // Line 695
  if (is_dos2unix(progname)) // Line 696
    D2U_ANSI_FPRINTF(stdout,_(" -r, --remove-bom      remove Byte Order Mark (default)\n")); // Line 697
  else // Line 698
    D2U_ANSI_FPRINTF(stdout,_(" -r, --remove-bom      remove Byte Order Mark\n")); // Line 699
  D2U_ANSI_FPRINTF(stdout,_(" -s, --safe            skip binary files (default)\n")); // Line 700
#ifdef D2U_UNICODE
  D2U_ANSI_FPRINTF(stdout,_(" -u,  --keep-utf16     keep UTF-16 encoding\n")); // Line 702
  D2U_ANSI_FPRINTF(stdout,_(" -ul, --assume-utf16le assume that the input format is UTF-16LE\n")); // Line 703
  D2U_ANSI_FPRINTF(stdout,_(" -ub, --assume-utf16be assume that the input format is UTF-16BE\n")); // Line 704
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -v,  --verbose        verbose operation\n")); // Line 706
#ifdef S_ISLNK
  D2U_ANSI_FPRINTF(stdout,_(" -F, --follow-symlink  follow symbolic links and convert the targets\n")); // Line 708
#endif
#if defined(S_ISLNK) || (defined(_WIN32) && !defined(__CYGWIN__))
  D2U_ANSI_FPRINTF(stdout,_(" -R, --replace-symlink replace symbolic links with converted files\n\
                         (original target files remain unchanged)\n")); // Line 712
  D2U_ANSI_FPRINTF(stdout,_(" -S, --skip-symlink    keep symbolic links and targets unchanged (default)\n")); // Line 713
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -V, --version         display version number\n")); // Line 715
} // Line 716

#define MINGW32_W64 1

void PrintVersion(const char *progname, const char *localedir) // Line 720
{ // Line 721
  D2U_ANSI_FPRINTF(stdout,"%s %s (%s)\n", progname, VER_REVISION, VER_DATE); // Line 722
#if DEBUG
  D2U_ANSI_FPRINTF(stdout,"VER_AUTHOR: %s\n", VER_AUTHOR); // Line 724
#endif
#if defined(__WATCOMC__) && defined(__I86__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 16 bit version (WATCOMC).\n")); // Line 727
#elif defined(__TURBOC__) && defined(__MSDOS__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 16 bit version (TURBOC).\n")); // Line 729
#elif defined(__WATCOMC__) && defined(__DOS__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 32 bit version (WATCOMC).\n")); // Line 731
#elif defined(__DJGPP__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 32 bit version (DJGPP).\n")); // Line 733
#elif defined(__MSYS__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("MSYS version.\n")); // Line 735
#elif defined(__CYGWIN__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Cygwin version.\n")); // Line 737
#elif defined(__WIN64__) && defined(__MINGW64__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 64 bit version (MinGW-w64).\n")); // Line 739
#elif defined(__WATCOMC__) && defined(__NT__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 32 bit version (WATCOMC).\n")); // Line 741
#elif defined(_WIN32) && defined(__MINGW32__) && (D2U_COMPILER == MINGW32_W64)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 32 bit version (MinGW-w64).\n")); // Line 743
#elif defined(_WIN32) && defined(__MINGW32__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 32 bit version (MinGW).\n")); // Line 745
#elif defined(_WIN64) && defined(_MSC_VER)
  D2U_ANSI_FPRINTF(stdout,_("Windows 64 bit version (MSVC %d).\n"),_MSC_VER); // Line 747
#elif defined(_WIN32) && defined(_MSC_VER)
  D2U_ANSI_FPRINTF(stdout,_("Windows 32 bit version (MSVC %d).\n"),_MSC_VER); // Line 749
#elif defined (__OS2__) && defined(__WATCOMC__) /* OS/2 Warp */
  D2U_ANSI_FPRINTF(stdout,"%s", _("OS/2 version (WATCOMC).\n")); // Line 751
#elif defined (__OS2__) && defined(__EMX__) /* OS/2 Warp */
  D2U_ANSI_FPRINTF(stdout,"%s", _("OS/2 version (EMX).\n")); // Line 753
#elif defined(__OS)
  D2U_ANSI_FPRINTF(stdout,_("%s version.\n"), __OS); // Line 755
#endif
#if defined(_WIN32) && defined(WINVER)
  D2U_ANSI_FPRINTF(stdout,"WINVER 0x%X\n",WINVER); // Line 758
#endif
#ifdef D2U_UNICODE
  D2U_ANSI_FPRINTF(stdout,"%s", _("With Unicode UTF-16 support.\n")); // Line 761
#else
  D2U_ANSI_FPRINTF(stdout,"%s", _("Without Unicode UTF-16 support.\n")); // Line 763
#endif
#ifdef _WIN32
#ifdef D2U_UNIFILE
  D2U_ANSI_FPRINTF(stdout,"%s", _("With Unicode file name support.\n")); // Line 767
#else
  D2U_ANSI_FPRINTF(stdout,"%s", _("Without Unicode file name support.\n")); // Line 769
#endif
#endif
#ifdef ENABLE_NLS
  D2U_ANSI_FPRINTF(stdout,"%s", _("With native language support.\n")); // Line 773
#else
  D2U_ANSI_FPRINTF(stdout,"%s", "Without native language support.\n"); // Line 775
#endif
#ifndef NO_CHOWN
  D2U_ANSI_FPRINTF(stdout,"%s", _("With support to preserve the user and group ownership of files.\n")); // Line 778
#else
  D2U_ANSI_FPRINTF(stdout,"%s", _("Without support to preserve the user and group ownership of files.\n")); // Line 780
#endif
#ifdef ENABLE_NLS
  D2U_ANSI_FPRINTF(stdout,"LOCALEDIR: %s\n", localedir); // Line 783
#endif
  D2U_ANSI_FPRINTF(stdout,"https://waterlan.home.xs4all.nl/dos2unix.html\n"); // Line 785
  D2U_ANSI_FPRINTF(stdout,"https://dos2unix.sourceforge.io/\n"); // Line 786
} // Line 787

/* opens file of name ipFN in read only mode // Line 789
 * returns: NULL if failure // Line 790
 *          file stream otherwise // Line 791
 */ // Line 792
FILE* OpenInFile(char *ipFN) // Line 793
{ // Line 794
#ifdef D2U_UNIFILE
  wchar_t pathw[D2U_MAX_PATH]; // Line 796

  d2u_MultiByteToWideChar(CP_UTF8, 0, ipFN, -1, pathw, D2U_MAX_PATH); // Line 798
  return _wfopen(pathw, R_CNTRLW); // Line 799
#else
  return (fopen(ipFN, R_CNTRL)); // Line 801
#endif
} // Line 803


/* opens file of name opFN in write only mode // Line 806
 * returns: NULL if failure // Line 807
 *          file stream otherwise // Line 808
 */ // Line 809
FILE* OpenOutFile(char *opFN) // Line 810
{ // Line 811
#ifdef D2U_UNIFILE
  wchar_t pathw[D2U_MAX_PATH]; // Line 813

  d2u_MultiByteToWideChar(CP_UTF8, 0, opFN, -1, pathw, D2U_MAX_PATH); // Line 815
  return _wfopen(pathw, W_CNTRLW); // Line 816
#else
  return (fopen(opFN, W_CNTRL)); // Line 818
#endif
} // Line 820

/* opens file descriptor in write only mode // Line 822
 * returns: NULL if failure // Line 823
 *          file stream otherwise // Line 824
 */ // Line 825
FILE* OpenOutFiled(int fd) // Line 826
{ // Line 827
  return (fdopen(fd, W_CNTRL)); // Line 828
} // Line 829

#if defined(__TURBOC__) || defined(__MSYS__) || defined(_MSC_VER)
/* Both dirname() and basename() may modify the contents of path. // Line 832
 * It may be desirable to pass a copy. */ // Line 833
char *dirname(char *path) // Line 834
{ // Line 835
  char *ptr; // Line 836

  /* replace all back slashes with slashes */ // Line 838
  while ( (ptr = strchr(path,'\\')) != NULL) // Line 839
    *ptr = '/'; // Line 840
  /* Code checkers may report that the condition (path == NULL) is redundant. // Line 841
     E.g. Cppcheck 1.72. The condition (path == NULL) is needed, because // Line 842
     the behaviour of strrchr is not specified when it get's a NULL string. // Line 843
     The behaviour may be undefined, dependent on the implementation. */ // Line 844
  if ((path == NULL) || ((ptr=strrchr(path,'/')) == NULL)) // Line 845
    return "."; // Line 846

  if (strcmp(path,"/") == 0) // Line 848
    return "/"; // Line 849

  *ptr = '\0'; // Line 851
  return path; // Line 852
} // Line 853

#ifdef NO_MKSTEMP
char *basename(char *path) // Line 856
{ // Line 857
  char *ptr; // Line 858

  /* replace all back slashes with slashes */ // Line 860
  while ( (ptr = strchr(path,'\\')) != NULL) // Line 861
    *ptr = '/'; // Line 862
  /* Code checkers may report that the condition (path == NULL) is redundant. // Line 863
     E.g. Cppcheck 1.72. The condition (path == NULL) is needed, because // Line 864
     the behaviour of strrchr is not specified when it get's a NULL string. // Line 865
     The behaviour may be undefined, dependent on the implementation. */ // Line 866
  if ((path == NULL) || ((ptr=strrchr(path,'/')) == NULL)) // Line 867
    return path ; // Line 868

  if (strcmp(path,"/") == 0) // Line 870
    return "/"; // Line 871

   ptr++; // Line 873
   return ptr ; // Line 874
} // Line 875
#endif
#endif

/* Standard mktemp() is not safe to use (See mktemp(3)). // Line 879
 * On Windows it is recommended to use GetTempFileName() (See MSDN). // Line 880
 * This mktemp() wrapper redirects to GetTempFileName() on Windows. // Line 881
 * On Windows template is not modified, the returned pointer has to // Line 882
 * be used. // Line 883
 */ // Line 884
#ifdef NO_MKSTEMP
char *d2u_mktemp(char *template) // Line 886
{ // Line 887
#if defined(_WIN32) && !defined(__CYGWIN__)

  unsigned int uRetVal; // Line 890
  char *cpy1, *cpy2, *dn, *bn; // Line 891
  char *ptr; // Line 892
  size_t len; // Line 893
#ifdef D2U_UNIFILE /* template is UTF-8 formatted. */
  wchar_t dnw[MAX_PATH]; // Line 895
  wchar_t bnw[MAX_PATH]; // Line 896
  wchar_t szTempFileNamew[MAX_PATH]; // Line 897
  char *fname_str; // Line 898
  int error = 0; // Line 899
#else
  char szTempFileName[MAX_PATH]; // Line 901
  char *fname_str; // Line 902
#endif
  if ((cpy1 = strdup(template)) == NULL) // Line 904
    return NULL; // Line 905
  if ((cpy2 = strdup(template)) == NULL) { // Line 906
    free(cpy1); // Line 907
    return NULL; // Line 908
  } // Line 909
  dn = dirname(cpy1); // Line 910
  bn = basename(cpy2); // Line 911
#ifdef D2U_UNIFILE /* template is UTF-8 formatted. */
  if (d2u_MultiByteToWideChar(CP_UTF8, 0, dn, -1, NULL, 0) > (MAX_PATH - 15)) { // Line 913
      D2U_UTF8_FPRINTF(stderr, "%s: ", "dos2unix"); // Line 914
      D2U_ANSI_FPRINTF(stderr, _("Path for temporary output file is too long:")); // Line 915
      D2U_UTF8_FPRINTF(stderr, " %s\n", dn); // Line 916
      error=1; // Line 917
  } // Line 918
  if ((!error) && (d2u_MultiByteToWideChar(CP_UTF8, 0, dn, -1, dnw, MAX_PATH) == 0)) // Line 919
    error=1; // Line 920
  if ((!error) && (d2u_MultiByteToWideChar(CP_UTF8, 0, bn, -1, bnw, MAX_PATH) == 0)) // Line 921
    error=1; // Line 922
  free(cpy1); // Line 923
  free(cpy2); // Line 924
  if (error) // Line 925
    return NULL; // Line 926
  uRetVal = GetTempFileNameW(dnw, bnw, 0, szTempFileNamew); // Line 927
  if (! uRetVal) { // Line 928
    d2u_PrintLastError("dos2unix"); // Line 929
    return NULL; // Line 930
  } // Line 931
  len =(size_t) d2u_WideCharToMultiByte(CP_UTF8, 0, szTempFileNamew, -1, NULL, 0, NULL, NULL); // Line 932
  fname_str = (char *)malloc(len); // Line 933
  if (! fname_str) // Line 934
    return NULL; // Line 935
  if (d2u_WideCharToMultiByte(CP_UTF8, 0, szTempFileNamew, -1, fname_str, MAX_PATH, NULL, NULL) == 0) // Line 936
    return NULL; // Line 937
#else
  uRetVal = GetTempFileNameA(dn, bn, 0, szTempFileName); // Line 939
  free(cpy1); // Line 940
  free(cpy2); // Line 941
  if (! uRetVal) { // Line 942
    d2u_PrintLastError("dos2unix"); // Line 943
    return NULL; // Line 944
  } // Line 945
  len = strlen(szTempFileName) +1; // Line 946
  fname_str = (char *)malloc(len); // Line 947
  if (! fname_str) // Line 948
    return NULL; // Line 949
  d2u_strncpy(fname_str, szTempFileName,len); // Line 950
#endif
  /* replace all back slashes with slashes */ // Line 952
  while ( (ptr = strchr(fname_str,'\\')) != NULL) // Line 953
    *ptr = '/'; // Line 954
  return fname_str; // Line 955

#else
  return mktemp(template); // Line 958
#endif
} // Line 960
#endif

FILE* MakeTempFileFrom(const char *OutFN, char **fname_ret) // Line 963
{ // Line 964
  char *cpy = strdup(OutFN); // Line 965
  char *dir = NULL; // Line 966
  size_t fname_len = 0; // Line 967
  char  *fname_str = NULL; // Line 968
  FILE *fp = NULL;  /* file pointer */ // Line 969
#ifdef NO_MKSTEMP
  char *name; // Line 971
#else
  int fd = -1;  /* file descriptor */ // Line 973
#endif

  *fname_ret = NULL; // Line 976

  if (!cpy) // Line 978
    goto make_failed; // Line 979

  dir = dirname(cpy); // Line 981

  fname_len = strlen(dir) + strlen("/d2utmpXXXXXX") + sizeof (char); // Line 983
  if (!(fname_str = (char *)malloc(fname_len))) // Line 984
    goto make_failed; // Line 985
  sprintf(fname_str, "%s%s", dir, "/d2utmpXXXXXX"); // Line 986
  *fname_ret = fname_str; // Line 987

  free(cpy); // Line 989
  cpy = NULL; // Line 990

#ifdef NO_MKSTEMP
  if ((name = d2u_mktemp(fname_str)) == NULL) // Line 993
    goto make_failed; // Line 994
  *fname_ret = name; // Line 995
  if ((fp = OpenOutFile(name)) == NULL) // Line 996
    goto make_failed; // Line 997
#else
  if ((fd = mkstemp(fname_str)) == -1) // Line 999
    goto make_failed; // Line 1000

  if ((fp=OpenOutFiled(fd)) == NULL) // Line 1002
    goto make_failed; // Line 1003
#endif

  return (fp); // Line 1006

  make_failed: // Line 1008
    if (cpy) { // Line 1009
       free(cpy); // Line 1010
       cpy = NULL; // Line 1011
    } // Line 1012
    free(*fname_ret); // Line 1013
    *fname_ret = NULL; // Line 1014
    return NULL; // Line 1015
} // Line 1016

/* Test if *lFN is the name of a symbolic link.  If not, set *rFN equal // Line 1018
 * to lFN, and return 0.  If so, then use canonicalize_file_name or // Line 1019
 * realpath to determine the pointed-to file; the resulting name is // Line 1020
 * stored in newly allocated memory, *rFN is set to point to that value, // Line 1021
 * and 1 is returned. On error, -1 is returned and errno is set as // Line 1022
 * appropriate. // Line 1023
 * // Line 1024
 * Note that if symbolic links are not supported, then 0 is always returned // Line 1025
 * and *rFN = lFN. // Line 1026
 * // Line 1027
 * returns: 0 if success, and *lFN is not a symlink // Line 1028
 *          1 if success, and *lFN is a symlink // Line 1029
 *         -1 otherwise // Line 1030
 */ // Line 1031
int ResolveSymbolicLink(char *lFN, char **rFN, CFlag *ipFlag, const char *progname) // Line 1032
{ // Line 1033
  int RetVal = 0; // Line 1034
#ifdef S_ISLNK
  struct stat StatBuf; // Line 1036
  const char *errstr; // Line 1037
  char *targetFN = NULL; // Line 1038

  if (STAT(lFN, &StatBuf)) { // Line 1040
    if (ipFlag->verbose) { // Line 1041
      ipFlag->error = errno; // Line 1042
      errstr = strerror(errno); // Line 1043
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1044
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1045
    } // Line 1046
    RetVal = -1; // Line 1047
  } // Line 1048
  else if (S_ISLNK(StatBuf.st_mode)) { // Line 1049
#if USE_CANONICALIZE_FILE_NAME
    targetFN = canonicalize_file_name(lFN); // Line 1051
    if (!targetFN) { // Line 1052
      if (ipFlag->verbose) { // Line 1053
        ipFlag->error = errno; // Line 1054
        errstr = strerror(errno); // Line 1055
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1056
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1057
      } // Line 1058
      RetVal = -1; // Line 1059
    } // Line 1060
    else { // Line 1061
      *rFN = targetFN; // Line 1062
      RetVal = 1; // Line 1063
    } // Line 1064
#else
    /* Sigh. Use realpath, but realize that it has a fatal // Line 1066
     * flaw: PATH_MAX isn't necessarily the maximum path // Line 1067
     * length -- so realpath() might fail. */ // Line 1068
    targetFN = (char *) malloc(PATH_MAX * sizeof(char)); // Line 1069
    if (!targetFN) { // Line 1070
      if (ipFlag->verbose) { // Line 1071
        ipFlag->error = errno; // Line 1072
        errstr = strerror(errno); // Line 1073
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1074
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1075
      } // Line 1076
      RetVal = -1; // Line 1077
    } // Line 1078
    else { // Line 1079
      /* is there any platform with S_ISLNK that does not have realpath? */ // Line 1080
      char *rVal = realpath(lFN, targetFN); // Line 1081
      if (!rVal) { // Line 1082
        if (ipFlag->verbose) { // Line 1083
          ipFlag->error = errno; // Line 1084
          errstr = strerror(errno); // Line 1085
          D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1086
          D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1087
        } // Line 1088
        free(targetFN); // Line 1089
        RetVal = -1; // Line 1090
      } // Line 1091
      else { // Line 1092
        *rFN = rVal; // Line 1093
        RetVal = 1; // Line 1094
      } // Line 1095
    } // Line 1096
#endif /* !USE_CANONICALIZE_FILE_NAME */
  } // Line 1098
  else // Line 1099
    *rFN = lFN; // Line 1100
#else  /* !S_ISLNK */
  *rFN = lFN; // Line 1102
#endif /* !S_ISLNK */
  return RetVal; // Line 1104
} // Line 1105

/* Read the Byte Order Mark. // Line 1107
   Returns file pointer or NULL in case of a read error */ // Line 1108

FILE *read_bom (FILE *f, int *bomtype) // Line 1110
{ // Line 1111
  /* BOMs // Line 1112
   * UTF16-LE  ff fe // Line 1113
   * UTF16-BE  fe ff // Line 1114
   * UTF-8     ef bb bf // Line 1115
   * GB18030   84 31 95 33 // Line 1116
   */ // Line 1117

  *bomtype = FILE_MBS; // Line 1119

   /* Check for BOM */ // Line 1121
   if  (f != NULL) { // Line 1122
      int bom[4]; // Line 1123
      if ((bom[0] = fgetc(f)) == EOF) { // Line 1124
         if (ferror(f)) { // Line 1125
           return NULL; // Line 1126
         } // Line 1127
         *bomtype = FILE_MBS; // Line 1128
         return(f); // Line 1129
      } // Line 1130
      if ((bom[0] != 0xff) && (bom[0] != 0xfe) && (bom[0] != 0xef) && (bom[0] != 0x84)) { // Line 1131
         if (ungetc(bom[0], f) == EOF) return NULL; // Line 1132
         *bomtype = FILE_MBS; // Line 1133
         return(f); // Line 1134
      } // Line 1135
      if ((bom[1] = fgetc(f)) == EOF) { // Line 1136
         if (ferror(f)) { // Line 1137
           return NULL; // Line 1138
         } // Line 1139
         if (ungetc(bom[1], f) == EOF) return NULL; // Line 1140
         if (ungetc(bom[0], f) == EOF) return NULL; // Line 1141
         *bomtype = FILE_MBS; // Line 1142
         return(f); // Line 1143
      } // Line 1144
      if ((bom[0] == 0xff) && (bom[1] == 0xfe)) { /* UTF16-LE */ // Line 1145
         *bomtype = FILE_UTF16LE; // Line 1146
         return(f); // Line 1147
      } // Line 1148
      if ((bom[0] == 0xfe) && (bom[1] == 0xff)) { /* UTF16-BE */ // Line 1149
         *bomtype = FILE_UTF16BE; // Line 1150
         return(f); // Line 1151
      } // Line 1152
      if ((bom[2] = fgetc(f)) == EOF) { // Line 1153
         if (ferror(f)) { // Line 1154
           return NULL; // Line 1155
         } // Line 1156
         if (ungetc(bom[2], f) == EOF) return NULL; // Line 1157
         if (ungetc(bom[1], f) == EOF) return NULL; // Line 1158
         if (ungetc(bom[0], f) == EOF) return NULL; // Line 1159
         *bomtype = FILE_MBS; // Line 1160
         return(f); // Line 1161
      } // Line 1162
      if ((bom[0] == 0xef) && (bom[1] == 0xbb) && (bom[2]== 0xbf)) { /* UTF-8 */ // Line 1163
         *bomtype = FILE_UTF8; // Line 1164
         return(f); // Line 1165
      } // Line 1166
      if ((bom[0] == 0x84) && (bom[1] == 0x31) && (bom[2]== 0x95)) { // Line 1167
         bom[3] = fgetc(f); // Line 1168
           if (ferror(f)) { // Line 1169
             return NULL; // Line 1170
          } // Line 1171
         if (bom[3]== 0x33) { /* GB18030 */ // Line 1172
           *bomtype = FILE_GB18030; // Line 1173
           return(f); // Line 1174
         } // Line 1175
         if (ungetc(bom[3], f) == EOF) return NULL; // Line 1176
      } // Line 1177
      if (ungetc(bom[2], f) == EOF) return NULL; // Line 1178
      if (ungetc(bom[1], f) == EOF) return NULL; // Line 1179
      if (ungetc(bom[0], f) == EOF) return NULL; // Line 1180
      *bomtype = FILE_MBS; // Line 1181
      return(f); // Line 1182
   } // Line 1183
  return(f); // Line 1184
} // Line 1185

FILE *write_bom (FILE *f, CFlag *ipFlag, const char *progname) // Line 1187
{ // Line 1188
  int bomtype = ipFlag->bomtype; // Line 1189

  if ((bomtype == FILE_MBS)&&(ipFlag->locale_target == TARGET_GB18030)) // Line 1191
    bomtype = FILE_GB18030; // Line 1192

  if (ipFlag->keep_utf16) // Line 1194
  { // Line 1195
    switch (bomtype) { // Line 1196
      case FILE_UTF16LE:   /* UTF-16 Little Endian */ // Line 1197
        if (fprintf(f, "%s", "\xFF\xFE") < 0) return NULL; // Line 1198
        if (ipFlag->verbose > 1) { // Line 1199
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1200
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-16LE")); // Line 1201
        } // Line 1202
        break; // Line 1203
      case FILE_UTF16BE:   /* UTF-16 Big Endian */ // Line 1204
        if (fprintf(f, "%s", "\xFE\xFF") < 0) return NULL; // Line 1205
        if (ipFlag->verbose > 1) { // Line 1206
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1207
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-16BE")); // Line 1208
        } // Line 1209
        break; // Line 1210
      case FILE_GB18030:  /* GB18030 */ // Line 1211
        if (fprintf(f, "%s", "\x84\x31\x95\x33") < 0) return NULL; // Line 1212
        if (ipFlag->verbose > 1) { // Line 1213
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1214
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("GB18030")); // Line 1215
        } // Line 1216
        break; // Line 1217
      default:      /* UTF-8 */ // Line 1218
        if (fprintf(f, "%s", "\xEF\xBB\xBF") < 0) return NULL; // Line 1219
        if (ipFlag->verbose > 1) { // Line 1220
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1221
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-8")); // Line 1222
        } // Line 1223
      ; // Line 1224
    } // Line 1225
  } else { // Line 1226
    if ((bomtype == FILE_GB18030) || // Line 1227
        (((bomtype == FILE_UTF16LE)||(bomtype == FILE_UTF16BE))&&(ipFlag->locale_target == TARGET_GB18030)) // Line 1228
       ) { // Line 1229
        if (fprintf(f, "%s", "\x84\x31\x95\x33") < 0) return NULL; /* GB18030 */ // Line 1230
        if (ipFlag->verbose > 1) // Line 1231
        { // Line 1232
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1233
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("GB18030")); // Line 1234
        } // Line 1235
     } else { // Line 1236
        if (fprintf(f, "%s", "\xEF\xBB\xBF") < 0) return NULL; /* UTF-8 */ // Line 1237
        if (ipFlag->verbose > 1) // Line 1238
        { // Line 1239
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1240
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-8")); // Line 1241
        } // Line 1242
     } // Line 1243
  } // Line 1244
  return(f); // Line 1245
} // Line 1246

void print_bom (const int bomtype, const char *filename, const char *progname) // Line 1248
{ // Line 1249
    char informat[64]; // Line 1250

    switch (bomtype) { // Line 1252
    case FILE_UTF16LE:   /* UTF-16 Little Endian */ // Line 1253
      d2u_strncpy(informat,_("UTF-16LE"),sizeof(informat)); // Line 1254
      break; // Line 1255
    case FILE_UTF16BE:   /* UTF-16 Big Endian */ // Line 1256
      d2u_strncpy(informat,_("UTF-16BE"),sizeof(informat)); // Line 1257
      break; // Line 1258
    case FILE_UTF8:      /* UTF-8 */ // Line 1259
      d2u_strncpy(informat,_("UTF-8"),sizeof(informat)); // Line 1260
      break; // Line 1261
    case FILE_GB18030:      /* GB18030 */ // Line 1262
      d2u_strncpy(informat,_("GB18030"),sizeof(informat)); // Line 1263
      break; // Line 1264
    default: // Line 1265
    ; // Line 1266
  } // Line 1267

  if (bomtype > 0) { // Line 1269
#ifdef D2U_UNIFILE
    wchar_t informatw[64]; // Line 1271
#endif
    informat[sizeof(informat)-1] = '\0'; // Line 1273

/* Change informat to UTF-8 for d2u_utf8_fprintf. */ // Line 1275
#ifdef D2U_UNIFILE
    /* The format string is encoded in the system default // Line 1277
     * Windows ANSI code page. May have been translated // Line 1278
     * by gettext. Convert it to wide characters. */ // Line 1279
    d2u_MultiByteToWideChar(CP_ACP,0, informat, -1, informatw, sizeof(informat)); // Line 1280
    /* then convert the format string to UTF-8 */ // Line 1281
    d2u_WideCharToMultiByte(CP_UTF8, 0, informatw, -1, informat, sizeof(informat), NULL, NULL); // Line 1282
#endif

    D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1285
    D2U_UTF8_FPRINTF(stderr, _("Input file %s has %s BOM.\n"), filename, informat); // Line 1286
  } // Line 1287

} // Line 1289

void print_bom_info (const int bomtype) // Line 1291
{ // Line 1292
/* The BOM info must not be translated to other languages, otherwise scripts // Line 1293
   that process the output may not work in other than English locales. */ // Line 1294
    switch (bomtype) { // Line 1295
    case FILE_UTF16LE:   /* UTF-16 Little Endian */ // Line 1296
      D2U_UTF8_FPRINTF(stdout, "  UTF-16LE"); // Line 1297
      break; // Line 1298
    case FILE_UTF16BE:   /* UTF-16 Big Endian */ // Line 1299
      D2U_UTF8_FPRINTF(stdout, "  UTF-16BE"); // Line 1300
      break; // Line 1301
    case FILE_UTF8:      /* UTF-8 */ // Line 1302
      D2U_UTF8_FPRINTF(stdout, "  UTF-8   "); // Line 1303
      break; // Line 1304
    case FILE_GB18030:   /* GB18030 */ // Line 1305
      D2U_UTF8_FPRINTF(stdout, "  GB18030 "); // Line 1306
      break; // Line 1307
    default: // Line 1308
      D2U_UTF8_FPRINTF(stdout, "  no_bom  "); // Line 1309
    ; // Line 1310
  } // Line 1311
} // Line 1312

/* check_unicode_info() // Line 1314
 * Print assumed encoding and read file's BOM. Return file's BOM in *bomtype_orig. // Line 1315
 * Set ipFlag->bomtype to assumed BOM type, when file's BOM == FILE_MBS. // Line 1316
 * Return -1 when a read error occurred, or when whar_t < 32 bit on non-Windows OS. // Line 1317
 * Return 0 when everything is OK. // Line 1318
 */ // Line 1319

int check_unicode_info(FILE *InF, CFlag *ipFlag, const char *progname, int *bomtype_orig) // Line 1321
{ // Line 1322
#ifdef D2U_UNICODE
  if (ipFlag->verbose > 1) { // Line 1324
    if (ipFlag->ConvMode == CONVMODE_UTF16LE) { // Line 1325
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1326
      D2U_UTF8_FPRINTF(stderr, _("Assuming UTF-16LE encoding.\n") ); // Line 1327
    } // Line 1328
    if (ipFlag->ConvMode == CONVMODE_UTF16BE) { // Line 1329
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1330
      D2U_UTF8_FPRINTF(stderr, _("Assuming UTF-16BE encoding.\n") ); // Line 1331
    } // Line 1332
  } // Line 1333
#endif
  if ((InF = read_bom(InF, &ipFlag->bomtype)) == NULL) { // Line 1335
    d2u_getc_error(ipFlag,progname); // Line 1336
    return -1; // Line 1337
  } // Line 1338
  *bomtype_orig = ipFlag->bomtype; // Line 1339
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_MBS) && (ipFlag->ConvMode == CONVMODE_UTF16LE)) // Line 1341
    ipFlag->bomtype = FILE_UTF16LE; // Line 1342
  if ((ipFlag->bomtype == FILE_MBS) && (ipFlag->ConvMode == CONVMODE_UTF16BE)) // Line 1343
    ipFlag->bomtype = FILE_UTF16BE; // Line 1344


#if !defined(_WIN32) && !defined(__CYGWIN__) /* Not Windows or Cygwin */
  if (!ipFlag->keep_utf16 && ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE))) { // Line 1348
    if (sizeof(wchar_t) < 4) { // Line 1349
      /* A decoded UTF-16 surrogate pair must fit in a wchar_t */ // Line 1350
      ipFlag->status |= WCHAR_T_TOO_SMALL ; // Line 1351
      if (!ipFlag->error) ipFlag->error = 1; // Line 1352
      return -1; // Line 1353
    } // Line 1354
  } // Line 1355
#endif
#endif

  return 0; // Line 1359
} // Line 1360

int check_unicode(FILE *InF, FILE *TempF,  CFlag *ipFlag, const char *ipInFN, const char *progname) // Line 1362
{ // Line 1363

#ifdef D2U_UNICODE
  if (ipFlag->verbose > 1) { // Line 1366
    if (ipFlag->ConvMode == CONVMODE_UTF16LE) { // Line 1367
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1368
      D2U_UTF8_FPRINTF(stderr, _("Assuming UTF-16LE encoding.\n") ); // Line 1369
    } // Line 1370
    if (ipFlag->ConvMode == CONVMODE_UTF16BE) { // Line 1371
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1372
      D2U_UTF8_FPRINTF(stderr, _("Assuming UTF-16BE encoding.\n") ); // Line 1373
    } // Line 1374
  } // Line 1375
#endif
  if ((InF = read_bom(InF, &ipFlag->bomtype)) == NULL) { // Line 1377
    d2u_getc_error(ipFlag,progname); // Line 1378
    return -1; // Line 1379
  } // Line 1380
  if (ipFlag->verbose > 1) // Line 1381
    print_bom(ipFlag->bomtype, ipInFN, progname); // Line 1382
#ifndef D2U_UNICODE
  /* It is possible that an UTF-16 has no 8-bit binary symbols. We must stop // Line 1384
   * processing an UTF-16 file when UTF-16 is not supported. Don't trust on // Line 1385
   * finding a binary symbol. // Line 1386
   */ // Line 1387
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1388
    ipFlag->status |= UNICODE_NOT_SUPPORTED ; // Line 1389
    return -1; // Line 1390
  } // Line 1391
#endif
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_MBS) && (ipFlag->ConvMode == CONVMODE_UTF16LE)) // Line 1394
    ipFlag->bomtype = FILE_UTF16LE; // Line 1395
  if ((ipFlag->bomtype == FILE_MBS) && (ipFlag->ConvMode == CONVMODE_UTF16BE)) // Line 1396
    ipFlag->bomtype = FILE_UTF16BE; // Line 1397


#if !defined(_WIN32) && !defined(__CYGWIN__) /* Not Windows or Cygwin */
  if (!ipFlag->keep_utf16 && ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE))) { // Line 1401
    if (sizeof(wchar_t) < 4) { // Line 1402
      /* A decoded UTF-16 surrogate pair must fit in a wchar_t */ // Line 1403
      ipFlag->status |= WCHAR_T_TOO_SMALL ; // Line 1404
      if (!ipFlag->error) ipFlag->error = 1; // Line 1405
      return -1; // Line 1406
    } // Line 1407
  } // Line 1408
#endif

#if !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__)  /* Unix, Cygwin */
  if (strcmp(nl_langinfo(CODESET), "GB18030") == 0) // Line 1412
    ipFlag->locale_target = TARGET_GB18030; // Line 1413
#endif
#endif

  if ((ipFlag->add_bom) || ((ipFlag->keep_bom) && (ipFlag->bomtype > 0))) // Line 1417
    if (write_bom(TempF, ipFlag, progname) == NULL) return -1; // Line 1418

  return 0; // Line 1420
} // Line 1421

/* convert file ipInFN and write to file ipOutFN // Line 1423
 * returns: 0 if success // Line 1424
 *         -1 otherwise // Line 1425
 */ // Line 1426
int ConvertNewFile(char *ipInFN, char *ipOutFN, CFlag *ipFlag, const char *progname, // Line 1427
                   int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 1428
#ifdef D2U_UNICODE
                 , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 1430
#endif
                  ) // Line 1432
{ // Line 1433
  int RetVal = 0; // Line 1434
  FILE *InF = NULL; // Line 1435
  FILE *TempF = NULL; // Line 1436
  char *TempPath; // Line 1437
  const char *errstr; // Line 1438
#ifdef D2U_UNIFILE
   struct _stat StatBuf; // Line 1440
   wchar_t pathw[D2U_MAX_PATH]; // Line 1441
#else
  struct stat StatBuf; // Line 1443
#endif
  struct utimbuf UTimeBuf; // Line 1445
#ifndef NO_CHMOD
  mode_t mask; // Line 1447
#endif
  char *TargetFN = NULL; // Line 1449
  int ResolveSymlinkResult = 0; // Line 1450

  ipFlag->status = 0 ; // Line 1452

  /* Test if output file is a symbolic link */ // Line 1454
  if (symbolic_link(ipOutFN) && !ipFlag->Follow) { // Line 1455
    ipFlag->status |= OUTPUTFILE_SYMLINK ; // Line 1456
    /* Not a failure, skipping input file according spec. (keep symbolic link unchanged) */ // Line 1457
    return -1; // Line 1458
  } // Line 1459

  /* Test if input file is a regular file or symbolic link */ // Line 1461
  if (regfile(ipInFN, 1, ipFlag, progname)) { // Line 1462
    ipFlag->status |= NO_REGFILE ; // Line 1463
    /* Not a failure, skipping non-regular input file according spec. */ // Line 1464
    return -1; // Line 1465
  } // Line 1466

  /* Test if input file target is a regular file */ // Line 1468
  if (symbolic_link(ipInFN) && regfile_target(ipInFN, ipFlag,progname)) { // Line 1469
    ipFlag->status |= INPUT_TARGET_NO_REGFILE ; // Line 1470
    /* Not a failure, skipping non-regular input file according spec. */ // Line 1471
    return -1; // Line 1472
  } // Line 1473

  /* Test if output file target is a regular file */ // Line 1475
  if (symbolic_link(ipOutFN) && (ipFlag->Follow == SYMLINK_FOLLOW) && regfile_target(ipOutFN, ipFlag,progname)) { // Line 1476
    ipFlag->status |= OUTPUT_TARGET_NO_REGFILE ; // Line 1477
    /* Failure, input is regular, cannot produce output. */ // Line 1478
    if (!ipFlag->error) ipFlag->error = 1; // Line 1479
    return -1; // Line 1480
  } // Line 1481

  /* retrieve ipInFN file date stamp */ // Line 1483
#ifdef D2U_UNIFILE
  d2u_MultiByteToWideChar(CP_UTF8, 0, ipInFN, -1, pathw, D2U_MAX_PATH); // Line 1485
  if (_wstat(pathw, &StatBuf)) { // Line 1486
#else
  if (stat(ipInFN, &StatBuf)) { // Line 1488
#endif
    if (ipFlag->verbose) { // Line 1490
      ipFlag->error = errno; // Line 1491
      errstr = strerror(errno); // Line 1492
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, ipInFN); // Line 1493
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1494
    } // Line 1495
    return -1; // Line 1496
  } // Line 1497

  /* can open in file? */ // Line 1499
  InF=OpenInFile(ipInFN); // Line 1500
  if (InF == NULL) { // Line 1501
    if (ipFlag->verbose) { // Line 1502
      ipFlag->error = errno; // Line 1503
      errstr = strerror(errno); // Line 1504
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, ipInFN); // Line 1505
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1506
    } // Line 1507
    return -1; // Line 1508
  } // Line 1509

  /* If output file is a symbolic link, optional resolve the link and modify  */ // Line 1511
  /* the target, instead of removing the link and creating a new regular file */ // Line 1512
  TargetFN = ipOutFN; // Line 1513
  if (symbolic_link(ipOutFN) && !RetVal) { // Line 1514
    ResolveSymlinkResult = 0; /* indicates that TargetFN need not be freed */ // Line 1515
    if (ipFlag->Follow == SYMLINK_FOLLOW) { // Line 1516
      ResolveSymlinkResult = ResolveSymbolicLink(ipOutFN, &TargetFN, ipFlag, progname); // Line 1517
      if (ResolveSymlinkResult < 0) { // Line 1518
        if (ipFlag->verbose) { // Line 1519
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1520
          D2U_UTF8_FPRINTF(stderr, _("problems resolving symbolic link '%s'\n"), ipOutFN); // Line 1521
        } // Line 1522
        RetVal = -1; // Line 1523
      } // Line 1524
    } // Line 1525
  } // Line 1526
  /* The symbolic link's target could be on another file system. rename() used below // Line 1527
   * can't move files to another file system. We need to create the temp file on the // Line 1528
   * target file system. // Line 1529
   */ // Line 1530

  /* can open temp output file? */ // Line 1532
  if((TempF = MakeTempFileFrom(TargetFN, &TempPath))==NULL) { // Line 1533
    if (ipFlag->verbose) { // Line 1534
      if (errno) { // Line 1535
        ipFlag->error = errno; // Line 1536
        errstr = strerror(errno); // Line 1537
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1538
        D2U_ANSI_FPRINTF(stderr, _("Failed to open temporary output file: %s\n"), errstr); // Line 1539
      } else { // Line 1540
        /*  In case temp path was too long on Windows, errno is 0. */ // Line 1541
        if (!ipFlag->error) ipFlag->error = 1; // Line 1542
      } // Line 1543
    } // Line 1544
    RetVal = -1; // Line 1545
  } // Line 1546

#if DEBUG
  if (TempPath != NULL) { // Line 1549
    D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1550
    D2U_UTF8_FPRINTF(stderr, _("using %s as temporary file\n"), TempPath); // Line 1551
  } // Line 1552
#endif

  if (!RetVal) // Line 1555
    if (check_unicode(InF, TempF, ipFlag, ipInFN, progname)) // Line 1556
      RetVal = -1; // Line 1557

  /* conversion successful? */ // Line 1559
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1561
    if ((!RetVal) && (ConvertW(InF, TempF, ipFlag, progname))) // Line 1562
      RetVal = -1; // Line 1563
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) { // Line 1564
      if (!ipFlag->error) ipFlag->error = 1; // Line 1565
      RetVal = -1; // Line 1566
    } // Line 1567
  } else { // Line 1568
    if ((!RetVal) && (Convert(InF, TempF, ipFlag, progname))) // Line 1569
      RetVal = -1; // Line 1570
  } // Line 1571
#else
  if ((!RetVal) && (Convert(InF, TempF, ipFlag, progname))) // Line 1573
    RetVal = -1; // Line 1574
#endif

   /* can close in file? */ // Line 1577
  if (d2u_fclose(InF, ipInFN, ipFlag, "r", progname) == EOF) // Line 1578
    RetVal = -1; // Line 1579

  /* can close output file? */ // Line 1581
  if (TempF) { // Line 1582
    if (d2u_fclose(TempF, TempPath, ipFlag, "w", progname) == EOF) // Line 1583
      RetVal = -1; // Line 1584
  } // Line 1585

#ifndef NO_CHMOD
  if (!RetVal) // Line 1588
  { // Line 1589
    if (ipFlag->NewFile == 0) { /* old-file mode */ // Line 1590
       RetVal = chmod (TempPath, StatBuf.st_mode); /* set original permissions */ // Line 1591
    } else { // Line 1592
       mask = umask(0); /* get process's umask */ // Line 1593
       umask(mask); /* set umask back to original */ // Line 1594
       RetVal = chmod(TempPath, StatBuf.st_mode & ~mask); /* set original permissions, minus umask */ // Line 1595
    } // Line 1596

    if (RetVal) { // Line 1598
       if (ipFlag->verbose) { // Line 1599
         ipFlag->error = errno; // Line 1600
         errstr = strerror(errno); // Line 1601
         D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1602
         D2U_UTF8_FPRINTF(stderr, _("Failed to change the permissions of temporary output file %s:"), TempPath); // Line 1603
         D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1604
       } // Line 1605
    } // Line 1606
  } // Line 1607
#endif

#ifndef NO_CHOWN
  if (!RetVal && (ipFlag->NewFile == 0)) { /* old-file mode */ // Line 1611
     /* Change owner and group of the temporary output file to the original file's uid and gid. */ // Line 1612
     /* Required when a different user (e.g. root) has write permission on the original file. */ // Line 1613
     /* Make sure that the original owner can still access the file. */ // Line 1614
     if (chown(TempPath, StatBuf.st_uid, StatBuf.st_gid)) { // Line 1615
        if (ipFlag->AllowChown) { // Line 1616
          if (ipFlag->verbose) { // Line 1617
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1618
            D2U_UTF8_FPRINTF(stderr, _("The user and/or group ownership of file %s is not preserved.\n"), ipOutFN); // Line 1619
          } // Line 1620
#ifndef NO_CHMOD
          /* Set read/write permissions same as in new file mode. */ // Line 1622
          mask = umask(0); /* get process's umask */ // Line 1623
          umask(mask); /* set umask back to original */ // Line 1624
          RetVal = chmod(TempPath, StatBuf.st_mode & ~mask); /* set original permissions, minus umask */ // Line 1625
          if (RetVal) { // Line 1626
             if (ipFlag->verbose) { // Line 1627
               ipFlag->error = errno; // Line 1628
               errstr = strerror(errno); // Line 1629
               D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1630
               D2U_UTF8_FPRINTF(stderr, _("Failed to change the permissions of temporary output file %s:"), TempPath); // Line 1631
               D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1632
             } // Line 1633
          } // Line 1634
#endif
        } else { // Line 1636
          if (ipFlag->verbose) { // Line 1637
            ipFlag->error = errno; // Line 1638
            errstr = strerror(errno); // Line 1639
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1640
            D2U_UTF8_FPRINTF(stderr, _("Failed to change the owner and group of temporary output file %s:"), TempPath); // Line 1641
            D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1642
          } // Line 1643
          RetVal = -1; // Line 1644
        } // Line 1645
     } // Line 1646
  } // Line 1647
#endif

  if ((!RetVal) && (ipFlag->KeepDate)) // Line 1650
  { // Line 1651
    UTimeBuf.actime = StatBuf.st_atime; // Line 1652
    UTimeBuf.modtime = StatBuf.st_mtime; // Line 1653
    /* can change output file time to in file time? */ // Line 1654
    if (utime(TempPath, &UTimeBuf) == -1) { // Line 1655
      if (ipFlag->verbose) { // Line 1656
        ipFlag->error = errno; // Line 1657
        errstr = strerror(errno); // Line 1658
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, TempPath); // Line 1659
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1660
      } // Line 1661
      RetVal = -1; // Line 1662
    } // Line 1663
  } // Line 1664

  /* any error? cleanup the temp file */ // Line 1666
  if (RetVal && (TempPath != NULL)) { // Line 1667
    if (d2u_unlink(TempPath) && (errno != ENOENT)) { // Line 1668
      if (ipFlag->verbose) { // Line 1669
        ipFlag->error = errno; // Line 1670
        errstr = strerror(errno); // Line 1671
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, TempPath); // Line 1672
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1673
      } // Line 1674
      RetVal = -1; // Line 1675
    } // Line 1676
  } // Line 1677

  /* can rename temporary file to output file? */ // Line 1679
  if (!RetVal) { // Line 1680
#ifdef NEED_REMOVE
    if (d2u_unlink(TargetFN) && (errno != ENOENT)) { // Line 1682
      if (ipFlag->verbose) { // Line 1683
        ipFlag->error = errno; // Line 1684
        errstr = strerror(errno); // Line 1685
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, TargetFN); // Line 1686
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1687
      } // Line 1688
      RetVal = -1; // Line 1689
    } // Line 1690
#endif

    if (d2u_rename(TempPath, TargetFN) != 0) { // Line 1693
      if (ipFlag->verbose) { // Line 1694
        ipFlag->error = errno; // Line 1695
        errstr = strerror(errno); // Line 1696
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1697
        D2U_UTF8_FPRINTF(stderr, _("problems renaming '%s' to '%s':"), TempPath, TargetFN); // Line 1698
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1699
#ifdef S_ISLNK
        if (ResolveSymlinkResult > 0) // Line 1701
          D2U_UTF8_FPRINTF(stderr, _("          which is the target of symbolic link '%s'\n"), ipOutFN); // Line 1702
#endif
        D2U_UTF8_FPRINTF(stderr, _("          output file remains in '%s'\n"), TempPath); // Line 1704
      } // Line 1705
      RetVal = -1; // Line 1706
    } // Line 1707

    if (ResolveSymlinkResult > 0) // Line 1709
      free(TargetFN); // Line 1710
  } // Line 1711
  free(TempPath); // Line 1712
  return RetVal; // Line 1713
} // Line 1714

/* convert file ipInFN and write to file ipOutFN // Line 1716
 * returns: 0 if success // Line 1717
 *         -1 otherwise // Line 1718
 */ // Line 1719
int ConvertToStdout(char *ipInFN, CFlag *ipFlag, const char *progname, // Line 1720
                   int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 1721
#ifdef D2U_UNICODE
                 , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 1723
#endif
                  ) // Line 1725
{ // Line 1726
  int RetVal = 0; // Line 1727
  FILE *InF = NULL; // Line 1728
  const char *errstr; // Line 1729

  ipFlag->status = 0 ; // Line 1731

  /* Test if input file is a regular file or symbolic link */ // Line 1733
  if (regfile(ipInFN, 1, ipFlag, progname)) { // Line 1734
    ipFlag->status |= NO_REGFILE ; // Line 1735
    /* Not a failure, skipping non-regular input file according spec. */ // Line 1736
    return -1; // Line 1737
  } // Line 1738

  /* Test if input file target is a regular file */ // Line 1740
  if (symbolic_link(ipInFN) && regfile_target(ipInFN, ipFlag,progname)) { // Line 1741
    ipFlag->status |= INPUT_TARGET_NO_REGFILE ; // Line 1742
    /* Not a failure, skipping non-regular input file according spec. */ // Line 1743
    return -1; // Line 1744
  } // Line 1745

  /* can open in file? */ // Line 1747
  InF=OpenInFile(ipInFN); // Line 1748
  if (InF == NULL) { // Line 1749
    if (ipFlag->verbose) { // Line 1750
      ipFlag->error = errno; // Line 1751
      errstr = strerror(errno); // Line 1752
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, ipInFN); // Line 1753
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1754
    } // Line 1755
    return -1; // Line 1756
  } // Line 1757

#if defined(_WIN32) && !defined(__CYGWIN__)

    /* stdin and stdout are by default text streams. We need // Line 1761
     * to set them to binary mode. Otherwise an LF will // Line 1762
     * automatically be converted to CR-LF on DOS/Windows. // Line 1763
     * Erwin */ // Line 1764

    /* POSIX 'setmode' was deprecated by MicroSoft since // Line 1766
     * Visual C++ 2005. Use ISO C++ conformant '_setmode' instead. */ // Line 1767

    _setmode(_fileno(stdout), _O_BINARY); // Line 1769
#elif defined(__MSDOS__) || defined(__CYGWIN__) || defined(__OS2__)
    setmode(fileno(stdout), O_BINARY); // Line 1771
#endif

  if (!RetVal) // Line 1774
    if (check_unicode(InF, stdout, ipFlag, ipInFN, progname)) // Line 1775
      RetVal = -1; // Line 1776

  /* conversion successful? */ // Line 1778
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1780
    if ((!RetVal) && (ConvertW(InF, stdout, ipFlag, progname))) // Line 1781
      RetVal = -1; // Line 1782
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) { // Line 1783
      if (!ipFlag->error) ipFlag->error = 1; // Line 1784
      RetVal = -1; // Line 1785
    } // Line 1786
  } else { // Line 1787
    if ((!RetVal) && (Convert(InF, stdout, ipFlag, progname))) // Line 1788
      RetVal = -1; // Line 1789
  } // Line 1790
#else
  if ((!RetVal) && (Convert(InF, stdout, ipFlag, progname))) // Line 1792
    RetVal = -1; // Line 1793
#endif

   /* can close in file? */ // Line 1796
  if (d2u_fclose(InF, ipInFN, ipFlag, "r", progname) == EOF) // Line 1797
    RetVal = -1; // Line 1798

  return RetVal; // Line 1800
} // Line 1801

/* convert stdin and write to stdout // Line 1803
 * returns: 0 if success // Line 1804
 *         -1 otherwise // Line 1805
 */ // Line 1806
int ConvertStdio(CFlag *ipFlag, const char *progname, // Line 1807
                   int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 1808
#ifdef D2U_UNICODE
                 , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 1810
#endif
                  ) // Line 1812
{ // Line 1813
    ipFlag->NewFile = 1; // Line 1814
    ipFlag->KeepDate = 0; // Line 1815

#if defined(_WIN32) && !defined(__CYGWIN__)

    /* stdin and stdout are by default text streams. We need // Line 1819
     * to set them to binary mode. Otherwise an LF will // Line 1820
     * automatically be converted to CR-LF on DOS/Windows. // Line 1821
     * Erwin */ // Line 1822

    /* POSIX 'setmode' was deprecated by MicroSoft since // Line 1824
     * Visual C++ 2005. Use ISO C++ conformant '_setmode' instead. */ // Line 1825

    _setmode(_fileno(stdout), _O_BINARY); // Line 1827
    _setmode(_fileno(stdin), _O_BINARY); // Line 1828
#elif defined(__MSDOS__) || defined(__CYGWIN__) || defined(__OS2__)
    setmode(fileno(stdout), O_BINARY); // Line 1830
    setmode(fileno(stdin), O_BINARY); // Line 1831
#endif

    if (check_unicode(stdin, stdout, ipFlag, "stdin", progname)) // Line 1834
        return -1; // Line 1835

#ifdef D2U_UNICODE
    if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1838
        return ConvertW(stdin, stdout, ipFlag, progname); // Line 1839
    } else { // Line 1840
        return Convert(stdin, stdout, ipFlag, progname); // Line 1841
    } // Line 1842
#else
    return Convert(stdin, stdout, ipFlag, progname); // Line 1844
#endif
} // Line 1846

void print_messages_stdio(const CFlag *pFlag, const char *progname) // Line 1848
{ // Line 1849
    if (pFlag->status & BINARY_FILE) { // Line 1850
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1851
      D2U_UTF8_FPRINTF(stderr, _("Skipping binary file %s\n"), "stdin"); // Line 1852
    } else if (pFlag->status & WRONG_CODEPAGE) { // Line 1853
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1854
      D2U_UTF8_FPRINTF(stderr, _("code page %d is not supported.\n"), pFlag->ConvMode); // Line 1855
#ifdef D2U_UNICODE
    } else if (pFlag->status & WCHAR_T_TOO_SMALL) { // Line 1857
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1858
      D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, the size of wchar_t is %d bytes.\n"), "stdin", (int)sizeof(wchar_t)); // Line 1859
    } else if (pFlag->status & UNICODE_CONVERSION_ERROR) { // Line 1860
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1861
      D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, an UTF-16 conversion error occurred on line %u.\n"), "stdin", pFlag->line_nr); // Line 1862
#else
    } else if (pFlag->status & UNICODE_NOT_SUPPORTED) { // Line 1864
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1865
      D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, UTF-16 conversion is not supported in this version of %s.\n"), "stdin", progname); // Line 1866
#endif
    } // Line 1868
} // Line 1869

void print_format(const CFlag *pFlag, char *informat, char *outformat, size_t lin, size_t lout) // Line 1871
{ // Line 1872
  informat[0]='\0'; // Line 1873
  outformat[0]='\0'; // Line 1874

  if (pFlag->bomtype == FILE_UTF16LE) // Line 1876
    d2u_strncpy(informat,_("UTF-16LE"),lin); // Line 1877
  if (pFlag->bomtype == FILE_UTF16BE) // Line 1878
    d2u_strncpy(informat,_("UTF-16BE"),lin); // Line 1879
  informat[lin-1]='\0'; // Line 1880

#ifdef D2U_UNICODE
  if ((pFlag->bomtype == FILE_UTF16LE)||(pFlag->bomtype == FILE_UTF16BE)) { // Line 1883
#if !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__)  /* Unix, Cygwin */
    d2u_strncpy(outformat,nl_langinfo(CODESET),lout); // Line 1885
#endif

#if defined(_WIN32) && !defined(__CYGWIN__) /* Windows, not Cygwin */
    if (pFlag->locale_target == TARGET_GB18030) // Line 1889
      d2u_strncpy(outformat, _("GB18030"),lout); // Line 1890
    else // Line 1891
      d2u_strncpy(outformat, _("UTF-8"),lout); // Line 1892
#endif

    if (pFlag->keep_utf16) // Line 1895
    { // Line 1896
      if (pFlag->bomtype == FILE_UTF16LE) // Line 1897
        d2u_strncpy(outformat,_("UTF-16LE"),lout); // Line 1898
      if (pFlag->bomtype == FILE_UTF16BE) // Line 1899
        d2u_strncpy(outformat,_("UTF-16BE"),lout); // Line 1900
    } // Line 1901
    outformat[lout-1]='\0'; // Line 1902
  } // Line 1903
#endif
} // Line 1905

void print_messages(const CFlag *pFlag, const char *infile, const char *outfile, const char *progname, const int conversion_error) // Line 1907
{ // Line 1908
  char informat[32]; // Line 1909
  char outformat[64]; // Line 1910
# ifdef D2U_UNIFILE
  wchar_t informatw[32]; // Line 1912
  wchar_t outformatw[64]; // Line 1913
#endif

  print_format(pFlag, informat, outformat, sizeof(informat), sizeof(outformat)); // Line 1916

/* Change informat and outformat to UTF-8 for d2u_utf8_fprintf. */ // Line 1918
# ifdef D2U_UNIFILE
   /* The format string is encoded in the system default // Line 1920
    * Windows ANSI code page. May have been translated // Line 1921
    * by gettext. Convert it to wide characters. */ // Line 1922
   d2u_MultiByteToWideChar(CP_ACP,0, informat, -1, informatw, sizeof(informat)); // Line 1923
   d2u_MultiByteToWideChar(CP_ACP,0, outformat, -1, outformatw, sizeof(outformat)); // Line 1924
   /* then convert the format string to UTF-8 */ // Line 1925
   d2u_WideCharToMultiByte(CP_UTF8, 0, informatw, -1, informat, sizeof(informat), NULL, NULL); // Line 1926
   d2u_WideCharToMultiByte(CP_UTF8, 0, outformatw, -1, outformat, sizeof(outformat), NULL, NULL); // Line 1927
#endif

  if (pFlag->status & NO_REGFILE) { // Line 1930
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1931
    D2U_UTF8_FPRINTF(stderr, _("Skipping %s, not a regular file.\n"), infile); // Line 1932
  } else if (pFlag->status & OUTPUTFILE_SYMLINK) { // Line 1933
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1934
    if (outfile) // Line 1935
      D2U_UTF8_FPRINTF(stderr, _("Skipping %s, output file %s is a symbolic link.\n"), infile, outfile); // Line 1936
    else // Line 1937
      D2U_UTF8_FPRINTF(stderr, _("Skipping symbolic link %s.\n"), infile); // Line 1938
  } else if (pFlag->status & INPUT_TARGET_NO_REGFILE) { // Line 1939
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1940
    D2U_UTF8_FPRINTF(stderr, _("Skipping symbolic link %s, target is not a regular file.\n"), infile); // Line 1941
  } else if ((pFlag->status & OUTPUT_TARGET_NO_REGFILE) && outfile) { // Line 1942
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1943
    D2U_UTF8_FPRINTF(stderr, _("Skipping %s, target of symbolic link %s is not a regular file.\n"), infile, outfile); // Line 1944
  } else if (pFlag->status & BINARY_FILE) { // Line 1945
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1946
    D2U_UTF8_FPRINTF(stderr, _("Skipping binary file %s\n"), infile); // Line 1947
  } else if (pFlag->status & WRONG_CODEPAGE) { // Line 1948
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1949
    D2U_UTF8_FPRINTF(stderr, _("code page %d is not supported.\n"), pFlag->ConvMode); // Line 1950
#ifdef D2U_UNICODE
  } else if (pFlag->status & WCHAR_T_TOO_SMALL) { // Line 1952
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1953
    D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, the size of wchar_t is %d bytes.\n"), infile, (int)sizeof(wchar_t)); // Line 1954
  } else if (pFlag->status & UNICODE_CONVERSION_ERROR) { // Line 1955
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1956
    D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, an UTF-16 conversion error occurred on line %u.\n"), infile, pFlag->line_nr); // Line 1957
#else
  } else if (pFlag->status & UNICODE_NOT_SUPPORTED) { // Line 1959
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1960
    D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, UTF-16 conversion is not supported in this version of %s.\n"), infile, progname); // Line 1961
#endif
  } else { // Line 1963
    if (!conversion_error) { // Line 1964
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1965
      if (informat[0] == '\0') { // Line 1966
        if (is_dos2unix(progname)) { // Line 1967
          if (outfile) // Line 1968
            D2U_UTF8_FPRINTF(stderr, _("converting file %s to file %s in Unix format...\n"), infile, outfile); // Line 1969
          else // Line 1970
            D2U_UTF8_FPRINTF(stderr, _("converting file %s to Unix format...\n"), infile); // Line 1971
        } else { // Line 1972
          if (pFlag->FromToMode == FROMTO_UNIX2MAC) { // Line 1973
            if (outfile) // Line 1974
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to file %s in Mac format...\n"), infile, outfile); // Line 1975
            else // Line 1976
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to Mac format...\n"), infile); // Line 1977
          } else { // Line 1978
            if (outfile) // Line 1979
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to file %s in DOS format...\n"), infile, outfile); // Line 1980
            else // Line 1981
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to DOS format...\n"), infile); // Line 1982
          } // Line 1983
        } // Line 1984
      } else { // Line 1985
        if (is_dos2unix(progname)) { // Line 1986
          if (outfile) // Line 1987
    /* TRANSLATORS: // Line 1988
1st %s is encoding of input file. // Line 1989
2nd %s is name of input file. // Line 1990
3rd %s is encoding of output file. // Line 1991
4th %s is name of output file. // Line 1992
E.g.: converting UTF-16LE file in.txt to UTF-8 file out.txt in Unix format... */ // Line 1993
            D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s file %s in Unix format...\n"), informat, infile, outformat, outfile); // Line 1994
          else // Line 1995
    /* TRANSLATORS: // Line 1996
1st %s is encoding of input file. // Line 1997
2nd %s is name of input file. // Line 1998
3rd %s is encoding of output (input file is overwritten). // Line 1999
E.g.: converting UTF-16LE file foo.txt to UTF-8 Unix format... */ // Line 2000
            D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s Unix format...\n"), informat, infile, outformat); // Line 2001
        } else { // Line 2002
          if (pFlag->FromToMode == FROMTO_UNIX2MAC) { // Line 2003
            if (outfile) // Line 2004
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s file %s in Mac format...\n"), informat, infile, outformat, outfile); // Line 2005
            else // Line 2006
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s Mac format...\n"), informat, infile, outformat); // Line 2007
          } else { // Line 2008
            if (outfile) // Line 2009
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s file %s in DOS format...\n"), informat, infile, outformat, outfile); // Line 2010
            else // Line 2011
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s DOS format...\n"), informat, infile, outformat); // Line 2012
          } // Line 2013
        } // Line 2014
      } // Line 2015
    } else { // Line 2016
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2017
      if (outfile) // Line 2018
        D2U_UTF8_FPRINTF(stderr, _("problems converting file %s to file %s\n"), infile, outfile); // Line 2019
      else // Line 2020
        D2U_UTF8_FPRINTF(stderr, _("problems converting file %s\n"), infile); // Line 2021
    } // Line 2022
  } // Line 2023
} // Line 2024

void print_messages_info(const CFlag *pFlag, const char *infile, const char *progname) // Line 2026
{ // Line 2027
  if (pFlag->status & NO_REGFILE) { // Line 2028
    if (pFlag->verbose) { // Line 2029
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2030
      D2U_UTF8_FPRINTF(stderr, _("Skipping %s, not a regular file.\n"), infile); // Line 2031
    } // Line 2032
  } else if (pFlag->status & INPUT_TARGET_NO_REGFILE) { // Line 2033
    if (pFlag->verbose) { // Line 2034
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2035
      D2U_UTF8_FPRINTF(stderr, _("Skipping symbolic link %s, target is not a regular file.\n"), infile); // Line 2036
    } // Line 2037
#ifdef D2U_UNICODE
  } else if (pFlag->status & WCHAR_T_TOO_SMALL) { // Line 2039
    if (pFlag->verbose) { // Line 2040
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2041
      D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, the size of wchar_t is %d bytes.\n"), infile, (int)sizeof(wchar_t)); // Line 2042
    } // Line 2043
#endif
  } // Line 2045
} // Line 2046

void printInfo(CFlag *ipFlag, const char *filename, int bomtype, unsigned int lb_dos, unsigned int lb_unix, unsigned int lb_mac, int last_eol) // Line 2048
{ // Line 2049
  static int header_done = 0; // Line 2050
  char eol[6]; // Line 2051

  if (ipFlag->file_info & INFO_CONVERT) { // Line 2053
    if ((ipFlag->FromToMode == FROMTO_DOS2UNIX) && (lb_dos == 0) && (! ipFlag->add_eol || last_eol == INFO_UNIX )) // Line 2054
      return; // Line 2055
    if ((ipFlag->FromToMode == FROMTO_UNIX2DOS) && (lb_unix == 0) && (! ipFlag->add_eol || last_eol == INFO_DOS )) // Line 2056
      return; // Line 2057
    if ((ipFlag->FromToMode == FROMTO_UNIX2MAC) && (lb_unix == 0) && (! ipFlag->add_eol || last_eol == INFO_MAC )) // Line 2058
      return; // Line 2059
    if ((ipFlag->FromToMode == FROMTO_MAC2UNIX) && (lb_mac == 0) && (! ipFlag->add_eol || last_eol == INFO_UNIX )) // Line 2060
      return; // Line 2061
    if ((ipFlag->Force == 0) && (ipFlag->status & BINARY_FILE)) // Line 2062
      return; // Line 2063
  } // Line 2064

  if ((ipFlag->file_info & INFO_HEADER) && (! header_done)) { // Line 2066
    if (ipFlag->file_info & INFO_DOS) // Line 2067
      D2U_UTF8_FPRINTF(stdout, "     DOS"); // Line 2068
    if (ipFlag->file_info & INFO_UNIX) // Line 2069
      D2U_UTF8_FPRINTF(stdout, "    UNIX"); // Line 2070
    if (ipFlag->file_info & INFO_MAC) // Line 2071
      D2U_UTF8_FPRINTF(stdout, "     MAC"); // Line 2072
    if (ipFlag->file_info & INFO_BOM) // Line 2073
      D2U_UTF8_FPRINTF(stdout, "  BOM     "); // Line 2074
    if (ipFlag->file_info & INFO_TEXT) // Line 2075
      D2U_UTF8_FPRINTF(stdout, "  TXTBIN"); // Line 2076
    if ((ipFlag->add_eol && !(ipFlag->file_info & INFO_CONVERT)) || ipFlag->file_info & INFO_EOL) // Line 2077
      D2U_UTF8_FPRINTF(stdout, " LASTLN"); // Line 2078
    if (*filename != '\0') { // Line 2079
      if ((ipFlag->file_info & INFO_DEFAULT) || (ipFlag->file_info & INFO_EOL)) // Line 2080
        D2U_UTF8_FPRINTF(stdout, "  "); // Line 2081
      D2U_UTF8_FPRINTF(stdout, "FILE"); // Line 2082
    } // Line 2083
    if (ipFlag->file_info & INFO_PRINT0) // Line 2084
      (void) fputc(0, stdout); // Line 2085
    else // Line 2086
      D2U_UTF8_FPRINTF(stdout, "\n"); // Line 2087
    header_done = 1; // Line 2088
  } // Line 2089

  switch (last_eol) { // Line 2091
    case INFO_DOS: // Line 2092
      strncpy(eol,"dos  ",sizeof(eol)); // Line 2093
      break; // Line 2094
    case INFO_UNIX: // Line 2095
      strncpy(eol,"unix ",sizeof(eol)); // Line 2096
      break; // Line 2097
    case INFO_MAC: // Line 2098
      strncpy(eol,"mac  ",sizeof(eol)); // Line 2099
      break; // Line 2100
    default: // Line 2101
      strncpy(eol,"noeol",sizeof(eol)); // Line 2102
  } // Line 2103

  if (ipFlag->file_info & INFO_DOS) // Line 2105
    D2U_UTF8_FPRINTF(stdout, "  %6u", lb_dos); // Line 2106
  if (ipFlag->file_info & INFO_UNIX) // Line 2107
    D2U_UTF8_FPRINTF(stdout, "  %6u", lb_unix); // Line 2108
  if (ipFlag->file_info & INFO_MAC) // Line 2109
    D2U_UTF8_FPRINTF(stdout, "  %6u", lb_mac); // Line 2110
  if (ipFlag->file_info & INFO_BOM) // Line 2111
    print_bom_info(bomtype); // Line 2112
  if (ipFlag->file_info & INFO_TEXT) { // Line 2113
    if (ipFlag->status & BINARY_FILE) // Line 2114
      D2U_UTF8_FPRINTF(stdout, "  binary"); // Line 2115
    else // Line 2116
      D2U_UTF8_FPRINTF(stdout, "  text  "); // Line 2117
  } // Line 2118
  if ((ipFlag->add_eol && !(ipFlag->file_info & INFO_CONVERT)) || ipFlag->file_info & INFO_EOL) // Line 2119
    D2U_UTF8_FPRINTF(stdout, " %s ", eol); // Line 2120
  if (*filename != '\0') { // Line 2121
    const char *ptr; // Line 2122
    if ((ipFlag->file_info & INFO_NOPATH) && (((ptr=strrchr(filename,'/')) != NULL) || ((ptr=strrchr(filename,'\\')) != NULL)) ) // Line 2123
      ptr++; // Line 2124
    else // Line 2125
      ptr = filename; // Line 2126
    if ((ipFlag->file_info & INFO_DEFAULT) || (ipFlag->file_info & INFO_EOL)) // Line 2127
      D2U_UTF8_FPRINTF(stdout, "  "); // Line 2128
    D2U_UTF8_FPRINTF(stdout, "%s",ptr); // Line 2129
  } // Line 2130
  if (ipFlag->file_info & INFO_PRINT0) // Line 2131
    (void) fputc(0, stdout); // Line 2132
  else // Line 2133
    D2U_UTF8_FPRINTF(stdout, "\n"); // Line 2134
} // Line 2135

#ifdef D2U_UNICODE
void FileInfoW(FILE* ipInF, CFlag *ipFlag, const char *filename, int bomtype, const char *progname) // Line 2138
{ // Line 2139
  wint_t TempChar; // Line 2140
  wint_t PreviousChar = 0; // Line 2141
  unsigned int lb_dos = 0; // Line 2142
  unsigned int lb_unix = 0; // Line 2143
  unsigned int lb_mac = 0; // Line 2144
  int last_eol = 0; // Line 2145

  ipFlag->status = 0; // Line 2147

  while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 2149
    if ( (TempChar < 32) && // Line 2150
        (TempChar != 0x0a) &&  /* Not an LF */ // Line 2151
        (TempChar != 0x0d) &&  /* Not a CR */ // Line 2152
        (TempChar != 0x09) &&  /* Not a TAB */ // Line 2153
        (TempChar != 0x0c)) {  /* Not a form feed */ // Line 2154
      ipFlag->status |= BINARY_FILE ; // Line 2155
    } // Line 2156
    if (TempChar != 0x0a) { /* Not an LF */ // Line 2157
      PreviousChar = TempChar; // Line 2158
      if (TempChar == 0x0d) { /* CR */ // Line 2159
        lb_mac++; // Line 2160
        last_eol = INFO_MAC; // Line 2161
      } else { // Line 2162
        last_eol = 0; // Line 2163
      } // Line 2164
    } else{ // Line 2165
      /* TempChar is an LF */ // Line 2166
      if ( PreviousChar == 0x0d ) { /* CR,LF pair. */ // Line 2167
        lb_dos++; // Line 2168
        lb_mac--; // Line 2169
        last_eol = INFO_DOS; // Line 2170
        PreviousChar = TempChar; // Line 2171
        continue; // Line 2172
      } // Line 2173
      PreviousChar = TempChar; // Line 2174
      lb_unix++; /* Unix line end (LF). */ // Line 2175
      last_eol = INFO_UNIX; // Line 2176
    } // Line 2177
  } // Line 2178
  if ((TempChar == WEOF) && ferror(ipInF)) { // Line 2179
    ipFlag->error = errno; // Line 2180
    if (ipFlag->verbose) { // Line 2181
      const char *errstr = strerror(errno); // Line 2182
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2183
      D2U_UTF8_FPRINTF(stderr, _("can not read from input file %s:"), filename); // Line 2184
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 2185
    } // Line 2186
    return; // Line 2187
  } // Line 2188

  printInfo(ipFlag, filename, bomtype, lb_dos, lb_unix, lb_mac, last_eol); // Line 2190

} // Line 2192
#endif

void FileInfo(FILE* ipInF, CFlag *ipFlag, const char *filename, int bomtype, const char *progname) // Line 2195
{ // Line 2196
  int TempChar; // Line 2197
  int PreviousChar = 0; // Line 2198
  unsigned int lb_dos = 0; // Line 2199
  unsigned int lb_unix = 0; // Line 2200
  unsigned int lb_mac = 0; // Line 2201
  int last_eol = 0; // Line 2202

  ipFlag->status = 0; // Line 2204

  while ((TempChar = fgetc(ipInF)) != EOF) { // Line 2206
    if ( (TempChar < 32) && // Line 2207
        (TempChar != '\x0a') &&  /* Not an LF */ // Line 2208
        (TempChar != '\x0d') &&  /* Not a CR */ // Line 2209
        (TempChar != '\x09') &&  /* Not a TAB */ // Line 2210
        (TempChar != '\x0c')) {  /* Not a form feed */ // Line 2211
      ipFlag->status |= BINARY_FILE ; // Line 2212
      } // Line 2213
    if (TempChar != '\x0a') { /* Not an LF */ // Line 2214
      PreviousChar = TempChar; // Line 2215
      if (TempChar == '\x0d') { /* CR */ // Line 2216
        lb_mac++; // Line 2217
        last_eol = INFO_MAC; // Line 2218
      } else { // Line 2219
        last_eol = 0; // Line 2220
      } // Line 2221
    } else { // Line 2222
      /* TempChar is an LF */ // Line 2223
      if ( PreviousChar == '\x0d' ) { /* CR,LF pair. */ // Line 2224
        lb_dos++; // Line 2225
        lb_mac--; // Line 2226
        last_eol = INFO_DOS; // Line 2227
        PreviousChar = TempChar; // Line 2228
        continue; // Line 2229
      } // Line 2230
      PreviousChar = TempChar; // Line 2231
      lb_unix++; /* Unix line end (LF). */ // Line 2232
      last_eol = INFO_UNIX; // Line 2233
    } // Line 2234
  } // Line 2235
  if ((TempChar == EOF) && ferror(ipInF)) { // Line 2236
    ipFlag->error = errno; // Line 2237
    if (ipFlag->verbose) { // Line 2238
      const char *errstr = strerror(errno); // Line 2239
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2240
      D2U_UTF8_FPRINTF(stderr, _("can not read from input file %s:"), filename); // Line 2241
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 2242
    } // Line 2243
    return; // Line 2244
  } // Line 2245

  printInfo(ipFlag, filename, bomtype, lb_dos, lb_unix, lb_mac, last_eol); // Line 2247
} // Line 2248

int GetFileInfo(char *ipInFN, CFlag *ipFlag, const char *progname) // Line 2250
{ // Line 2251
  FILE *InF = NULL; // Line 2252
  int bomtype_orig = FILE_MBS; /* messages must print the real bomtype, not the assumed bomtype */ // Line 2253

  ipFlag->status = 0 ; // Line 2255

  /* Test if input file is a regular file or symbolic link */ // Line 2257
  if (regfile(ipInFN, 1, ipFlag, progname)) { // Line 2258
    ipFlag->status |= NO_REGFILE ; // Line 2259
    /* Not a failure, skipping non-regular input file according spec. */ // Line 2260
    return -1; // Line 2261
  } // Line 2262

  /* Test if input file target is a regular file */ // Line 2264
  if (symbolic_link(ipInFN) && regfile_target(ipInFN, ipFlag,progname)) { // Line 2265
    ipFlag->status |= INPUT_TARGET_NO_REGFILE ; // Line 2266
    /* Not a failure, skipping non-regular input file according spec. */ // Line 2267
    return -1; // Line 2268
  } // Line 2269


  /* can open in file? */ // Line 2272
  InF=OpenInFile(ipInFN); // Line 2273
  if (InF == NULL) { // Line 2274
    if (ipFlag->verbose) { // Line 2275
      const char *errstr = strerror(errno); // Line 2276
      ipFlag->error = errno; // Line 2277
      D2U_UTF8_FPRINTF(stderr, "%s: %s: ", progname, ipInFN); // Line 2278
      D2U_ANSI_FPRINTF(stderr, "%s\n", errstr); // Line 2279
    } // Line 2280
    return -1; // Line 2281
  } // Line 2282


  if (check_unicode_info(InF, ipFlag, progname, &bomtype_orig)) { // Line 2285
    d2u_fclose(InF, ipInFN, ipFlag, "r", progname); // Line 2286
    return -1; // Line 2287
  } // Line 2288

  /* info successful? */ // Line 2290
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 2292
    FileInfoW(InF, ipFlag, ipInFN, bomtype_orig, progname); // Line 2293
  } else { // Line 2294
    FileInfo(InF, ipFlag, ipInFN, bomtype_orig, progname); // Line 2295
  } // Line 2296
#else
  FileInfo(InF, ipFlag, ipInFN, bomtype_orig, progname); // Line 2298
#endif

  /* can close in file? */ // Line 2301
  if (d2u_fclose(InF, ipInFN, ipFlag, "r", progname) == EOF) // Line 2302
    return -1; // Line 2303

  return 0; // Line 2305
} // Line 2306

int GetFileInfoStdio(CFlag *ipFlag, const char *progname) // Line 2308
{ // Line 2309
  int bomtype_orig = FILE_MBS; /* messages must print the real bomtype, not the assumed bomtype */ // Line 2310

  ipFlag->status = 0 ; // Line 2312

#if defined(_WIN32) && !defined(__CYGWIN__)

    /* stdin and stdout are by default text streams. We need // Line 2316
     * to set them to binary mode. Otherwise an LF will // Line 2317
     * automatically be converted to CR-LF on DOS/Windows. // Line 2318
     * Erwin */ // Line 2319

    /* POSIX 'setmode' was deprecated by MicroSoft since // Line 2321
     * Visual C++ 2005. Use ISO C++ conformant '_setmode' instead. */ // Line 2322

    _setmode(_fileno(stdin), _O_BINARY); // Line 2324
#elif defined(__MSDOS__) || defined(__CYGWIN__) || defined(__OS2__)
    setmode(fileno(stdin), O_BINARY); // Line 2326
#endif

  if (check_unicode_info(stdin, ipFlag, progname, &bomtype_orig)) // Line 2329
    return -1; // Line 2330

  /* info successful? */ // Line 2332
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 2334
    FileInfoW(stdin, ipFlag, "", bomtype_orig, progname); // Line 2335
  } else { // Line 2336
    FileInfo(stdin, ipFlag, "", bomtype_orig, progname); // Line 2337
  } // Line 2338
#else
  FileInfo(stdin, ipFlag, "", bomtype_orig, progname); // Line 2340
#endif

  return 0; // Line 2343
} // Line 2344

void get_info_options(char *option, CFlag *pFlag, const char *progname) // Line 2346
{ // Line 2347
  char *ptr; // Line 2348
  int default_info = 1; // Line 2349

  ptr = option; // Line 2351

  if (*ptr == '\0') { /* no flags */ // Line 2353
    pFlag->file_info |= INFO_DEFAULT; // Line 2354
    return; // Line 2355
  } // Line 2356

  while (*ptr != '\0') { // Line 2358
    switch (*ptr) { // Line 2359
      case '0':   /* Print null characters instead of newline characters. */ // Line 2360
        pFlag->file_info |= INFO_PRINT0; // Line 2361
        break; // Line 2362
      case 'd':   /* Print nr of DOS line breaks. */ // Line 2363
        pFlag->file_info |= INFO_DOS; // Line 2364
        default_info = 0; // Line 2365
        break; // Line 2366
      case 'u':   /* Print nr of Unix line breaks. */ // Line 2367
        pFlag->file_info |= INFO_UNIX; // Line 2368
        default_info = 0; // Line 2369
        break; // Line 2370
      case 'm':   /* Print nr of Mac line breaks. */ // Line 2371
        pFlag->file_info |= INFO_MAC; // Line 2372
        default_info = 0; // Line 2373
        break; // Line 2374
      case 'b':   /* Print BOM. */ // Line 2375
        pFlag->file_info |= INFO_BOM; // Line 2376
        default_info = 0; // Line 2377
        break; // Line 2378
      case 't':   /* Text or binary. */ // Line 2379
        pFlag->file_info |= INFO_TEXT; // Line 2380
        default_info = 0; // Line 2381
        break; // Line 2382
      case 'e':   /* Print EOL of last line. */ // Line 2383
        pFlag->file_info |= INFO_EOL; // Line 2384
        default_info = 0; // Line 2385
        break; // Line 2386
      case 'c':   /* Print only files that would be converted. */ // Line 2387
        pFlag->file_info |= INFO_CONVERT; // Line 2388
        default_info = 0; // Line 2389
        break; // Line 2390
      case 'h':   /* Print a header. */ // Line 2391
        pFlag->file_info |= INFO_HEADER; // Line 2392
        break; // Line 2393
      case 'p':   /* Remove path from file names. */ // Line 2394
        pFlag->file_info |= INFO_NOPATH; // Line 2395
        break; // Line 2396
      default: // Line 2397
       /* Terminate the program on a wrong option. If pFlag->file_info is // Line 2398
          zero and the program goes on, it may do unwanted conversions. */ // Line 2399
        D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2400
        D2U_UTF8_FPRINTF(stderr,_("wrong flag '%c' for option -i or --info\n"), *ptr); // Line 2401
        exit(1); // Line 2402
      ; // Line 2403
    } // Line 2404
    ptr++; // Line 2405
  } // Line 2406
  if (default_info) // Line 2407
    pFlag->file_info |= INFO_DEFAULT; // Line 2408
} // Line 2409

int parse_options(int argc, char *argv[], // Line 2411
                  CFlag *pFlag, const char *localedir, const char *progname, // Line 2412
                  void (*PrintLicense)(void), // Line 2413
                  int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 2414
#ifdef D2U_UNICODE
                , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 2416
#endif
                  ) // Line 2418
{ // Line 2419
  int ArgIdx = 0; // Line 2420
  int ShouldExit = 0; // Line 2421
  int CanSwitchFileMode = 1; // Line 2422
  int process_options = 1; // Line 2423
#ifdef D2U_UNIFILE
  char *ptr; // Line 2425
#endif

  /* variable initialisations */ // Line 2428
  pFlag->NewFile = 0; // Line 2429
  pFlag->verbose = 1; // Line 2430
  pFlag->KeepDate = 0; // Line 2431
  pFlag->ConvMode = CONVMODE_ASCII;  /* default ascii */ // Line 2432
  pFlag->NewLine = 0; // Line 2433
  pFlag->Force = 0; // Line 2434
  pFlag->Follow = SYMLINK_SKIP; // Line 2435
  pFlag->status = 0; // Line 2436
  pFlag->stdio_mode = 1; // Line 2437
  pFlag->to_stdout = 0; // Line 2438
  pFlag->error = 0; // Line 2439
  pFlag->bomtype = FILE_MBS; // Line 2440
  pFlag->add_bom = 0; // Line 2441
  pFlag->keep_utf16 = 0; // Line 2442
  pFlag->file_info = 0; // Line 2443
  pFlag->locale_target = TARGET_UTF8; // Line 2444
  pFlag->add_eol = 0; // Line 2445

#ifdef D2U_UNIFILE
   ptr = getenv("DOS2UNIX_DISPLAY_ENC"); // Line 2448
   if (ptr != NULL) { // Line 2449
      if (strncmp(ptr, "ansi", sizeof("ansi")) == 0) // Line 2450
         d2u_display_encoding = D2U_DISPLAY_ANSI; // Line 2451
      else if (strncmp(ptr, "unicode", sizeof("unicode")) == 0) // Line 2452
         d2u_display_encoding = D2U_DISPLAY_UNICODE; // Line 2453
      else if (strncmp(ptr, "unicodebom", sizeof("unicodebom")) == 0) // Line 2454
         d2u_display_encoding = D2U_DISPLAY_UNICODEBOM; // Line 2455
      else if (strncmp(ptr, "utf8", sizeof("utf8")) == 0) // Line 2456
         d2u_display_encoding = D2U_DISPLAY_UTF8; // Line 2457
      else if (strncmp(ptr, "utf8bom", sizeof("utf8bom")) == 0) // Line 2458
         d2u_display_encoding = D2U_DISPLAY_UTF8BOM; // Line 2459
   } // Line 2460
#endif

  while ((++ArgIdx < argc) && (!ShouldExit)) // Line 2463
  { // Line 2464
    /* is it an option? */ // Line 2465
    if ((argv[ArgIdx][0] == '-') && process_options) // Line 2466
    { // Line 2467
      /* an option */ // Line 2468
      if (strcmp(argv[ArgIdx],"--") == 0) // Line 2469
        process_options = 0; // Line 2470
      else if ((strcmp(argv[ArgIdx],"-h") == 0) || (strcmp(argv[ArgIdx],"--help") == 0)) // Line 2471
      { // Line 2472
        PrintUsage(progname); // Line 2473
        return(pFlag->error); // Line 2474
      } // Line 2475
      else if ((strcmp(argv[ArgIdx],"-b") == 0) || (strcmp(argv[ArgIdx],"--keep-bom") == 0)) // Line 2476
        pFlag->keep_bom = 1; // Line 2477
      else if ((strcmp(argv[ArgIdx],"-k") == 0) || (strcmp(argv[ArgIdx],"--keepdate") == 0)) // Line 2478
        pFlag->KeepDate = 1; // Line 2479
      else if ((strcmp(argv[ArgIdx],"-e") == 0) || (strcmp(argv[ArgIdx],"--add-eol") == 0)) // Line 2480
        pFlag->add_eol = 1; // Line 2481
      else if (strcmp(argv[ArgIdx],"--no-add-eol") == 0) // Line 2482
        pFlag->add_eol = 0; // Line 2483
      else if ((strcmp(argv[ArgIdx],"-f") == 0) || (strcmp(argv[ArgIdx],"--force") == 0)) // Line 2484
        pFlag->Force = 1; // Line 2485
#ifndef NO_CHOWN
      else if (strcmp(argv[ArgIdx],"--allow-chown") == 0) // Line 2487
        pFlag->AllowChown = 1; // Line 2488
      else if (strcmp(argv[ArgIdx],"--no-allow-chown") == 0) // Line 2489
        pFlag->AllowChown = 0; // Line 2490
#endif
#ifdef D2U_UNICODE
#if (defined(_WIN32) && !defined(__CYGWIN__))
      else if ((strcmp(argv[ArgIdx],"-gb") == 0) || (strcmp(argv[ArgIdx],"--gb18030") == 0)) // Line 2494
        pFlag->locale_target = TARGET_GB18030; // Line 2495
#endif
#endif
      else if ((strcmp(argv[ArgIdx],"-s") == 0) || (strcmp(argv[ArgIdx],"--safe") == 0)) // Line 2498
        pFlag->Force = 0; // Line 2499
      else if ((strcmp(argv[ArgIdx],"-q") == 0) || (strcmp(argv[ArgIdx],"--quiet") == 0)) // Line 2500
        pFlag->verbose = 0; // Line 2501
      else if ((strcmp(argv[ArgIdx],"-v") == 0) || (strcmp(argv[ArgIdx],"--verbose") == 0)) // Line 2502
        pFlag->verbose = 2; // Line 2503
      else if ((strcmp(argv[ArgIdx],"-l") == 0) || (strcmp(argv[ArgIdx],"--newline") == 0)) // Line 2504
        pFlag->NewLine = 1; // Line 2505
      else if ((strcmp(argv[ArgIdx],"-m") == 0) || (strcmp(argv[ArgIdx],"--add-bom") == 0)) // Line 2506
        pFlag->add_bom = 1; // Line 2507
      else if ((strcmp(argv[ArgIdx],"-r") == 0) || (strcmp(argv[ArgIdx],"--remove-bom") == 0)) { // Line 2508
        pFlag->keep_bom = 0; // Line 2509
        pFlag->add_bom = 0; // Line 2510
      } // Line 2511
      else if ((strcmp(argv[ArgIdx],"-S") == 0) || (strcmp(argv[ArgIdx],"--skip-symlink") == 0)) // Line 2512
        pFlag->Follow = SYMLINK_SKIP; // Line 2513
      else if ((strcmp(argv[ArgIdx],"-F") == 0) || (strcmp(argv[ArgIdx],"--follow-symlink") == 0)) // Line 2514
        pFlag->Follow = SYMLINK_FOLLOW; // Line 2515
      else if ((strcmp(argv[ArgIdx],"-R") == 0) || (strcmp(argv[ArgIdx],"--replace-symlink") == 0)) // Line 2516
        pFlag->Follow = SYMLINK_REPLACE; // Line 2517
      else if ((strcmp(argv[ArgIdx],"-V") == 0) || (strcmp(argv[ArgIdx],"--version") == 0)) { // Line 2518
        PrintVersion(progname, localedir); // Line 2519
        return(pFlag->error); // Line 2520
      } // Line 2521
      else if ((strcmp(argv[ArgIdx],"-L") == 0) || (strcmp(argv[ArgIdx],"--license") == 0)) { // Line 2522
        PrintLicense(); // Line 2523
        return(pFlag->error); // Line 2524
      } // Line 2525
      else if (strcmp(argv[ArgIdx],"-ascii") == 0) { /* SunOS compatible options */ // Line 2526
        pFlag->ConvMode = CONVMODE_ASCII; // Line 2527
        pFlag->keep_utf16 = 0; // Line 2528
        pFlag->locale_target = TARGET_UTF8; // Line 2529
      } // Line 2530
      else if (strcmp(argv[ArgIdx],"-7") == 0) // Line 2531
        pFlag->ConvMode = CONVMODE_7BIT; // Line 2532
      else if (strcmp(argv[ArgIdx],"-iso") == 0) { // Line 2533
        pFlag->ConvMode = (int)query_con_codepage(); // Line 2534
        if (pFlag->verbose) { // Line 2535
           D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2536
           D2U_UTF8_FPRINTF(stderr,_("active code page: %d\n"), pFlag->ConvMode); // Line 2537
        } // Line 2538
        if (pFlag->ConvMode < 2) // Line 2539
           pFlag->ConvMode = CONVMODE_437; // Line 2540
      } // Line 2541
      else if (strcmp(argv[ArgIdx],"-437") == 0) // Line 2542
        pFlag->ConvMode = CONVMODE_437; // Line 2543
      else if (strcmp(argv[ArgIdx],"-850") == 0) // Line 2544
        pFlag->ConvMode = CONVMODE_850; // Line 2545
      else if (strcmp(argv[ArgIdx],"-860") == 0) // Line 2546
        pFlag->ConvMode = CONVMODE_860; // Line 2547
      else if (strcmp(argv[ArgIdx],"-863") == 0) // Line 2548
        pFlag->ConvMode = CONVMODE_863; // Line 2549
      else if (strcmp(argv[ArgIdx],"-865") == 0) // Line 2550
        pFlag->ConvMode = CONVMODE_865; // Line 2551
      else if (strcmp(argv[ArgIdx],"-1252") == 0) // Line 2552
        pFlag->ConvMode = CONVMODE_1252; // Line 2553
#ifdef D2U_UNICODE
      else if ((strcmp(argv[ArgIdx],"-u") == 0) || (strcmp(argv[ArgIdx],"--keep-utf16") == 0)) // Line 2555
        pFlag->keep_utf16 = 1; // Line 2556
      else if ((strcmp(argv[ArgIdx],"-ul") == 0) || (strcmp(argv[ArgIdx],"--assume-utf16le") == 0)) // Line 2557
        pFlag->ConvMode = CONVMODE_UTF16LE; // Line 2558
      else if ((strcmp(argv[ArgIdx],"-ub") == 0) || (strcmp(argv[ArgIdx],"--assume-utf16be") == 0)) // Line 2559
        pFlag->ConvMode = CONVMODE_UTF16BE; // Line 2560
#endif
      else if (strcmp(argv[ArgIdx],"--info") == 0) // Line 2562
        pFlag->file_info |= INFO_DEFAULT; // Line 2563
      else if (strncmp(argv[ArgIdx],"--info=", (size_t)7) == 0) { // Line 2564
        get_info_options(argv[ArgIdx]+7, pFlag, progname); // Line 2565
      } else if (strncmp(argv[ArgIdx],"-i", (size_t)2) == 0) { // Line 2566
        get_info_options(argv[ArgIdx]+2, pFlag, progname); // Line 2567
      } else if ((strcmp(argv[ArgIdx],"-c") == 0) || (strcmp(argv[ArgIdx],"--convmode") == 0)) { // Line 2568
        if (++ArgIdx < argc) { // Line 2569
          if (strcmpi(argv[ArgIdx],"ascii") == 0) { /* Benjamin Lin's legacy options */ // Line 2570
            pFlag->ConvMode = CONVMODE_ASCII; // Line 2571
            pFlag->keep_utf16 = 0; // Line 2572
          } // Line 2573
          else if (strcmpi(argv[ArgIdx], "7bit") == 0) // Line 2574
            pFlag->ConvMode = CONVMODE_7BIT; // Line 2575
          else if (strcmpi(argv[ArgIdx], "iso") == 0) { // Line 2576
            pFlag->ConvMode = (int)query_con_codepage(); // Line 2577
            if (pFlag->verbose) { // Line 2578
               D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2579
               D2U_UTF8_FPRINTF(stderr,_("active code page: %d\n"), pFlag->ConvMode); // Line 2580
            } // Line 2581
            if (pFlag->ConvMode < 2) // Line 2582
               pFlag->ConvMode = CONVMODE_437; // Line 2583
          } // Line 2584
          else if (strcmpi(argv[ArgIdx], "mac") == 0) { // Line 2585
            if (is_dos2unix(progname)) // Line 2586
              pFlag->FromToMode = FROMTO_MAC2UNIX; // Line 2587
            else // Line 2588
              pFlag->FromToMode = FROMTO_UNIX2MAC; // Line 2589
          } else { // Line 2590
            D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2591
            D2U_UTF8_FPRINTF(stderr, _("invalid %s conversion mode specified\n"),argv[ArgIdx]); // Line 2592
            pFlag->error = 1; // Line 2593
            ShouldExit = 1; // Line 2594
            pFlag->stdio_mode = 0; // Line 2595
          } // Line 2596
        } else { // Line 2597
          ArgIdx--; // Line 2598
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2599
          D2U_UTF8_FPRINTF(stderr,_("option '%s' requires an argument\n"),argv[ArgIdx]); // Line 2600
          pFlag->error = 1; // Line 2601
          ShouldExit = 1; // Line 2602
          pFlag->stdio_mode = 0; // Line 2603
        } // Line 2604
      } // Line 2605

#ifdef D2U_UNIFILE
      else if ((strcmp(argv[ArgIdx],"-D") == 0) || (strcmp(argv[ArgIdx],"--display-enc") == 0)) { // Line 2608
        if (++ArgIdx < argc) { // Line 2609
          if (strcmpi(argv[ArgIdx],"ansi") == 0) // Line 2610
            d2u_display_encoding = D2U_DISPLAY_ANSI; // Line 2611
          else if (strcmpi(argv[ArgIdx], "unicode") == 0) // Line 2612
            d2u_display_encoding = D2U_DISPLAY_UNICODE; // Line 2613
          else if (strcmpi(argv[ArgIdx], "unicodebom") == 0) // Line 2614
            d2u_display_encoding = D2U_DISPLAY_UNICODEBOM; // Line 2615
          else if (strcmpi(argv[ArgIdx], "utf8") == 0) // Line 2616
            d2u_display_encoding = D2U_DISPLAY_UTF8; // Line 2617
          else if (strcmpi(argv[ArgIdx], "utf8bom") == 0) { // Line 2618
            d2u_display_encoding = D2U_DISPLAY_UTF8BOM; // Line 2619
          } else { // Line 2620
            D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2621
            D2U_UTF8_FPRINTF(stderr, _("invalid %s display encoding specified\n"),argv[ArgIdx]); // Line 2622
            pFlag->error = 1; // Line 2623
            ShouldExit = 1; // Line 2624
            pFlag->stdio_mode = 0; // Line 2625
          } // Line 2626
        } else { // Line 2627
          ArgIdx--; // Line 2628
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2629
          D2U_UTF8_FPRINTF(stderr,_("option '%s' requires an argument\n"),argv[ArgIdx]); // Line 2630
          pFlag->error = 1; // Line 2631
          ShouldExit = 1; // Line 2632
          pFlag->stdio_mode = 0; // Line 2633
        } // Line 2634
      } // Line 2635
#endif

      else if ((strcmp(argv[ArgIdx],"-o") == 0) || (strcmp(argv[ArgIdx],"--oldfile") == 0)) { // Line 2638
        /* last convert not paired */ // Line 2639
        if (!CanSwitchFileMode) { // Line 2640
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2641
          D2U_UTF8_FPRINTF(stderr, _("target of file %s not specified in new-file mode\n"), argv[ArgIdx-1]); // Line 2642
          pFlag->error = 1; // Line 2643
          ShouldExit = 1; // Line 2644
          pFlag->stdio_mode = 0; // Line 2645
        } // Line 2646
        pFlag->NewFile = 0; // Line 2647
        pFlag->file_info = 0; // Line 2648
        pFlag->to_stdout = 0; // Line 2649
      } // Line 2650

      else if ((strcmp(argv[ArgIdx],"-n") == 0) || (strcmp(argv[ArgIdx],"--newfile") == 0)) { // Line 2652
        /* last convert not paired */ // Line 2653
        if (!CanSwitchFileMode) { // Line 2654
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2655
          D2U_UTF8_FPRINTF(stderr, _("target of file %s not specified in new-file mode\n"), argv[ArgIdx-1]); // Line 2656
          pFlag->error = 1; // Line 2657
          ShouldExit = 1; // Line 2658
          pFlag->stdio_mode = 0; // Line 2659
        } // Line 2660
        pFlag->NewFile = 1; // Line 2661
        pFlag->file_info = 0; // Line 2662
      } // Line 2663
      else if ((strcmp(argv[ArgIdx],"-O") == 0) || (strcmp(argv[ArgIdx],"--to-stdout") == 0)) { // Line 2664
        /* last convert not paired */ // Line 2665
        if (!CanSwitchFileMode) { // Line 2666
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2667
          D2U_UTF8_FPRINTF(stderr, _("target of file %s not specified in new-file mode\n"), argv[ArgIdx-1]); // Line 2668
          pFlag->error = 1; // Line 2669
          ShouldExit = 1; // Line 2670
          pFlag->stdio_mode = 0; // Line 2671
        } // Line 2672
        pFlag->NewFile = 0; // Line 2673
        pFlag->to_stdout = 1; // Line 2674
      } // Line 2675
      else { /* wrong option */ // Line 2676
        PrintUsage(progname); // Line 2677
        ShouldExit = 1; // Line 2678
        pFlag->error = 1; // Line 2679
        pFlag->stdio_mode = 0; // Line 2680
      } // Line 2681
    } else { // Line 2682
      /* not an option */ // Line 2683
      int conversion_error; // Line 2684
      pFlag->stdio_mode = 0; // Line 2685
      if (pFlag->NewFile) { // Line 2686
        if (CanSwitchFileMode) // Line 2687
          CanSwitchFileMode = 0; // Line 2688
        else { // Line 2689
#ifdef D2U_UNICODE
          conversion_error = ConvertNewFile(argv[ArgIdx-1], argv[ArgIdx], pFlag, progname, Convert, ConvertW); // Line 2691
#else
          conversion_error = ConvertNewFile(argv[ArgIdx-1], argv[ArgIdx], pFlag, progname, Convert); // Line 2693
#endif
          if (pFlag->verbose) // Line 2695
            print_messages(pFlag, argv[ArgIdx-1], argv[ArgIdx], progname, conversion_error); // Line 2696
          CanSwitchFileMode = 1; // Line 2697
        } // Line 2698
      } else { // Line 2699
        if (pFlag->file_info) { // Line 2700
          conversion_error = GetFileInfo(argv[ArgIdx], pFlag, progname); // Line 2701
          print_messages_info(pFlag, argv[ArgIdx], progname); // Line 2702
        } else { // Line 2703
          /* Old file mode */ // Line 2704
          if (pFlag->to_stdout) { // Line 2705
#ifdef D2U_UNICODE
            conversion_error = ConvertToStdout(argv[ArgIdx], pFlag, progname, Convert, ConvertW); // Line 2707
#else
            conversion_error = ConvertToStdout(argv[ArgIdx], pFlag, progname, Convert); // Line 2709
#endif
          } else { // Line 2711
#ifdef D2U_UNICODE
            conversion_error = ConvertNewFile(argv[ArgIdx], argv[ArgIdx], pFlag, progname, Convert, ConvertW); // Line 2713
#else
            conversion_error = ConvertNewFile(argv[ArgIdx], argv[ArgIdx], pFlag, progname, Convert); // Line 2715
#endif
          } // Line 2717
          if (pFlag->verbose) // Line 2718
            print_messages(pFlag, argv[ArgIdx], NULL, progname, conversion_error); // Line 2719
        } // Line 2720
      } // Line 2721
    } // Line 2722
  } // Line 2723

  /* no file argument, use stdin and stdout */ // Line 2725
  if ( (argc > 0) && pFlag->stdio_mode) { // Line 2726
    if (pFlag->file_info) { // Line 2727
      GetFileInfoStdio(pFlag, progname); // Line 2728
      print_messages_info(pFlag, "stdin", progname); // Line 2729
    } else { // Line 2730
#ifdef D2U_UNICODE
      ConvertStdio(pFlag, progname, Convert, ConvertW); // Line 2732
#else
      ConvertStdio(pFlag, progname, Convert); // Line 2734
#endif
      if (pFlag->verbose) // Line 2736
        print_messages_stdio(pFlag, progname); // Line 2737
    } // Line 2738
    return pFlag->error; // Line 2739
  } // Line 2740

  return pFlag->error; // Line 2742
} // Line 2743

void d2u_getc_error(CFlag *ipFlag, const char *progname) // Line 2745
{ // Line 2746
    ipFlag->error = errno; // Line 2747
    if (ipFlag->verbose) { // Line 2748
      const char *errstr = strerror(errno); // Line 2749
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2750
      D2U_ANSI_FPRINTF(stderr, _("can not read from input file: %s\n"), errstr); // Line 2751
    } // Line 2752
} // Line 2753

void d2u_putc_error(CFlag *ipFlag, const char *progname) // Line 2755
{ // Line 2756
    ipFlag->error = errno; // Line 2757
    if (ipFlag->verbose) { // Line 2758
      const char *errstr = strerror(errno); // Line 2759
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2760
      D2U_ANSI_FPRINTF(stderr, _("can not write to output file: %s\n"), errstr); // Line 2761
    } // Line 2762
} // Line 2763

#ifdef D2U_UNICODE
void d2u_putwc_error(CFlag *ipFlag, const char *progname) // Line 2766
{ // Line 2767
    if (!(ipFlag->status & UNICODE_CONVERSION_ERROR)) { // Line 2768
      ipFlag->error = errno; // Line 2769
      if (ipFlag->verbose) { // Line 2770
        const char *errstr = strerror(errno); // Line 2771
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2772
        D2U_ANSI_FPRINTF(stderr, _("can not write to output file: %s\n"), errstr); // Line 2773
      } // Line 2774
    } // Line 2775
} // Line 2776

wint_t d2u_getwc(FILE *f, int bomtype) // Line 2778
{ // Line 2779
   int c_trail, c_lead; // Line 2780
   wint_t wc; // Line 2781

   if (((c_lead=fgetc(f)) == EOF)  || ((c_trail=fgetc(f)) == EOF)) // Line 2783
      return(WEOF); // Line 2784

   if (bomtype == FILE_UTF16LE) { /* UTF16 little endian */ // Line 2786
      c_trail <<=8; // Line 2787
      wc = (wint_t)(c_trail + c_lead) ; // Line 2788
   } else {                      /* UTF16 big endian */ // Line 2789
      c_lead <<=8; // Line 2790
      wc = (wint_t)(c_trail + c_lead) ; // Line 2791
   } // Line 2792
   return(wc); // Line 2793
} // Line 2794

wint_t d2u_ungetwc(wint_t wc, FILE *f, int bomtype) // Line 2796
{ // Line 2797
   int c_trail, c_lead; // Line 2798

   if (bomtype == FILE_UTF16LE) { /* UTF16 little endian */ // Line 2800
      c_trail = (int)(wc & 0xff00); // Line 2801
      c_trail >>=8; // Line 2802
      c_lead  = (int)(wc & 0xff); // Line 2803
   } else {                      /* UTF16 big endian */ // Line 2804
      c_lead = (int)(wc & 0xff00); // Line 2805
      c_lead >>=8; // Line 2806
      c_trail  = (int)(wc & 0xff); // Line 2807
   } // Line 2808

   /* push back in reverse order */ // Line 2810
   if ((ungetc(c_trail,f) == EOF)  || (ungetc(c_lead,f) == EOF)) // Line 2811
      return(WEOF); // Line 2812
   return(wc); // Line 2813
} // Line 2814

/* Put wide character */ // Line 2816
wint_t d2u_putwc(wint_t wc, FILE *f, CFlag *ipFlag, const char *progname) // Line 2817
{ // Line 2818
   static char mbs[8]; // Line 2819
   static wchar_t lead=0x01;  /* lead get's invalid value */ // Line 2820
   static wchar_t wstr[3]; // Line 2821
   size_t len; // Line 2822
#if (defined(_WIN32) && !defined(__CYGWIN__))
   DWORD dwFlags; // Line 2824
#endif

   if (ipFlag->keep_utf16) { // Line 2827
     int c_trail, c_lead; // Line 2828
     if (ipFlag->bomtype == FILE_UTF16LE) { /* UTF16 little endian */ // Line 2829
        c_trail = (int)(wc & 0xff00); // Line 2830
        c_trail >>=8; // Line 2831
        c_lead  = (int)(wc & 0xff); // Line 2832
     } else {                      /* UTF16 big endian */ // Line 2833
        c_lead = (int)(wc & 0xff00); // Line 2834
        c_lead >>=8; // Line 2835
        c_trail  = (int)(wc & 0xff); // Line 2836
     } // Line 2837
     if ((fputc(c_lead,f) == EOF)  || (fputc(c_trail,f) == EOF)) // Line 2838
       return(WEOF); // Line 2839
     return wc; // Line 2840
   } // Line 2841

   /* Note: In the new Unicode standard lead is named "high", and trail is name "low". */ // Line 2843

   /* check for lead without a trail */ // Line 2845
   if ((lead >= 0xd800) && (lead < 0xdc00) && ((wc < 0xdc00) || (wc >= 0xe000))) { // Line 2846
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2847
      D2U_UTF8_FPRINTF(stderr, _("error: Invalid surrogate pair. Missing low surrogate.\n")); // Line 2848
      ipFlag->status |= UNICODE_CONVERSION_ERROR ; // Line 2849
      return(WEOF); // Line 2850
   } // Line 2851

   if ((wc >= 0xd800) && (wc < 0xdc00)) {   /* Surrogate lead */ // Line 2853
      /* fprintf(stderr, "UTF-16 lead %x\n",wc); */ // Line 2854
      lead = (wchar_t)wc; /* lead (high) surrogate */ // Line 2855
      return(wc); // Line 2856
   } // Line 2857
   if ((wc >= 0xdc00) && (wc < 0xe000)) {   /* Surrogate trail */ // Line 2858
      static wchar_t trail; // Line 2859

      /* check for trail without a lead */ // Line 2861
      if ((lead < 0xd800) || (lead >= 0xdc00)) { // Line 2862
         D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2863
         D2U_UTF8_FPRINTF(stderr, _("error: Invalid surrogate pair. Missing high surrogate.\n")); // Line 2864
         ipFlag->status |= UNICODE_CONVERSION_ERROR ; // Line 2865
         return(WEOF); // Line 2866
      } // Line 2867
      /* fprintf(stderr, "UTF-16 trail %x\n",wc); */ // Line 2868
      trail = (wchar_t)wc; /* trail (low) surrogate */ // Line 2869
#if defined(_WIN32) || defined(__CYGWIN__)
      /* On Windows (including Cygwin) wchar_t is 16 bit */ // Line 2871
      /* We cannot decode an UTF-16 surrogate pair, because it will // Line 2872
         not fit in a 16 bit wchar_t. */ // Line 2873
      wstr[0] = lead; // Line 2874
      wstr[1] = trail; // Line 2875
      wstr[2] = L'\0'; // Line 2876
      lead = 0x01; /* make lead invalid */ // Line 2877
#else
      /* On Unix wchar_t is 32 bit */ // Line 2879
      /* When we don't decode the UTF-16 surrogate pair, wcstombs() does not // Line 2880
       * produce the same UTF-8 as WideCharToMultiByte().  The UTF-8 output // Line 2881
       * produced by wcstombs() is bigger, because it just translates the wide // Line 2882
       * characters in the range 0xD800..0xDBFF individually to UTF-8 sequences // Line 2883
       * (although these code points are reserved for use only as surrogate // Line 2884
       * pairs in UTF-16). // Line 2885
       * // Line 2886
       * Some smart viewers can still display this UTF-8 correctly (like Total // Line 2887
       * Commander lister), however the UTF-8 is not readable by Windows // Line 2888
       * Notepad (on Windows 7).  When we decode the UTF-16 surrogate pairs // Line 2889
       * ourselves the wcstombs() UTF-8 output is identical to what // Line 2890
       * WideCharToMultiByte() produces, and is readable by Notepad. // Line 2891
       * // Line 2892
       * Surrogate halves in UTF-8 are invalid. See also // Line 2893
       * https://en.wikipedia.org/wiki/UTF-8#Invalid_code_points // Line 2894
       * https://tools.ietf.org/html/rfc3629#page-5 // Line 2895
       * It is a bug in (some implementations of) wcstombs(). // Line 2896
       * On Cygwin 1.7 wcstombs() produces correct UTF-8 from UTF-16 surrogate pairs. // Line 2897
       */ // Line 2898
      /* Decode UTF-16 surrogate pair */ // Line 2899
      wstr[0] = 0x10000; // Line 2900
      wstr[0] += (lead & 0x03FF) << 10; // Line 2901
      wstr[0] += (trail & 0x03FF); // Line 2902
      wstr[1] = L'\0'; // Line 2903
      lead = 0x01; /* make lead invalid */ // Line 2904
      /* fprintf(stderr, "UTF-32  %x\n",wstr[0]); */ // Line 2905
#endif
   } else { // Line 2907
      wstr[0] = (wchar_t)wc; // Line 2908
      wstr[1] = L'\0'; // Line 2909
   } // Line 2910

   if (wc == 0x0000) { // Line 2912
      if (fputc(0, f) == EOF) // Line 2913
         return(WEOF); // Line 2914
      return(wc); // Line 2915
   } // Line 2916

#if (defined(_WIN32) && !defined(__CYGWIN__))
/* The WC_ERR_INVALID_CHARS flag is available since Windows Vista (0x0600). It enables checking for // Line 2919
   invalid input characters. */ // Line 2920
#if WINVER >= 0x0600
   dwFlags = WC_ERR_INVALID_CHARS; // Line 2922
#else
   dwFlags = 0; // Line 2924
#endif
   /* On Windows we convert UTF-16 always to UTF-8 or GB18030 */ // Line 2926
   if (ipFlag->locale_target == TARGET_GB18030) { // Line 2927
     len = (size_t)(WideCharToMultiByte(54936, dwFlags, wstr, -1, mbs, sizeof(mbs), NULL, NULL) -1); // Line 2928
   } else { // Line 2929
     len = (size_t)(WideCharToMultiByte(CP_UTF8, dwFlags, wstr, -1, mbs, sizeof(mbs), NULL, NULL) -1); // Line 2930
   } // Line 2931
#else
   /* On Unix we convert UTF-16 to the locale encoding */ // Line 2933
   len = wcstombs(mbs, wstr, sizeof(mbs)); // Line 2934
   /* fprintf(stderr, "len  %d\n",len); */ // Line 2935
#endif

   if ( len == (size_t)(-1) ) { // Line 2938
      /* Stop when there is a conversion error */ // Line 2939
   /* On Windows we convert UTF-16 always to UTF-8 or GB18030 */ // Line 2940
      if (ipFlag->verbose) { // Line 2941
#if (defined(_WIN32) && !defined(__CYGWIN__))
        d2u_PrintLastError(progname); // Line 2943
#else
        const char *errstr = strerror(errno); // Line 2945
        D2U_UTF8_FPRINTF(stderr, "%s:", progname); // Line 2946
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 2947
#endif
      } // Line 2949
      ipFlag->status |= UNICODE_CONVERSION_ERROR ; // Line 2950
      return(WEOF); // Line 2951
   } else { // Line 2952
      size_t i; // Line 2953
      for (i=0; i<len; i++) { // Line 2954
         if (fputc(mbs[i], f) == EOF) // Line 2955
            return(WEOF); // Line 2956
      } // Line 2957
   } // Line 2958
   return(wc); // Line 2959
} // Line 2960
#endif
