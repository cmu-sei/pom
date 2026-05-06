int GetFileInfoStdio(CFlag *ipFlag, const char *progname) // Line 2308
{ // Line 2309
  int bomtype_orig = FILE_MBS; /* messages must print the real bomtype, not the assumed bomtype */ // Line 2310

  ipFlag->status = 0 ; // Line 2312

#if defined(_WIN32) && !defined(__CYGWIN__)

    /* stdin and stdout are by default text streams. We need // Line 2316
     * to set them to binary mode. Otherwise an LF will // Line 2317
     * automatically be converted to CR-LF on DOS/Windows. // Line 2318
     * Erwin */ // Line 2319

    /* POSIX 'setmode' was deprecated by MicroSoft since // Line 2321
     * Visual C++ 2005. Use ISO C++ conformant '_setmode' instead. */ // Line 2322

    _setmode(_fileno(stdin), _O_BINARY); // Line 2324
#elif defined(__MSDOS__) || defined(__CYGWIN__) || defined(__OS2__)
    setmode(fileno(stdin), O_BINARY); // Line 2326
#endif

  if (check_unicode_info(stdin, ipFlag, progname, &bomtype_orig)) // Line 2329
    return -1; // Line 2330

  /* info successful? */ // Line 2332
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 2334
    FileInfoW(stdin, ipFlag, "", bomtype_orig, progname); // Line 2335
  } else { // Line 2336
    FileInfo(stdin, ipFlag, "", bomtype_orig, progname); // Line 2337
  } // Line 2338
#else
  FileInfo(stdin, ipFlag, "", bomtype_orig, progname); // Line 2340
#endif

  return 0; // Line 2343
} // Line 2344
