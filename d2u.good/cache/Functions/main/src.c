int main (int argc, char *argv[]) // Line 550
{ // Line 551
  /* variable declarations */ // Line 552
  char progname[9]; // Line 553
  CFlag *pFlag; // Line 554
  char *ptr; // Line 555
  char localedir[1024]; // Line 556
  int ret; // Line 557
# ifdef __MINGW64__
  int _dowildcard = -1; /* enable wildcard expansion for Win64 */ // Line 559
# endif
  int  argc_new; // Line 561
  char **argv_new; // Line 562
#ifdef D2U_UNIFILE
  wchar_t **wargv; // Line 564
  char ***argv_glob; // Line 565
# endif

  progname[8] = '\0'; // Line 568
  strcpy(progname,"dos2unix"); // Line 569

#ifdef ENABLE_NLS
   ptr = getenv("DOS2UNIX_LOCALEDIR"); // Line 572
   if (ptr == NULL) // Line 573
      d2u_strncpy(localedir,LOCALEDIR,sizeof(localedir)); // Line 574
   else { // Line 575
      if (strlen(ptr) < sizeof(localedir)) // Line 576
         d2u_strncpy(localedir,ptr,sizeof(localedir)); // Line 577
      else { // Line 578
         D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 579
         D2U_ANSI_FPRINTF(stderr, "%s", _("error: Value of environment variable DOS2UNIX_LOCALEDIR is too long.\n")); // Line 580
         d2u_strncpy(localedir,LOCALEDIR,sizeof(localedir)); // Line 581
      } // Line 582
   } // Line 583
#endif

#if defined(ENABLE_NLS) || (defined(D2U_UNICODE) && !defined(__MSDOS__) && !defined(_WIN32) && !defined(__OS2__))
/* setlocale() is also needed for nl_langinfo() */ // Line 587
#if (defined(_WIN32) && !defined(__CYGWIN__))
/* When the locale is set to "" on Windows all East-Asian multi-byte ANSI encoded text is printed // Line 589
   wrongly when you use standard printf(). Also UTF-8 code is printed wrongly. See also test/setlocale.c. // Line 590
   When we set the locale to "C" gettext still translates the messages on Windows. On Unix this would disable // Line 591
   gettext. */ // Line 592
   setlocale (LC_ALL, "C"); // Line 593
#else
   setlocale (LC_ALL, ""); // Line 595
#endif
#endif

#ifdef ENABLE_NLS
   bindtextdomain (PACKAGE, localedir); // Line 600
   textdomain (PACKAGE); // Line 601
#endif


  /* variable initialisations */ // Line 605
  pFlag = (CFlag*)malloc(sizeof(CFlag)); // Line 606
  if (pFlag == NULL) { // Line 607
    D2U_UTF8_FPRINTF(stderr, "dos2unix:"); // Line 608
    D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno)); // Line 609
    return errno; // Line 610
  } // Line 611
  pFlag->FromToMode = FROMTO_DOS2UNIX;  /* default dos2unix */ // Line 612
  pFlag->keep_bom = 0; // Line 613

  if ( ((ptr=strrchr(argv[0],'/')) == NULL) && ((ptr=strrchr(argv[0],'\\')) == NULL) ) // Line 615
    ptr = argv[0]; // Line 616
  else // Line 617
    ptr++; // Line 618

  if ((strcmpi("mac2unix", ptr) == 0) || (strcmpi("mac2unix.exe", ptr) == 0)) { // Line 620
    pFlag->FromToMode = FROMTO_MAC2UNIX; // Line 621
    strcpy(progname,"mac2unix"); // Line 622
  } // Line 623

#ifdef D2U_UNIFILE
  /* Get arguments in wide Unicode format in the Windows Command Prompt */ // Line 626

  /* This does not support wildcard expansion (globbing) */ // Line 628
  wargv = CommandLineToArgvW(GetCommandLineW(), &argc); // Line 629

  argv_glob = (char ***)malloc(sizeof(char***)); // Line 631
  if (argv_glob == NULL) { // Line 632
    D2U_UTF8_FPRINTF(stderr, "%s:", progname); // Line 633
    D2U_ANSI_FPRINTF(stderr, " %s\n", strerror(errno)); // Line 634
    free(pFlag); // Line 635
    return errno; // Line 636
  } // Line 637
  /* Glob the arguments and convert them to UTF-8 */ // Line 638
  argc_new = glob_warg(argc, wargv, argv_glob, pFlag, progname); // Line 639
  argv_new = *argv_glob; // Line 640
#else  
  argc_new = argc; // Line 642
  argv_new = argv; // Line 643
#endif

#ifdef D2U_UNICODE
  ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense, ConvertDosToUnix, ConvertDosToUnixW); // Line 647
#else
  ret = parse_options(argc_new, argv_new, pFlag, localedir, progname, PrintLicense, ConvertDosToUnix); // Line 649
#endif
  free(pFlag); // Line 651
  return ret; // Line 652
} // Line 653
