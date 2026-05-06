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
