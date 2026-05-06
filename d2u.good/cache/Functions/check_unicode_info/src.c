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
