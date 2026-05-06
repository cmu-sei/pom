void PrintVersion(const char *progname, const char *localedir) // Line 720
{ // Line 721
  D2U_ANSI_FPRINTF(stdout,"%s %s (%s)\n", progname, VER_REVISION, VER_DATE); // Line 722
#if DEBUG
  D2U_ANSI_FPRINTF(stdout,"VER_AUTHOR: %s\n", VER_AUTHOR); // Line 724
#endif
#if defined(__WATCOMC__) && defined(__I86__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 16 bit version (WATCOMC).\n")); // Line 727
#elif defined(__TURBOC__) && defined(__MSDOS__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 16 bit version (TURBOC).\n")); // Line 729
#elif defined(__WATCOMC__) && defined(__DOS__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 32 bit version (WATCOMC).\n")); // Line 731
#elif defined(__DJGPP__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("DOS 32 bit version (DJGPP).\n")); // Line 733
#elif defined(__MSYS__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("MSYS version.\n")); // Line 735
#elif defined(__CYGWIN__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Cygwin version.\n")); // Line 737
#elif defined(__WIN64__) && defined(__MINGW64__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 64 bit version (MinGW-w64).\n")); // Line 739
#elif defined(__WATCOMC__) && defined(__NT__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 32 bit version (WATCOMC).\n")); // Line 741
#elif defined(_WIN32) && defined(__MINGW32__) && (D2U_COMPILER == MINGW32_W64)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 32 bit version (MinGW-w64).\n")); // Line 743
#elif defined(_WIN32) && defined(__MINGW32__)
  D2U_ANSI_FPRINTF(stdout,"%s", _("Windows 32 bit version (MinGW).\n")); // Line 745
#elif defined(_WIN64) && defined(_MSC_VER)
  D2U_ANSI_FPRINTF(stdout,_("Windows 64 bit version (MSVC %d).\n"),_MSC_VER); // Line 747
#elif defined(_WIN32) && defined(_MSC_VER)
  D2U_ANSI_FPRINTF(stdout,_("Windows 32 bit version (MSVC %d).\n"),_MSC_VER); // Line 749
#elif defined (__OS2__) && defined(__WATCOMC__) /* OS/2 Warp */
  D2U_ANSI_FPRINTF(stdout,"%s", _("OS/2 version (WATCOMC).\n")); // Line 751
#elif defined (__OS2__) && defined(__EMX__) /* OS/2 Warp */
  D2U_ANSI_FPRINTF(stdout,"%s", _("OS/2 version (EMX).\n")); // Line 753
#elif defined(__OS)
  D2U_ANSI_FPRINTF(stdout,_("%s version.\n"), __OS); // Line 755
#endif
#if defined(_WIN32) && defined(WINVER)
  D2U_ANSI_FPRINTF(stdout,"WINVER 0x%X\n",WINVER); // Line 758
#endif
#ifdef D2U_UNICODE
  D2U_ANSI_FPRINTF(stdout,"%s", _("With Unicode UTF-16 support.\n")); // Line 761
#else
  D2U_ANSI_FPRINTF(stdout,"%s", _("Without Unicode UTF-16 support.\n")); // Line 763
#endif
#ifdef _WIN32
#ifdef D2U_UNIFILE
  D2U_ANSI_FPRINTF(stdout,"%s", _("With Unicode file name support.\n")); // Line 767
#else
  D2U_ANSI_FPRINTF(stdout,"%s", _("Without Unicode file name support.\n")); // Line 769
#endif
#endif
#ifdef ENABLE_NLS
  D2U_ANSI_FPRINTF(stdout,"%s", _("With native language support.\n")); // Line 773
#else
  D2U_ANSI_FPRINTF(stdout,"%s", "Without native language support.\n"); // Line 775
#endif
#ifndef NO_CHOWN
  D2U_ANSI_FPRINTF(stdout,"%s", _("With support to preserve the user and group ownership of files.\n")); // Line 778
#else
  D2U_ANSI_FPRINTF(stdout,"%s", _("Without support to preserve the user and group ownership of files.\n")); // Line 780
#endif
#ifdef ENABLE_NLS
  D2U_ANSI_FPRINTF(stdout,"LOCALEDIR: %s\n", localedir); // Line 783
#endif
  D2U_ANSI_FPRINTF(stdout,"https://waterlan.home.xs4all.nl/dos2unix.html\n"); // Line 785
  D2U_ANSI_FPRINTF(stdout,"https://dos2unix.sourceforge.io/\n"); // Line 786
} // Line 787
