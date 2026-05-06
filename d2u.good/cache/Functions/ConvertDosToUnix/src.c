int ConvertDosToUnix(FILE* ipInF, FILE* ipOutF, CFlag *ipFlag, const char *progname) // Line 348
{ // Line 349
    int RetVal = 0; // Line 350
    int PrevChar = EOF; // Line 351
    int TempChar; // Line 352
    int TempNextChar; // Line 353
    int *ConvTable; // Line 354
    unsigned int line_nr = 1; // Line 355
    unsigned int converted = 0; // Line 356

    ipFlag->status = 0; // Line 358

    switch (ipFlag->ConvMode) { // Line 360
      case CONVMODE_ASCII: /* ascii */ // Line 361
      case CONVMODE_UTF16LE: /* Assume UTF-16LE, bomtype = FILE_UTF8 or GB18030 */ // Line 362
      case CONVMODE_UTF16BE: /* Assume UTF-16BE, bomtype = FILE_UTF8 or GB18030 */ // Line 363
        ConvTable = D2UAsciiTable; // Line 364
        break; // Line 365
      case CONVMODE_7BIT: /* 7bit */ // Line 366
        ConvTable = D2U7BitTable; // Line 367
        break; // Line 368
      case CONVMODE_437: /* iso */ // Line 369
        ConvTable = D2UIso437Table; // Line 370
        break; // Line 371
      case CONVMODE_850: /* iso */ // Line 372
        ConvTable = D2UIso850Table; // Line 373
        break; // Line 374
      case CONVMODE_860: /* iso */ // Line 375
        ConvTable = D2UIso860Table; // Line 376
        break; // Line 377
      case CONVMODE_863: /* iso */ // Line 378
        ConvTable = D2UIso863Table; // Line 379
        break; // Line 380
      case CONVMODE_865: /* iso */ // Line 381
        ConvTable = D2UIso865Table; // Line 382
        break; // Line 383
      case CONVMODE_1252: /* iso */ // Line 384
        ConvTable = D2UIso1252Table; // Line 385
        break; // Line 386
      default: /* unknown convmode */ // Line 387
        ipFlag->status |= WRONG_CODEPAGE ; // Line 388
        return(-1); // Line 389
    } // Line 390
    /* Turn off ISO and 7-bit conversion for Unicode text files */ // Line 391
    if (ipFlag->bomtype > 0) // Line 392
      ConvTable = D2UAsciiTable; // Line 393

    if ((ipFlag->ConvMode > CONVMODE_7BIT) && (ipFlag->verbose)) { /* not ascii or 7bit */ // Line 395
       D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 396
       D2U_UTF8_FPRINTF(stderr, _("using code page %d.\n"), ipFlag->ConvMode); // Line 397
    } // Line 398

    /* CR-LF -> LF */ // Line 400
    /* LF    -> LF, in case the input file is a Unix text file */ // Line 401
    /* CR    -> CR, in dos2unix mode (don't modify Mac file) */ // Line 402
    /* CR    -> LF, in Mac mode */ // Line 403
    /* \x0a = Newline/Line Feed (LF) */ // Line 404
    /* \x0d = Carriage Return (CR) */ // Line 405

    switch (ipFlag->FromToMode) { // Line 407
      case FROMTO_DOS2UNIX: /* dos2unix */ // Line 408
        while ((TempChar = fgetc(ipInF)) != EOF) {  /* get character */ // Line 409
          if ((ipFlag->Force == 0) && // Line 410
              (TempChar < 32) && // Line 411
              (TempChar != '\x0a') &&  /* Not an LF */ // Line 412
              (TempChar != '\x0d') &&  /* Not a CR */ // Line 413
              (TempChar != '\x09') &&  /* Not a TAB */ // Line 414
              (TempChar != '\x0c')) {  /* Not a form feed */ // Line 415
            RetVal = -1; // Line 416
            ipFlag->status |= BINARY_FILE ; // Line 417
            if (ipFlag->verbose) { // Line 418
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 419
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 420
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x%02X found at line %u\n"),TempChar, line_nr); // Line 421
            } // Line 422
            break; // Line 423
          } // Line 424
          if (TempChar != '\x0d') { // Line 425
            if (TempChar == '\x0a') /* Count all DOS and Unix line breaks */ // Line 426
              ++line_nr; // Line 427
            if (fputc(ConvTable[TempChar], ipOutF) == EOF) { // Line 428
              RetVal = -1; // Line 429
              d2u_putc_error(ipFlag,progname); // Line 430
              break; // Line 431
            } // Line 432
          } else { // Line 433
            if (StripDelimiter( ipInF, ipOutF, ipFlag, TempChar, &converted, progname) == EOF) { // Line 434
              RetVal = -1; // Line 435
              break; // Line 436
            } // Line 437
          } // Line 438
          PrevChar = TempChar; // Line 439
        } // Line 440
        if (TempChar == EOF && ipFlag->add_eol && PrevChar != EOF && PrevChar != '\x0a') { // Line 441
          /* Add missing line break at the last line. */ // Line 442
            if (ipFlag->verbose > 1) { // Line 443
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 444
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 445
            } // Line 446
           if (fputc('\x0a', ipOutF) == EOF) { // Line 447
              RetVal = -1; // Line 448
              d2u_putc_error(ipFlag,progname); // Line 449
            } // Line 450
        } // Line 451
        if ((TempChar == EOF) && ferror(ipInF)) { // Line 452
          RetVal = -1; // Line 453
          d2u_getc_error(ipFlag,progname); // Line 454
        } // Line 455
        break; // Line 456
      case FROMTO_MAC2UNIX: /* mac2unix */ // Line 457
        while ((TempChar = fgetc(ipInF)) != EOF) { // Line 458
          if ((ipFlag->Force == 0) && // Line 459
              (TempChar < 32) && // Line 460
              (TempChar != '\x0a') &&  /* Not an LF */ // Line 461
              (TempChar != '\x0d') &&  /* Not a CR */ // Line 462
              (TempChar != '\x09') &&  /* Not a TAB */ // Line 463
              (TempChar != '\x0c')) {  /* Not a form feed */ // Line 464
            RetVal = -1; // Line 465
            ipFlag->status |= BINARY_FILE ; // Line 466
            if (ipFlag->verbose) { // Line 467
              if ((ipFlag->stdio_mode) && (!ipFlag->error)) ipFlag->error = 1; // Line 468
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 469
              D2U_UTF8_FPRINTF(stderr, _("Binary symbol 0x%02X found at line %u\n"),TempChar, line_nr); // Line 470
            } // Line 471
            break; // Line 472
          } // Line 473
          if ((TempChar != '\x0d')) { // Line 474
            if (TempChar == '\x0a') /* Count all DOS and Unix line breaks */ // Line 475
              ++line_nr; // Line 476
            if(fputc(ConvTable[TempChar], ipOutF) == EOF) { // Line 477
              RetVal = -1; // Line 478
              d2u_putc_error(ipFlag,progname); // Line 479
              break; // Line 480
            } // Line 481
          } else{ // Line 482
            /* TempChar is a CR */ // Line 483
            if ( (TempNextChar = fgetc(ipInF)) != EOF) { // Line 484
              if (ungetc( TempNextChar, ipInF ) == EOF) {  /* put back peek char */ // Line 485
                d2u_getc_error(ipFlag,progname); // Line 486
                RetVal = -1; // Line 487
                break; // Line 488
              } // Line 489
              /* Don't touch this delimiter if it's a CR,LF pair. */ // Line 490
              if ( TempNextChar == '\x0a' ) { // Line 491
                if (fputc('\x0d', ipOutF) == EOF) { /* put CR, part of DOS CR-LF */ // Line 492
                  RetVal = -1; // Line 493
                  d2u_putc_error(ipFlag,progname); // Line 494
                  break; // Line 495
                } // Line 496
                PrevChar = TempChar; // Line 497
                continue; // Line 498
              } // Line 499
            } // Line 500
            if (fputc('\x0a', ipOutF) == EOF) { /* MAC line end (CR). Put LF */ // Line 501
              RetVal = -1; // Line 502
              d2u_putc_error(ipFlag,progname); // Line 503
              break; // Line 504
            } // Line 505
            converted++; // Line 506
            line_nr++; /* Count all Mac line breaks */ // Line 507
            if (ipFlag->NewLine) {  /* add additional LF? */ // Line 508
              if (fputc('\x0a', ipOutF) == EOF) { // Line 509
                RetVal = -1; // Line 510
                d2u_putc_error(ipFlag,progname); // Line 511
                break; // Line 512
              } // Line 513
            } // Line 514
          } // Line 515
          PrevChar = TempChar; // Line 516
        } // Line 517
        if (TempChar == EOF && ipFlag->add_eol && PrevChar != EOF && !(PrevChar == '\x0a' || PrevChar == '\x0d')) { // Line 518
          /* Add missing line break at the last line. */ // Line 519
            if (ipFlag->verbose > 1) { // Line 520
              D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 521
              D2U_UTF8_FPRINTF(stderr, _("Added line break to last line.\n")); // Line 522
            } // Line 523
            if (fputc('\x0a', ipOutF) == EOF) { // Line 524
              RetVal = -1; // Line 525
              d2u_putc_error(ipFlag,progname); // Line 526
            } // Line 527
        } // Line 528
        if ((TempChar == EOF) && ferror(ipInF)) { // Line 529
          RetVal = -1; // Line 530
          d2u_getc_error(ipFlag,progname); // Line 531
        } // Line 532
        break; // Line 533
      default: /* unknown FromToMode */ // Line 534
      ; // Line 535
#if DEBUG
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 537
      D2U_UTF8_FPRINTF(stderr, _("program error, invalid conversion mode %d\n"),ipFlag->FromToMode); // Line 538
      exit(1); // Line 539
#endif
    } // Line 541
    if ((RetVal == 0) && (ipFlag->verbose > 1)) { // Line 542
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 543
      D2U_UTF8_FPRINTF(stderr, _("Converted %u out of %u line breaks.\n"),converted, line_nr -1); // Line 544
    } // Line 545
    return RetVal; // Line 546
} // Line 547
