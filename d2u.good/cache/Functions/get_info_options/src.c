void get_info_options(char *option, CFlag *pFlag, const char *progname) // Line 2346
{ // Line 2347
  char *ptr; // Line 2348
  int default_info = 1; // Line 2349

  ptr = option; // Line 2351

  if (*ptr == '\0') { /* no flags */ // Line 2353
    pFlag->file_info |= INFO_DEFAULT; // Line 2354
    return; // Line 2355
  } // Line 2356

  while (*ptr != '\0') { // Line 2358
    switch (*ptr) { // Line 2359
      case '0':   /* Print null characters instead of newline characters. */ // Line 2360
        pFlag->file_info |= INFO_PRINT0; // Line 2361
        break; // Line 2362
      case 'd':   /* Print nr of DOS line breaks. */ // Line 2363
        pFlag->file_info |= INFO_DOS; // Line 2364
        default_info = 0; // Line 2365
        break; // Line 2366
      case 'u':   /* Print nr of Unix line breaks. */ // Line 2367
        pFlag->file_info |= INFO_UNIX; // Line 2368
        default_info = 0; // Line 2369
        break; // Line 2370
      case 'm':   /* Print nr of Mac line breaks. */ // Line 2371
        pFlag->file_info |= INFO_MAC; // Line 2372
        default_info = 0; // Line 2373
        break; // Line 2374
      case 'b':   /* Print BOM. */ // Line 2375
        pFlag->file_info |= INFO_BOM; // Line 2376
        default_info = 0; // Line 2377
        break; // Line 2378
      case 't':   /* Text or binary. */ // Line 2379
        pFlag->file_info |= INFO_TEXT; // Line 2380
        default_info = 0; // Line 2381
        break; // Line 2382
      case 'e':   /* Print EOL of last line. */ // Line 2383
        pFlag->file_info |= INFO_EOL; // Line 2384
        default_info = 0; // Line 2385
        break; // Line 2386
      case 'c':   /* Print only files that would be converted. */ // Line 2387
        pFlag->file_info |= INFO_CONVERT; // Line 2388
        default_info = 0; // Line 2389
        break; // Line 2390
      case 'h':   /* Print a header. */ // Line 2391
        pFlag->file_info |= INFO_HEADER; // Line 2392
        break; // Line 2393
      case 'p':   /* Remove path from file names. */ // Line 2394
        pFlag->file_info |= INFO_NOPATH; // Line 2395
        break; // Line 2396
      default: // Line 2397
       /* Terminate the program on a wrong option. If pFlag->file_info is // Line 2398
          zero and the program goes on, it may do unwanted conversions. */ // Line 2399
        D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2400
        D2U_UTF8_FPRINTF(stderr,_("wrong flag '%c' for option -i or --info\n"), *ptr); // Line 2401
        exit(1); // Line 2402
      ; // Line 2403
    } // Line 2404
    ptr++; // Line 2405
  } // Line 2406
  if (default_info) // Line 2407
    pFlag->file_info |= INFO_DEFAULT; // Line 2408
} // Line 2409
