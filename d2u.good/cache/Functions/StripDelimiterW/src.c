wint_t StripDelimiterW(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, wint_t CurChar, unsigned int *converted, const char *progname) // Line 89
{ // Line 90
  wint_t TempNextChar; // Line 91
  /* CurChar is always CR (x0d) */ // Line 92
  /* In normal dos2unix mode put nothing (skip CR). */ // Line 93
  /* Don't modify Mac files when in dos2unix mode. */ // Line 94
  if ( (TempNextChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 95
    if (d2u_ungetwc( TempNextChar, ipInF, ipFlag->bomtype) == WEOF) {  /* put back peek char */ // Line 96
        d2u_getc_error(ipFlag,progname); // Line 97
        return WEOF; // Line 98
    } // Line 99
    if ( TempNextChar != 0x0a ) { // Line 100
      if (d2u_putwc(CurChar, ipOutF, ipFlag, progname) == WEOF) {  /* Mac line, put CR */ // Line 101
          d2u_putwc_error(ipFlag,progname); // Line 102
          return WEOF; // Line 103
      } // Line 104
    } else { // Line 105
      (*converted)++; // Line 106
      if (ipFlag->NewLine) {  /* add additional LF? */ // Line 107
        if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 108
            d2u_putwc_error(ipFlag,progname); // Line 109
            return WEOF; // Line 110
        } // Line 111
      } // Line 112
    } // Line 113
  } else { // Line 114
    if (ferror(ipInF)) { // Line 115
        d2u_getc_error(ipFlag,progname); // Line 116
        return WEOF; // Line 117
    } // Line 118
    if ( CurChar == 0x0d ) {  /* EOF: last Mac line delimiter (CR)? */ // Line 119
        if (d2u_putwc(CurChar, ipOutF, ipFlag, progname) == WEOF) { // Line 120
            d2u_putwc_error(ipFlag,progname); // Line 121
            return WEOF; // Line 122
        } // Line 123
    } // Line 124
  } // Line 125
  return CurChar; // Line 126
} // Line 127
