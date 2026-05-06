int GetFileInfo(char *ipInFN, CFlag *ipFlag, const char *progname) // Line 2250
{ // Line 2251
  FILE *InF = NULL; // Line 2252
  int bomtype_orig = FILE_MBS; /* messages must print the real bomtype, not the assumed bomtype */ // Line 2253

  ipFlag->status = 0 ; // Line 2255

  /* Test if input file is a regular file or symbolic link */ // Line 2257
  if (regfile(ipInFN, 1, ipFlag, progname)) { // Line 2258
    ipFlag->status |= NO_REGFILE ; // Line 2259
    /* Not a failure, skipping non-regular input file according spec. */ // Line 2260
    return -1; // Line 2261
  } // Line 2262

  /* Test if input file target is a regular file */ // Line 2264
  if (symbolic_link(ipInFN) && regfile_target(ipInFN, ipFlag,progname)) { // Line 2265
    ipFlag->status |= INPUT_TARGET_NO_REGFILE ; // Line 2266
    /* Not a failure, skipping non-regular input file according spec. */ // Line 2267
    return -1; // Line 2268
  } // Line 2269


  /* can open in file? */ // Line 2272
  InF=OpenInFile(ipInFN); // Line 2273
  if (InF == NULL) { // Line 2274
    if (ipFlag->verbose) { // Line 2275
      const char *errstr = strerror(errno); // Line 2276
      ipFlag->error = errno; // Line 2277
      D2U_UTF8_FPRINTF(stderr, "%s: %s: ", progname, ipInFN); // Line 2278
      D2U_ANSI_FPRINTF(stderr, "%s\n", errstr); // Line 2279
    } // Line 2280
    return -1; // Line 2281
  } // Line 2282


  if (check_unicode_info(InF, ipFlag, progname, &bomtype_orig)) { // Line 2285
    d2u_fclose(InF, ipInFN, ipFlag, "r", progname); // Line 2286
    return -1; // Line 2287
  } // Line 2288

  /* info successful? */ // Line 2290
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 2292
    FileInfoW(InF, ipFlag, ipInFN, bomtype_orig, progname); // Line 2293
  } else { // Line 2294
    FileInfo(InF, ipFlag, ipInFN, bomtype_orig, progname); // Line 2295
  } // Line 2296
#else
  FileInfo(InF, ipFlag, ipInFN, bomtype_orig, progname); // Line 2298
#endif

  /* can close in file? */ // Line 2301
  if (d2u_fclose(InF, ipInFN, ipFlag, "r", progname) == EOF) // Line 2302
    return -1; // Line 2303

  return 0; // Line 2305
} // Line 2306
