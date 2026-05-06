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
