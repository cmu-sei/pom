int check_unicode(FILE *InF, FILE *TempF,  CFlag *ipFlag, const char *ipInFN, const char *progname) // Line 1362
{ // Line 1363

#ifdef D2U_UNICODE
  if (ipFlag->verbose > 1) { // Line 1366
    if (ipFlag->ConvMode == CONVMODE_UTF16LE) { // Line 1367
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1368
      D2U_UTF8_FPRINTF(stderr, _("Assuming UTF-16LE encoding.\n") ); // Line 1369
    } // Line 1370
    if (ipFlag->ConvMode == CONVMODE_UTF16BE) { // Line 1371
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1372
      D2U_UTF8_FPRINTF(stderr, _("Assuming UTF-16BE encoding.\n") ); // Line 1373
    } // Line 1374
  } // Line 1375
#endif
  if ((InF = read_bom(InF, &ipFlag->bomtype)) == NULL) { // Line 1377
    d2u_getc_error(ipFlag,progname); // Line 1378
    return -1; // Line 1379
  } // Line 1380
  if (ipFlag->verbose > 1) // Line 1381
    print_bom(ipFlag->bomtype, ipInFN, progname); // Line 1382
#ifndef D2U_UNICODE
  /* It is possible that an UTF-16 has no 8-bit binary symbols. We must stop // Line 1384
   * processing an UTF-16 file when UTF-16 is not supported. Don't trust on // Line 1385
   * finding a binary symbol. // Line 1386
   */ // Line 1387
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1388
    ipFlag->status |= UNICODE_NOT_SUPPORTED ; // Line 1389
    return -1; // Line 1390
  } // Line 1391
#endif
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_MBS) && (ipFlag->ConvMode == CONVMODE_UTF16LE)) // Line 1394
    ipFlag->bomtype = FILE_UTF16LE; // Line 1395
  if ((ipFlag->bomtype == FILE_MBS) && (ipFlag->ConvMode == CONVMODE_UTF16BE)) // Line 1396
    ipFlag->bomtype = FILE_UTF16BE; // Line 1397


#if !defined(_WIN32) && !defined(__CYGWIN__) /* Not Windows or Cygwin */
  if (!ipFlag->keep_utf16 && ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE))) { // Line 1401
    if (sizeof(wchar_t) < 4) { // Line 1402
      /* A decoded UTF-16 surrogate pair must fit in a wchar_t */ // Line 1403
      ipFlag->status |= WCHAR_T_TOO_SMALL ; // Line 1404
      if (!ipFlag->error) ipFlag->error = 1; // Line 1405
      return -1; // Line 1406
    } // Line 1407
  } // Line 1408
#endif

#if !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__)  /* Unix, Cygwin */
  if (strcmp(nl_langinfo(CODESET), "GB18030") == 0) // Line 1412
    ipFlag->locale_target = TARGET_GB18030; // Line 1413
#endif
#endif

  if ((ipFlag->add_bom) || ((ipFlag->keep_bom) && (ipFlag->bomtype > 0))) // Line 1417
    if (write_bom(TempF, ipFlag, progname) == NULL) return -1; // Line 1418

  return 0; // Line 1420
} // Line 1421
