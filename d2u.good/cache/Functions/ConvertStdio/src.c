int ConvertStdio(CFlag *ipFlag, const char *progname, // Line 1807
                   int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 1808
#ifdef D2U_UNICODE
                 , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 1810
#endif
                  ) // Line 1812
{ // Line 1813
    ipFlag->NewFile = 1; // Line 1814
    ipFlag->KeepDate = 0; // Line 1815

#if defined(_WIN32) && !defined(__CYGWIN__)

    /* stdin and stdout are by default text streams. We need // Line 1819
     * to set them to binary mode. Otherwise an LF will // Line 1820
     * automatically be converted to CR-LF on DOS/Windows. // Line 1821
     * Erwin */ // Line 1822

    /* POSIX 'setmode' was deprecated by MicroSoft since // Line 1824
     * Visual C++ 2005. Use ISO C++ conformant '_setmode' instead. */ // Line 1825

    _setmode(_fileno(stdout), _O_BINARY); // Line 1827
    _setmode(_fileno(stdin), _O_BINARY); // Line 1828
#elif defined(__MSDOS__) || defined(__CYGWIN__) || defined(__OS2__)
    setmode(fileno(stdout), O_BINARY); // Line 1830
    setmode(fileno(stdin), O_BINARY); // Line 1831
#endif

    if (check_unicode(stdin, stdout, ipFlag, "stdin", progname)) // Line 1834
        return -1; // Line 1835

#ifdef D2U_UNICODE
    if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1838
        return ConvertW(stdin, stdout, ipFlag, progname); // Line 1839
    } else { // Line 1840
        return Convert(stdin, stdout, ipFlag, progname); // Line 1841
    } // Line 1842
#else
    return Convert(stdin, stdout, ipFlag, progname); // Line 1844
#endif
} // Line 1846
