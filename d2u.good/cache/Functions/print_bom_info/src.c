void print_bom_info (const int bomtype) // Line 1291
{ // Line 1292
/* The BOM info must not be translated to other languages, otherwise scripts // Line 1293
   that process the output may not work in other than English locales. */ // Line 1294
    switch (bomtype) { // Line 1295
    case FILE_UTF16LE:   /* UTF-16 Little Endian */ // Line 1296
      D2U_UTF8_FPRINTF(stdout, "  UTF-16LE"); // Line 1297
      break; // Line 1298
    case FILE_UTF16BE:   /* UTF-16 Big Endian */ // Line 1299
      D2U_UTF8_FPRINTF(stdout, "  UTF-16BE"); // Line 1300
      break; // Line 1301
    case FILE_UTF8:      /* UTF-8 */ // Line 1302
      D2U_UTF8_FPRINTF(stdout, "  UTF-8   "); // Line 1303
      break; // Line 1304
    case FILE_GB18030:   /* GB18030 */ // Line 1305
      D2U_UTF8_FPRINTF(stdout, "  GB18030 "); // Line 1306
      break; // Line 1307
    default: // Line 1308
      D2U_UTF8_FPRINTF(stdout, "  no_bom  "); // Line 1309
    ; // Line 1310
  } // Line 1311
} // Line 1312
