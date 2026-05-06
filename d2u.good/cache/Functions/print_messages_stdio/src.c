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
