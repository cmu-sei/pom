void d2u_putc_error(CFlag *ipFlag, const char *progname) // Line 2755
{ // Line 2756
    ipFlag->error = errno; // Line 2757
    if (ipFlag->verbose) { // Line 2758
      const char *errstr = strerror(errno); // Line 2759
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2760
      D2U_ANSI_FPRINTF(stderr, _("can not write to output file: %s\n"), errstr); // Line 2761
    } // Line 2762
} // Line 2763
