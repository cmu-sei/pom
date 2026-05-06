void print_bom (const int bomtype, const char *filename, const char *progname) // Line 1248
{ // Line 1249
    char informat[64]; // Line 1250

    switch (bomtype) { // Line 1252
    case FILE_UTF16LE:   /* UTF-16 Little Endian */ // Line 1253
      d2u_strncpy(informat,_("UTF-16LE"),sizeof(informat)); // Line 1254
      break; // Line 1255
    case FILE_UTF16BE:   /* UTF-16 Big Endian */ // Line 1256
      d2u_strncpy(informat,_("UTF-16BE"),sizeof(informat)); // Line 1257
      break; // Line 1258
    case FILE_UTF8:      /* UTF-8 */ // Line 1259
      d2u_strncpy(informat,_("UTF-8"),sizeof(informat)); // Line 1260
      break; // Line 1261
    case FILE_GB18030:      /* GB18030 */ // Line 1262
      d2u_strncpy(informat,_("GB18030"),sizeof(informat)); // Line 1263
      break; // Line 1264
    default: // Line 1265
    ; // Line 1266
  } // Line 1267

  if (bomtype > 0) { // Line 1269
#ifdef D2U_UNIFILE
    wchar_t informatw[64]; // Line 1271
#endif
    informat[sizeof(informat)-1] = '\0'; // Line 1273

/* Change informat to UTF-8 for d2u_utf8_fprintf. */ // Line 1275
#ifdef D2U_UNIFILE
    /* The format string is encoded in the system default // Line 1277
     * Windows ANSI code page. May have been translated // Line 1278
     * by gettext. Convert it to wide characters. */ // Line 1279
    d2u_MultiByteToWideChar(CP_ACP,0, informat, -1, informatw, sizeof(informat)); // Line 1280
    /* then convert the format string to UTF-8 */ // Line 1281
    d2u_WideCharToMultiByte(CP_UTF8, 0, informatw, -1, informat, sizeof(informat), NULL, NULL); // Line 1282
#endif

    D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1285
    D2U_UTF8_FPRINTF(stderr, _("Input file %s has %s BOM.\n"), filename, informat); // Line 1286
  } // Line 1287

} // Line 1289
