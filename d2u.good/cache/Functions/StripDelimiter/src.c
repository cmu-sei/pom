int StripDelimiter(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, int CurChar, unsigned int *converted, const char *progname) // Line 135
{ // Line 136
  int TempNextChar; // Line 137
  /* CurChar is always CR (x0d) */ // Line 138
  /* In normal dos2unix mode put nothing (skip CR). */ // Line 139
  /* Don't modify Mac files when in dos2unix mode. */ // Line 140
  if ( (TempNextChar = fgetc(ipInF)) != EOF) { // Line 141
    if (ungetc( TempNextChar, ipInF ) == EOF) { /* put back peek char */ // Line 142
        d2u_getc_error(ipFlag,progname); // Line 143
        return EOF; // Line 144
    } // Line 145
    if ( TempNextChar != '\x0a' ) { // Line 146
      if (fputc( CurChar, ipOutF ) == EOF) { /* Mac line, put CR */ // Line 147
          d2u_putc_error(ipFlag,progname); // Line 148
          return EOF; // Line 149
      } // Line 150
    } else { // Line 151
      (*converted)++; // Line 152
      if (ipFlag->NewLine) {  /* add additional LF? */ // Line 153
        if (fputc('\x0a', ipOutF) == EOF) { // Line 154
            d2u_putc_error(ipFlag,progname); // Line 155
            return EOF; // Line 156
        } // Line 157
      } // Line 158
    } // Line 159
  } else { // Line 160
    if (ferror(ipInF)) { // Line 161
        d2u_getc_error(ipFlag,progname); // Line 162
        return EOF; // Line 163
    } // Line 164
    if ( CurChar == '\x0d' ) {  /* EOF: last Mac line delimiter (CR)? */ // Line 165
        if (fputc( CurChar, ipOutF ) == EOF) { // Line 166
            d2u_putc_error(ipFlag,progname); // Line 167
            return EOF; // Line 168
        } // Line 169
    } // Line 170
  } // Line 171
  return CurChar; // Line 172
} // Line 173
