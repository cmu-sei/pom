void FileInfoW(FILE* ipInF, CFlag *ipFlag, const char *filename, int bomtype, const char *progname) // Line 2138
{ // Line 2139
  wint_t TempChar; // Line 2140
  wint_t PreviousChar = 0; // Line 2141
  unsigned int lb_dos = 0; // Line 2142
  unsigned int lb_unix = 0; // Line 2143
  unsigned int lb_mac = 0; // Line 2144
  int last_eol = 0; // Line 2145

  ipFlag->status = 0; // Line 2147

  while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 2149
    if ( (TempChar < 32) && // Line 2150
        (TempChar != 0x0a) &&  /* Not an LF */ // Line 2151
        (TempChar != 0x0d) &&  /* Not a CR */ // Line 2152
        (TempChar != 0x09) &&  /* Not a TAB */ // Line 2153
        (TempChar != 0x0c)) {  /* Not a form feed */ // Line 2154
      ipFlag->status |= BINARY_FILE ; // Line 2155
    } // Line 2156
    if (TempChar != 0x0a) { /* Not an LF */ // Line 2157
      PreviousChar = TempChar; // Line 2158
      if (TempChar == 0x0d) { /* CR */ // Line 2159
        lb_mac++; // Line 2160
        last_eol = INFO_MAC; // Line 2161
      } else { // Line 2162
        last_eol = 0; // Line 2163
      } // Line 2164
    } else{ // Line 2165
      /* TempChar is an LF */ // Line 2166
      if ( PreviousChar == 0x0d ) { /* CR,LF pair. */ // Line 2167
        lb_dos++; // Line 2168
        lb_mac--; // Line 2169
        last_eol = INFO_DOS; // Line 2170
        PreviousChar = TempChar; // Line 2171
        continue; // Line 2172
      } // Line 2173
      PreviousChar = TempChar; // Line 2174
      lb_unix++; /* Unix line end (LF). */ // Line 2175
      last_eol = INFO_UNIX; // Line 2176
    } // Line 2177
  } // Line 2178
  if ((TempChar == WEOF) && ferror(ipInF)) { // Line 2179
    ipFlag->error = errno; // Line 2180
    if (ipFlag->verbose) { // Line 2181
      const char *errstr = strerror(errno); // Line 2182
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2183
      D2U_UTF8_FPRINTF(stderr, _("can not read from input file %s:"), filename); // Line 2184
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 2185
    } // Line 2186
    return; // Line 2187
  } // Line 2188

  printInfo(ipFlag, filename, bomtype, lb_dos, lb_unix, lb_mac, last_eol); // Line 2190

} // Line 2192
