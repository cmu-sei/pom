int parse_options(int argc, char *argv[], // Line 2411
                  CFlag *pFlag, const char *localedir, const char *progname, // Line 2412
                  void (*PrintLicense)(void), // Line 2413
                  int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 2414
#ifdef D2U_UNICODE
                , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 2416
#endif
                  ) // Line 2418
{ // Line 2419
  int ArgIdx = 0; // Line 2420
  int ShouldExit = 0; // Line 2421
  int CanSwitchFileMode = 1; // Line 2422
  int process_options = 1; // Line 2423
#ifdef D2U_UNIFILE
  char *ptr; // Line 2425
#endif

  /* variable initialisations */ // Line 2428
  pFlag->NewFile = 0; // Line 2429
  pFlag->verbose = 1; // Line 2430
  pFlag->KeepDate = 0; // Line 2431
  pFlag->ConvMode = CONVMODE_ASCII;  /* default ascii */ // Line 2432
  pFlag->NewLine = 0; // Line 2433
  pFlag->Force = 0; // Line 2434
  pFlag->Follow = SYMLINK_SKIP; // Line 2435
  pFlag->status = 0; // Line 2436
  pFlag->stdio_mode = 1; // Line 2437
  pFlag->to_stdout = 0; // Line 2438
  pFlag->error = 0; // Line 2439
  pFlag->bomtype = FILE_MBS; // Line 2440
  pFlag->add_bom = 0; // Line 2441
  pFlag->keep_utf16 = 0; // Line 2442
  pFlag->file_info = 0; // Line 2443
  pFlag->locale_target = TARGET_UTF8; // Line 2444
  pFlag->add_eol = 0; // Line 2445

#ifdef D2U_UNIFILE
   ptr = getenv("DOS2UNIX_DISPLAY_ENC"); // Line 2448
   if (ptr != NULL) { // Line 2449
      if (strncmp(ptr, "ansi", sizeof("ansi")) == 0) // Line 2450
         d2u_display_encoding = D2U_DISPLAY_ANSI; // Line 2451
      else if (strncmp(ptr, "unicode", sizeof("unicode")) == 0) // Line 2452
         d2u_display_encoding = D2U_DISPLAY_UNICODE; // Line 2453
      else if (strncmp(ptr, "unicodebom", sizeof("unicodebom")) == 0) // Line 2454
         d2u_display_encoding = D2U_DISPLAY_UNICODEBOM; // Line 2455
      else if (strncmp(ptr, "utf8", sizeof("utf8")) == 0) // Line 2456
         d2u_display_encoding = D2U_DISPLAY_UTF8; // Line 2457
      else if (strncmp(ptr, "utf8bom", sizeof("utf8bom")) == 0) // Line 2458
         d2u_display_encoding = D2U_DISPLAY_UTF8BOM; // Line 2459
   } // Line 2460
#endif

  while ((++ArgIdx < argc) && (!ShouldExit)) // Line 2463
  { // Line 2464
    /* is it an option? */ // Line 2465
    if ((argv[ArgIdx][0] == '-') && process_options) // Line 2466
    { // Line 2467
      /* an option */ // Line 2468
      if (strcmp(argv[ArgIdx],"--") == 0) // Line 2469
        process_options = 0; // Line 2470
      else if ((strcmp(argv[ArgIdx],"-h") == 0) || (strcmp(argv[ArgIdx],"--help") == 0)) // Line 2471
      { // Line 2472
        PrintUsage(progname); // Line 2473
        return(pFlag->error); // Line 2474
      } // Line 2475
      else if ((strcmp(argv[ArgIdx],"-b") == 0) || (strcmp(argv[ArgIdx],"--keep-bom") == 0)) // Line 2476
        pFlag->keep_bom = 1; // Line 2477
      else if ((strcmp(argv[ArgIdx],"-k") == 0) || (strcmp(argv[ArgIdx],"--keepdate") == 0)) // Line 2478
        pFlag->KeepDate = 1; // Line 2479
      else if ((strcmp(argv[ArgIdx],"-e") == 0) || (strcmp(argv[ArgIdx],"--add-eol") == 0)) // Line 2480
        pFlag->add_eol = 1; // Line 2481
      else if (strcmp(argv[ArgIdx],"--no-add-eol") == 0) // Line 2482
        pFlag->add_eol = 0; // Line 2483
      else if ((strcmp(argv[ArgIdx],"-f") == 0) || (strcmp(argv[ArgIdx],"--force") == 0)) // Line 2484
        pFlag->Force = 1; // Line 2485
#ifndef NO_CHOWN
      else if (strcmp(argv[ArgIdx],"--allow-chown") == 0) // Line 2487
        pFlag->AllowChown = 1; // Line 2488
      else if (strcmp(argv[ArgIdx],"--no-allow-chown") == 0) // Line 2489
        pFlag->AllowChown = 0; // Line 2490
#endif
#ifdef D2U_UNICODE
#if (defined(_WIN32) && !defined(__CYGWIN__))
      else if ((strcmp(argv[ArgIdx],"-gb") == 0) || (strcmp(argv[ArgIdx],"--gb18030") == 0)) // Line 2494
        pFlag->locale_target = TARGET_GB18030; // Line 2495
#endif
#endif
      else if ((strcmp(argv[ArgIdx],"-s") == 0) || (strcmp(argv[ArgIdx],"--safe") == 0)) // Line 2498
        pFlag->Force = 0; // Line 2499
      else if ((strcmp(argv[ArgIdx],"-q") == 0) || (strcmp(argv[ArgIdx],"--quiet") == 0)) // Line 2500
        pFlag->verbose = 0; // Line 2501
      else if ((strcmp(argv[ArgIdx],"-v") == 0) || (strcmp(argv[ArgIdx],"--verbose") == 0)) // Line 2502
        pFlag->verbose = 2; // Line 2503
      else if ((strcmp(argv[ArgIdx],"-l") == 0) || (strcmp(argv[ArgIdx],"--newline") == 0)) // Line 2504
        pFlag->NewLine = 1; // Line 2505
      else if ((strcmp(argv[ArgIdx],"-m") == 0) || (strcmp(argv[ArgIdx],"--add-bom") == 0)) // Line 2506
        pFlag->add_bom = 1; // Line 2507
      else if ((strcmp(argv[ArgIdx],"-r") == 0) || (strcmp(argv[ArgIdx],"--remove-bom") == 0)) { // Line 2508
        pFlag->keep_bom = 0; // Line 2509
        pFlag->add_bom = 0; // Line 2510
      } // Line 2511
      else if ((strcmp(argv[ArgIdx],"-S") == 0) || (strcmp(argv[ArgIdx],"--skip-symlink") == 0)) // Line 2512
        pFlag->Follow = SYMLINK_SKIP; // Line 2513
      else if ((strcmp(argv[ArgIdx],"-F") == 0) || (strcmp(argv[ArgIdx],"--follow-symlink") == 0)) // Line 2514
        pFlag->Follow = SYMLINK_FOLLOW; // Line 2515
      else if ((strcmp(argv[ArgIdx],"-R") == 0) || (strcmp(argv[ArgIdx],"--replace-symlink") == 0)) // Line 2516
        pFlag->Follow = SYMLINK_REPLACE; // Line 2517
      else if ((strcmp(argv[ArgIdx],"-V") == 0) || (strcmp(argv[ArgIdx],"--version") == 0)) { // Line 2518
        PrintVersion(progname, localedir); // Line 2519
        return(pFlag->error); // Line 2520
      } // Line 2521
      else if ((strcmp(argv[ArgIdx],"-L") == 0) || (strcmp(argv[ArgIdx],"--license") == 0)) { // Line 2522
        PrintLicense(); // Line 2523
        return(pFlag->error); // Line 2524
      } // Line 2525
      else if (strcmp(argv[ArgIdx],"-ascii") == 0) { /* SunOS compatible options */ // Line 2526
        pFlag->ConvMode = CONVMODE_ASCII; // Line 2527
        pFlag->keep_utf16 = 0; // Line 2528
        pFlag->locale_target = TARGET_UTF8; // Line 2529
      } // Line 2530
      else if (strcmp(argv[ArgIdx],"-7") == 0) // Line 2531
        pFlag->ConvMode = CONVMODE_7BIT; // Line 2532
      else if (strcmp(argv[ArgIdx],"-iso") == 0) { // Line 2533
        pFlag->ConvMode = (int)query_con_codepage(); // Line 2534
        if (pFlag->verbose) { // Line 2535
           D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2536
           D2U_UTF8_FPRINTF(stderr,_("active code page: %d\n"), pFlag->ConvMode); // Line 2537
        } // Line 2538
        if (pFlag->ConvMode < 2) // Line 2539
           pFlag->ConvMode = CONVMODE_437; // Line 2540
      } // Line 2541
      else if (strcmp(argv[ArgIdx],"-437") == 0) // Line 2542
        pFlag->ConvMode = CONVMODE_437; // Line 2543
      else if (strcmp(argv[ArgIdx],"-850") == 0) // Line 2544
        pFlag->ConvMode = CONVMODE_850; // Line 2545
      else if (strcmp(argv[ArgIdx],"-860") == 0) // Line 2546
        pFlag->ConvMode = CONVMODE_860; // Line 2547
      else if (strcmp(argv[ArgIdx],"-863") == 0) // Line 2548
        pFlag->ConvMode = CONVMODE_863; // Line 2549
      else if (strcmp(argv[ArgIdx],"-865") == 0) // Line 2550
        pFlag->ConvMode = CONVMODE_865; // Line 2551
      else if (strcmp(argv[ArgIdx],"-1252") == 0) // Line 2552
        pFlag->ConvMode = CONVMODE_1252; // Line 2553
#ifdef D2U_UNICODE
      else if ((strcmp(argv[ArgIdx],"-u") == 0) || (strcmp(argv[ArgIdx],"--keep-utf16") == 0)) // Line 2555
        pFlag->keep_utf16 = 1; // Line 2556
      else if ((strcmp(argv[ArgIdx],"-ul") == 0) || (strcmp(argv[ArgIdx],"--assume-utf16le") == 0)) // Line 2557
        pFlag->ConvMode = CONVMODE_UTF16LE; // Line 2558
      else if ((strcmp(argv[ArgIdx],"-ub") == 0) || (strcmp(argv[ArgIdx],"--assume-utf16be") == 0)) // Line 2559
        pFlag->ConvMode = CONVMODE_UTF16BE; // Line 2560
#endif
      else if (strcmp(argv[ArgIdx],"--info") == 0) // Line 2562
        pFlag->file_info |= INFO_DEFAULT; // Line 2563
      else if (strncmp(argv[ArgIdx],"--info=", (size_t)7) == 0) { // Line 2564
        get_info_options(argv[ArgIdx]+7, pFlag, progname); // Line 2565
      } else if (strncmp(argv[ArgIdx],"-i", (size_t)2) == 0) { // Line 2566
        get_info_options(argv[ArgIdx]+2, pFlag, progname); // Line 2567
      } else if ((strcmp(argv[ArgIdx],"-c") == 0) || (strcmp(argv[ArgIdx],"--convmode") == 0)) { // Line 2568
        if (++ArgIdx < argc) { // Line 2569
          if (strcmpi(argv[ArgIdx],"ascii") == 0) { /* Benjamin Lin's legacy options */ // Line 2570
            pFlag->ConvMode = CONVMODE_ASCII; // Line 2571
            pFlag->keep_utf16 = 0; // Line 2572
          } // Line 2573
          else if (strcmpi(argv[ArgIdx], "7bit") == 0) // Line 2574
            pFlag->ConvMode = CONVMODE_7BIT; // Line 2575
          else if (strcmpi(argv[ArgIdx], "iso") == 0) { // Line 2576
            pFlag->ConvMode = (int)query_con_codepage(); // Line 2577
            if (pFlag->verbose) { // Line 2578
               D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2579
               D2U_UTF8_FPRINTF(stderr,_("active code page: %d\n"), pFlag->ConvMode); // Line 2580
            } // Line 2581
            if (pFlag->ConvMode < 2) // Line 2582
               pFlag->ConvMode = CONVMODE_437; // Line 2583
          } // Line 2584
          else if (strcmpi(argv[ArgIdx], "mac") == 0) { // Line 2585
            if (is_dos2unix(progname)) // Line 2586
              pFlag->FromToMode = FROMTO_MAC2UNIX; // Line 2587
            else // Line 2588
              pFlag->FromToMode = FROMTO_UNIX2MAC; // Line 2589
          } else { // Line 2590
            D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2591
            D2U_UTF8_FPRINTF(stderr, _("invalid %s conversion mode specified\n"),argv[ArgIdx]); // Line 2592
            pFlag->error = 1; // Line 2593
            ShouldExit = 1; // Line 2594
            pFlag->stdio_mode = 0; // Line 2595
          } // Line 2596
        } else { // Line 2597
          ArgIdx--; // Line 2598
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2599
          D2U_UTF8_FPRINTF(stderr,_("option '%s' requires an argument\n"),argv[ArgIdx]); // Line 2600
          pFlag->error = 1; // Line 2601
          ShouldExit = 1; // Line 2602
          pFlag->stdio_mode = 0; // Line 2603
        } // Line 2604
      } // Line 2605

#ifdef D2U_UNIFILE
      else if ((strcmp(argv[ArgIdx],"-D") == 0) || (strcmp(argv[ArgIdx],"--display-enc") == 0)) { // Line 2608
        if (++ArgIdx < argc) { // Line 2609
          if (strcmpi(argv[ArgIdx],"ansi") == 0) // Line 2610
            d2u_display_encoding = D2U_DISPLAY_ANSI; // Line 2611
          else if (strcmpi(argv[ArgIdx], "unicode") == 0) // Line 2612
            d2u_display_encoding = D2U_DISPLAY_UNICODE; // Line 2613
          else if (strcmpi(argv[ArgIdx], "unicodebom") == 0) // Line 2614
            d2u_display_encoding = D2U_DISPLAY_UNICODEBOM; // Line 2615
          else if (strcmpi(argv[ArgIdx], "utf8") == 0) // Line 2616
            d2u_display_encoding = D2U_DISPLAY_UTF8; // Line 2617
          else if (strcmpi(argv[ArgIdx], "utf8bom") == 0) { // Line 2618
            d2u_display_encoding = D2U_DISPLAY_UTF8BOM; // Line 2619
          } else { // Line 2620
            D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2621
            D2U_UTF8_FPRINTF(stderr, _("invalid %s display encoding specified\n"),argv[ArgIdx]); // Line 2622
            pFlag->error = 1; // Line 2623
            ShouldExit = 1; // Line 2624
            pFlag->stdio_mode = 0; // Line 2625
          } // Line 2626
        } else { // Line 2627
          ArgIdx--; // Line 2628
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2629
          D2U_UTF8_FPRINTF(stderr,_("option '%s' requires an argument\n"),argv[ArgIdx]); // Line 2630
          pFlag->error = 1; // Line 2631
          ShouldExit = 1; // Line 2632
          pFlag->stdio_mode = 0; // Line 2633
        } // Line 2634
      } // Line 2635
#endif

      else if ((strcmp(argv[ArgIdx],"-o") == 0) || (strcmp(argv[ArgIdx],"--oldfile") == 0)) { // Line 2638
        /* last convert not paired */ // Line 2639
        if (!CanSwitchFileMode) { // Line 2640
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2641
          D2U_UTF8_FPRINTF(stderr, _("target of file %s not specified in new-file mode\n"), argv[ArgIdx-1]); // Line 2642
          pFlag->error = 1; // Line 2643
          ShouldExit = 1; // Line 2644
          pFlag->stdio_mode = 0; // Line 2645
        } // Line 2646
        pFlag->NewFile = 0; // Line 2647
        pFlag->file_info = 0; // Line 2648
        pFlag->to_stdout = 0; // Line 2649
      } // Line 2650

      else if ((strcmp(argv[ArgIdx],"-n") == 0) || (strcmp(argv[ArgIdx],"--newfile") == 0)) { // Line 2652
        /* last convert not paired */ // Line 2653
        if (!CanSwitchFileMode) { // Line 2654
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2655
          D2U_UTF8_FPRINTF(stderr, _("target of file %s not specified in new-file mode\n"), argv[ArgIdx-1]); // Line 2656
          pFlag->error = 1; // Line 2657
          ShouldExit = 1; // Line 2658
          pFlag->stdio_mode = 0; // Line 2659
        } // Line 2660
        pFlag->NewFile = 1; // Line 2661
        pFlag->file_info = 0; // Line 2662
      } // Line 2663
      else if ((strcmp(argv[ArgIdx],"-O") == 0) || (strcmp(argv[ArgIdx],"--to-stdout") == 0)) { // Line 2664
        /* last convert not paired */ // Line 2665
        if (!CanSwitchFileMode) { // Line 2666
          D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2667
          D2U_UTF8_FPRINTF(stderr, _("target of file %s not specified in new-file mode\n"), argv[ArgIdx-1]); // Line 2668
          pFlag->error = 1; // Line 2669
          ShouldExit = 1; // Line 2670
          pFlag->stdio_mode = 0; // Line 2671
        } // Line 2672
        pFlag->NewFile = 0; // Line 2673
        pFlag->to_stdout = 1; // Line 2674
      } // Line 2675
      else { /* wrong option */ // Line 2676
        PrintUsage(progname); // Line 2677
        ShouldExit = 1; // Line 2678
        pFlag->error = 1; // Line 2679
        pFlag->stdio_mode = 0; // Line 2680
      } // Line 2681
    } else { // Line 2682
      /* not an option */ // Line 2683
      int conversion_error; // Line 2684
      pFlag->stdio_mode = 0; // Line 2685
      if (pFlag->NewFile) { // Line 2686
        if (CanSwitchFileMode) // Line 2687
          CanSwitchFileMode = 0; // Line 2688
        else { // Line 2689
#ifdef D2U_UNICODE
          conversion_error = ConvertNewFile(argv[ArgIdx-1], argv[ArgIdx], pFlag, progname, Convert, ConvertW); // Line 2691
#else
          conversion_error = ConvertNewFile(argv[ArgIdx-1], argv[ArgIdx], pFlag, progname, Convert); // Line 2693
#endif
          if (pFlag->verbose) // Line 2695
            print_messages(pFlag, argv[ArgIdx-1], argv[ArgIdx], progname, conversion_error); // Line 2696
          CanSwitchFileMode = 1; // Line 2697
        } // Line 2698
      } else { // Line 2699
        if (pFlag->file_info) { // Line 2700
          conversion_error = GetFileInfo(argv[ArgIdx], pFlag, progname); // Line 2701
          print_messages_info(pFlag, argv[ArgIdx], progname); // Line 2702
        } else { // Line 2703
          /* Old file mode */ // Line 2704
          if (pFlag->to_stdout) { // Line 2705
#ifdef D2U_UNICODE
            conversion_error = ConvertToStdout(argv[ArgIdx], pFlag, progname, Convert, ConvertW); // Line 2707
#else
            conversion_error = ConvertToStdout(argv[ArgIdx], pFlag, progname, Convert); // Line 2709
#endif
          } else { // Line 2711
#ifdef D2U_UNICODE
            conversion_error = ConvertNewFile(argv[ArgIdx], argv[ArgIdx], pFlag, progname, Convert, ConvertW); // Line 2713
#else
            conversion_error = ConvertNewFile(argv[ArgIdx], argv[ArgIdx], pFlag, progname, Convert); // Line 2715
#endif
          } // Line 2717
          if (pFlag->verbose) // Line 2718
            print_messages(pFlag, argv[ArgIdx], NULL, progname, conversion_error); // Line 2719
        } // Line 2720
      } // Line 2721
    } // Line 2722
  } // Line 2723

  /* no file argument, use stdin and stdout */ // Line 2725
  if ( (argc > 0) && pFlag->stdio_mode) { // Line 2726
    if (pFlag->file_info) { // Line 2727
      GetFileInfoStdio(pFlag, progname); // Line 2728
      print_messages_info(pFlag, "stdin", progname); // Line 2729
    } else { // Line 2730
#ifdef D2U_UNICODE
      ConvertStdio(pFlag, progname, Convert, ConvertW); // Line 2732
#else
      ConvertStdio(pFlag, progname, Convert); // Line 2734
#endif
      if (pFlag->verbose) // Line 2736
        print_messages_stdio(pFlag, progname); // Line 2737
    } // Line 2738
    return pFlag->error; // Line 2739
  } // Line 2740

  return pFlag->error; // Line 2742
} // Line 2743
