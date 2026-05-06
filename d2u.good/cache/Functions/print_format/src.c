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
