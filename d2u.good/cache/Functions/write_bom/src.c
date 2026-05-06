FILE *write_bom (FILE *f, CFlag *ipFlag, const char *progname) // Line 1187
{ // Line 1188
  int bomtype = ipFlag->bomtype; // Line 1189

  if ((bomtype == FILE_MBS)&&(ipFlag->locale_target == TARGET_GB18030)) // Line 1191
    bomtype = FILE_GB18030; // Line 1192

  if (ipFlag->keep_utf16) // Line 1194
  { // Line 1195
    switch (bomtype) { // Line 1196
      case FILE_UTF16LE:   /* UTF-16 Little Endian */ // Line 1197
        if (fprintf(f, "%s", "\xFF\xFE") < 0) return NULL; // Line 1198
        if (ipFlag->verbose > 1) { // Line 1199
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1200
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-16LE")); // Line 1201
        } // Line 1202
        break; // Line 1203
      case FILE_UTF16BE:   /* UTF-16 Big Endian */ // Line 1204
        if (fprintf(f, "%s", "\xFE\xFF") < 0) return NULL; // Line 1205
        if (ipFlag->verbose > 1) { // Line 1206
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1207
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-16BE")); // Line 1208
        } // Line 1209
        break; // Line 1210
      case FILE_GB18030:  /* GB18030 */ // Line 1211
        if (fprintf(f, "%s", "\x84\x31\x95\x33") < 0) return NULL; // Line 1212
        if (ipFlag->verbose > 1) { // Line 1213
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1214
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("GB18030")); // Line 1215
        } // Line 1216
        break; // Line 1217
      default:      /* UTF-8 */ // Line 1218
        if (fprintf(f, "%s", "\xEF\xBB\xBF") < 0) return NULL; // Line 1219
        if (ipFlag->verbose > 1) { // Line 1220
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1221
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-8")); // Line 1222
        } // Line 1223
      ; // Line 1224
    } // Line 1225
  } else { // Line 1226
    if ((bomtype == FILE_GB18030) || // Line 1227
        (((bomtype == FILE_UTF16LE)||(bomtype == FILE_UTF16BE))&&(ipFlag->locale_target == TARGET_GB18030)) // Line 1228
       ) { // Line 1229
        if (fprintf(f, "%s", "\x84\x31\x95\x33") < 0) return NULL; /* GB18030 */ // Line 1230
        if (ipFlag->verbose > 1) // Line 1231
        { // Line 1232
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1233
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("GB18030")); // Line 1234
        } // Line 1235
     } else { // Line 1236
        if (fprintf(f, "%s", "\xEF\xBB\xBF") < 0) return NULL; /* UTF-8 */ // Line 1237
        if (ipFlag->verbose > 1) // Line 1238
        { // Line 1239
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1240
          D2U_ANSI_FPRINTF(stderr, _("Writing %s BOM.\n"), _("UTF-8")); // Line 1241
        } // Line 1242
     } // Line 1243
  } // Line 1244
  return(f); // Line 1245
} // Line 1246
