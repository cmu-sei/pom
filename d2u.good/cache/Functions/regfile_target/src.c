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
