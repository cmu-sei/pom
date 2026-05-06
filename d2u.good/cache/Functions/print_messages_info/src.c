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
