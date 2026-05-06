void FileInfo(FILE* ipInF, CFlag *ipFlag, const char *filename, int bomtype, const char *progname) // Line 2195
{ // Line 2196
  int TempChar; // Line 2197
  int PreviousChar = 0; // Line 2198
  unsigned int lb_dos = 0; // Line 2199
  unsigned int lb_unix = 0; // Line 2200
  unsigned int lb_mac = 0; // Line 2201
  int last_eol = 0; // Line 2202

  ipFlag->status = 0; // Line 2204

  while ((TempChar = fgetc(ipInF)) != EOF) { // Line 2206
    if ( (TempChar < 32) && // Line 2207
        (TempChar != '\x0a') &&  /* Not an LF */ // Line 2208
        (TempChar != '\x0d') &&  /* Not a CR */ // Line 2209
        (TempChar != '\x09') &&  /* Not a TAB */ // Line 2210
        (TempChar != '\x0c')) {  /* Not a form feed */ // Line 2211
      ipFlag->status |= BINARY_FILE ; // Line 2212
      } // Line 2213
    if (TempChar != '\x0a') { /* Not an LF */ // Line 2214
      PreviousChar = TempChar; // Line 2215
      if (TempChar == '\x0d') { /* CR */ // Line 2216
        lb_mac++; // Line 2217
        last_eol = INFO_MAC; // Line 2218
      } else { // Line 2219
        last_eol = 0; // Line 2220
      } // Line 2221
    } else { // Line 2222
      /* TempChar is an LF */ // Line 2223
      if ( PreviousChar == '\x0d' ) { /* CR,LF pair. */ // Line 2224
        lb_dos++; // Line 2225
        lb_mac--; // Line 2226
        last_eol = INFO_DOS; // Line 2227
        PreviousChar = TempChar; // Line 2228
        continue; // Line 2229
      } // Line 2230
      PreviousChar = TempChar; // Line 2231
      lb_unix++; /* Unix line end (LF). */ // Line 2232
      last_eol = INFO_UNIX; // Line 2233
    } // Line 2234
  } // Line 2235
  if ((TempChar == EOF) && ferror(ipInF)) { // Line 2236
    ipFlag->error = errno; // Line 2237
    if (ipFlag->verbose) { // Line 2238
      const char *errstr = strerror(errno); // Line 2239
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2240
      D2U_UTF8_FPRINTF(stderr, _("can not read from input file %s:"), filename); // Line 2241
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 2242
    } // Line 2243
    return; // Line 2244
  } // Line 2245

  printInfo(ipFlag, filename, bomtype, lb_dos, lb_unix, lb_mac, last_eol); // Line 2247
} // Line 2248
