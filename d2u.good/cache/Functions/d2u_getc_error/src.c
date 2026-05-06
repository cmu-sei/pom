void d2u_getc_error(CFlag *ipFlag, const char *progname) // Line 2745
{ // Line 2746
    ipFlag->error = errno; // Line 2747
    if (ipFlag->verbose) { // Line 2748
      const char *errstr = strerror(errno); // Line 2749
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2750
      D2U_ANSI_FPRINTF(stderr, _("can not read from input file: %s\n"), errstr); // Line 2751
    } // Line 2752
} // Line 2753
