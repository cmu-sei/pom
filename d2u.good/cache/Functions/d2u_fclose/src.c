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
