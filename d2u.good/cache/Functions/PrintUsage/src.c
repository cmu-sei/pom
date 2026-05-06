void PrintUsage(const char *progname) // Line 646
{ // Line 647
  D2U_ANSI_FPRINTF(stdout,_("Usage: %s [options] [file ...] [-n infile outfile ...]\n"), progname); // Line 648
#ifndef NO_CHOWN
  D2U_ANSI_FPRINTF(stdout,_(" --allow-chown         allow file ownership change\n")); // Line 650
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -ascii                default conversion mode\n")); // Line 652
  D2U_ANSI_FPRINTF(stdout,_(" -iso                  conversion between DOS and ISO-8859-1 character set\n")); // Line 653
  D2U_ANSI_FPRINTF(stdout,_("   -1252               use Windows code page 1252 (Western European)\n")); // Line 654
  D2U_ANSI_FPRINTF(stdout,_("   -437                use DOS code page 437 (US) (default)\n")); // Line 655
  D2U_ANSI_FPRINTF(stdout,_("   -850                use DOS code page 850 (Western European)\n")); // Line 656
  D2U_ANSI_FPRINTF(stdout,_("   -860                use DOS code page 860 (Portuguese)\n")); // Line 657
  D2U_ANSI_FPRINTF(stdout,_("   -863                use DOS code page 863 (French Canadian)\n")); // Line 658
  D2U_ANSI_FPRINTF(stdout,_("   -865                use DOS code page 865 (Nordic)\n")); // Line 659
  D2U_ANSI_FPRINTF(stdout,_(" -7                    convert 8 bit characters to 7 bit space\n")); // Line 660
  if (is_dos2unix(progname)) // Line 661
    D2U_ANSI_FPRINTF(stdout,_(" -b, --keep-bom        keep Byte Order Mark\n")); // Line 662
  else // Line 663
    D2U_ANSI_FPRINTF(stdout,_(" -b, --keep-bom        keep Byte Order Mark (default)\n")); // Line 664
  D2U_ANSI_FPRINTF(stdout,_(" -c, --convmode        conversion mode\n\
   convmode            ascii, 7bit, iso, mac, default to ascii\n")); // Line 666
#ifdef D2U_UNIFILE
  D2U_ANSI_FPRINTF(stdout,_(" -D, --display-enc     set encoding of displayed text messages\n\
   encoding            ansi, unicode, utf8, default to ansi\n")); // Line 669
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -e, --add-eol         add a line break to the last line if there isn't one\n")); // Line 671
  D2U_ANSI_FPRINTF(stdout,_(" -f, --force           force conversion of binary files\n")); // Line 672
#ifdef D2U_UNICODE
#if (defined(_WIN32) && !defined(__CYGWIN__))
  D2U_ANSI_FPRINTF(stdout,_(" -gb, --gb18030        convert UTF-16 to GB18030\n")); // Line 675
#endif
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -h, --help            display this help text\n")); // Line 678
  D2U_ANSI_FPRINTF(stdout,_(" -i, --info[=FLAGS]    display file information\n\
   file ...            files to analyze\n")); // Line 680
  D2U_ANSI_FPRINTF(stdout,_(" -k, --keepdate        keep output file date\n")); // Line 681
  D2U_ANSI_FPRINTF(stdout,_(" -L, --license         display software license\n")); // Line 682
  D2U_ANSI_FPRINTF(stdout,_(" -l, --newline         add additional newline\n")); // Line 683
  D2U_ANSI_FPRINTF(stdout,_(" -m, --add-bom         add Byte Order Mark (default UTF-8)\n")); // Line 684
  D2U_ANSI_FPRINTF(stdout,_(" -n, --newfile         write to new file\n\
   infile              original file in new-file mode\n\
   outfile             output file in new-file mode\n")); // Line 687
#ifndef NO_CHOWN
  D2U_ANSI_FPRINTF(stdout,_(" --no-allow-chown      don't allow file ownership change (default)\n")); // Line 689
#endif
  D2U_ANSI_FPRINTF(stdout,_(" --no-add-eol          don't add a line break to the last line if there isn't one (default)\n")); // Line 691
  D2U_ANSI_FPRINTF(stdout,_(" -O, --to-stdout       write to standard output\n")); // Line 692
  D2U_ANSI_FPRINTF(stdout,_(" -o, --oldfile         write to old file (default)\n\
   file ...            files to convert in old-file mode\n")); // Line 694
  D2U_ANSI_FPRINTF(stdout,_(" -q, --quiet           quiet mode, suppress all warnings\n")); // Line 695
  if (is_dos2unix(progname)) // Line 696
    D2U_ANSI_FPRINTF(stdout,_(" -r, --remove-bom      remove Byte Order Mark (default)\n")); // Line 697
  else // Line 698
    D2U_ANSI_FPRINTF(stdout,_(" -r, --remove-bom      remove Byte Order Mark\n")); // Line 699
  D2U_ANSI_FPRINTF(stdout,_(" -s, --safe            skip binary files (default)\n")); // Line 700
#ifdef D2U_UNICODE
  D2U_ANSI_FPRINTF(stdout,_(" -u,  --keep-utf16     keep UTF-16 encoding\n")); // Line 702
  D2U_ANSI_FPRINTF(stdout,_(" -ul, --assume-utf16le assume that the input format is UTF-16LE\n")); // Line 703
  D2U_ANSI_FPRINTF(stdout,_(" -ub, --assume-utf16be assume that the input format is UTF-16BE\n")); // Line 704
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -v,  --verbose        verbose operation\n")); // Line 706
#ifdef S_ISLNK
  D2U_ANSI_FPRINTF(stdout,_(" -F, --follow-symlink  follow symbolic links and convert the targets\n")); // Line 708
#endif
#if defined(S_ISLNK) || (defined(_WIN32) && !defined(__CYGWIN__))
  D2U_ANSI_FPRINTF(stdout,_(" -R, --replace-symlink replace symbolic links with converted files\n\
                         (original target files remain unchanged)\n")); // Line 712
  D2U_ANSI_FPRINTF(stdout,_(" -S, --skip-symlink    keep symbolic links and targets unchanged (default)\n")); // Line 713
#endif
  D2U_ANSI_FPRINTF(stdout,_(" -V, --version         display version number\n")); // Line 715
} // Line 716
