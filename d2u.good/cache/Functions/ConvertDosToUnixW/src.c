int ConvertDosToUnixW(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, const char *progname) // Line 180
{ // Line 181
    int RetVal = 0; // Line 182
    wint_t PrevChar = WEOF; // Line 183
    wint_t TempChar; // Line 184
    wint_t TempNextChar; // Line 185
    unsigned int line_nr = 1; // Line 186
    unsigned int converted = 0; // Line 187

    ipFlag->status = 0; // Line 189

    /* CR-LF -> LF */ // Line 191
    /* LF    -> LF, in case the input file is a Unix text file */ // Line 192
    /* CR    -> CR, in dos2unix mode (don't modify Mac file) */ // Line 193
    /* CR    -> LF, in Mac mode */ // Line 194
    /* \x0a = Newline/Line Feed (LF) */ // Line 195
    /* \x0d = Carriage Return (CR) */ // Line 196

    switch (ipFlag->FromToMode) // Line 198
    { // Line 199
      case FROMTO_DOS2UNIX: /* dos2unix */ // Line 200
        while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) {  /* get character */ // Line 201
          if ((ipFlag->Force == 0) && // Line 202
              (TempChar < 32) && // Line 203
              (TempChar != 0x0a) &&  /* Not an LF */ // Line 204
              (TempChar != 0x0d) &&  /* Not a CR */ // Line 205
              (TempChar != 0x09) &&  /* Not a TAB */ // Line 206
              (TempChar != 0x0c)) {  /* Not a form feed */ // Line 207
            RetVal = -1; // Line 208
            ipFlag->status |= BINARY_FILE ; // Line 209
            if (ipFlag->verbose) { // Line 210
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 211
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 212
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x00%02X found at line %u\n"),TempChar, line_nr); // Line 213
            } // Line 214
            break; // Line 215
          } // Line 216
          if (TempChar != 0x0d) { // Line 217
            if (TempChar == 0x0a) /* Count all DOS and Unix line breaks */ // Line 218
              ++line_nr; // Line 219
            if (d2u_putwc(TempChar, ipOutF, ipFlag, progname) == WEOF) { // Line 220
              RetVal = -1; // Line 221
              d2u_putwc_error(ipFlag,progname); // Line 222
              break; // Line 223
            } // Line 224
          } else { // Line 225
            if (StripDelimiterW( ipInF, ipOutF, ipFlag, TempChar, &converted, progname) == WEOF) { // Line 226
              RetVal = -1; // Line 227
              break; // Line 228
            } // Line 229
          } // Line 230
          PrevChar = TempChar; // Line 231
        } // Line 232
        if (TempChar == WEOF && ipFlag->add_eol && PrevChar != WEOF && PrevChar != 0x0a) { // Line 233
          /* Add missing line break at the last line. */ // Line 234
            if (ipFlag->verbose > 1) { // Line 235
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 236
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 237
            } // Line 238
            if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 239
              RetVal = -1; // Line 240
              d2u_putwc_error(ipFlag,progname); // Line 241
            } // Line 242
        } // Line 243
        if ((TempChar == WEOF) && ferror(ipInF)) { // Line 244
          RetVal = -1; // Line 245
          d2u_getc_error(ipFlag,progname); // Line 246
        } // Line 247
        break; // Line 248
      case FROMTO_MAC2UNIX: /* mac2unix */ // Line 249
        while ((TempChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 250
          if ((ipFlag->Force == 0) && // Line 251
              (TempChar < 32) && // Line 252
              (TempChar != 0x0a) &&  /* Not an LF */ // Line 253
              (TempChar != 0x0d) &&  /* Not a CR */ // Line 254
              (TempChar != 0x09) &&  /* Not a TAB */ // Line 255
              (TempChar != 0x0c)) {  /* Not a form feed */ // Line 256
            RetVal = -1; // Line 257
            ipFlag->status |= BINARY_FILE ; // Line 258
            if (ipFlag->verbose) { // Line 259
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 260
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 261
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x00%02X found at line %u\n"), TempChar, line_nr); // Line 262
            } // Line 263
            break; // Line 264
          } // Line 265
          if ((TempChar != 0x0d)) { // Line 266
            if (TempChar == 0x0a) /* Count all DOS and Unix line breaks */ // Line 267
              ++line_nr; // Line 268
            if(d2u_putwc(TempChar, ipOutF, ipFlag, progname) == WEOF) { // Line 269
              RetVal = -1; // Line 270
              d2u_putwc_error(ipFlag,progname); // Line 271
              break; // Line 272
            } // Line 273
          } else{ // Line 274
            /* TempChar is a CR */ // Line 275
            if ( (TempNextChar = d2u_getwc(ipInF, ipFlag->bomtype)) != WEOF) { // Line 276
              if (d2u_ungetwc( TempNextChar, ipInF, ipFlag->bomtype) == WEOF) {  /* put back peek char */ // Line 277
                d2u_getc_error(ipFlag,progname); // Line 278
                RetVal = -1; // Line 279
                break; // Line 280
              } // Line 281
              /* Don't touch this delimiter if it's a CR,LF pair. */ // Line 282
              if ( TempNextChar == 0x0a ) { // Line 283
                if (d2u_putwc(0x0d, ipOutF, ipFlag, progname) == WEOF) { /* put CR, part of DOS CR-LF */ // Line 284
                  d2u_putwc_error(ipFlag,progname); // Line 285
                  RetVal = -1; // Line 286
                  break; // Line 287
                } // Line 288
                PrevChar = TempChar; // Line 289
                continue; // Line 290
              } // Line 291
            } // Line 292
            if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { /* MAC line end (CR). Put LF */ // Line 293
              RetVal = -1; // Line 294
              d2u_putwc_error(ipFlag,progname); // Line 295
              break; // Line 296
            } // Line 297
            converted++; // Line 298
            line_nr++; /* Count all Mac line breaks */ // Line 299
            if (ipFlag->NewLine) {  /* add additional LF? */ // Line 300
              if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 301
                RetVal = -1; // Line 302
                d2u_putwc_error(ipFlag,progname); // Line 303
                break; // Line 304
              } // Line 305
            } // Line 306
          } // Line 307
          PrevChar = TempChar; // Line 308
        } // Line 309
        if (TempChar == WEOF && ipFlag->add_eol && PrevChar != WEOF && !(PrevChar == 0x0a || PrevChar == 0x0d)) { // Line 310
          /* Add missing line break at the last line. */ // Line 311
            if (ipFlag->verbose > 1) { // Line 312
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 313
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 314
            } // Line 315
            if (d2u_putwc(0x0a, ipOutF, ipFlag, progname) == WEOF) { // Line 316
              RetVal = -1; // Line 317
              d2u_putwc_error(ipFlag,progname); // Line 318
            } // Line 319
        } // Line 320
        if ((TempChar == WEOF) && ferror(ipInF)) { // Line 321
          RetVal = -1; // Line 322
          d2u_getc_error(ipFlag,progname); // Line 323
        } // Line 324
        break; // Line 325
      default: /* unknown FromToMode */ // Line 326
      ; // Line 327
#if DEBUG
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 329
      D2U_UTF8_FPRINTF(stderr, _("program error, invalid conversion mode %d\n"),ipFlag->FromToMode); // Line 330
      exit(1); // Line 331
#endif
    } // Line 333
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) // Line 334
        ipFlag->line_nr = line_nr; // Line 335
    if ((RetVal == 0) && (ipFlag->verbose > 1)) { // Line 336
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 337
      D2U_UTF8_FPRINTF(stderr, _("Converted %u out of %u line breaks.\n"), converted, line_nr -1); // Line 338
    } // Line 339
    return RetVal; // Line 340
} // Line 341
