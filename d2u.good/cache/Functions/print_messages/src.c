void print_messages(const CFlag *pFlag, const char *infile, const char *outfile, const char *progname, const int conversion_error) // Line 1907
{ // Line 1908
  char informat[32]; // Line 1909
  char outformat[64]; // Line 1910
# ifdef D2U_UNIFILE
  wchar_t informatw[32]; // Line 1912
  wchar_t outformatw[64]; // Line 1913
#endif

  print_format(pFlag, informat, outformat, sizeof(informat), sizeof(outformat)); // Line 1916

/* Change informat and outformat to UTF-8 for d2u_utf8_fprintf. */ // Line 1918
# ifdef D2U_UNIFILE
   /* The format string is encoded in the system default // Line 1920
    * Windows ANSI code page. May have been translated // Line 1921
    * by gettext. Convert it to wide characters. */ // Line 1922
   d2u_MultiByteToWideChar(CP_ACP,0, informat, -1, informatw, sizeof(informat)); // Line 1923
   d2u_MultiByteToWideChar(CP_ACP,0, outformat, -1, outformatw, sizeof(outformat)); // Line 1924
   /* then convert the format string to UTF-8 */ // Line 1925
   d2u_WideCharToMultiByte(CP_UTF8, 0, informatw, -1, informat, sizeof(informat), NULL, NULL); // Line 1926
   d2u_WideCharToMultiByte(CP_UTF8, 0, outformatw, -1, outformat, sizeof(outformat), NULL, NULL); // Line 1927
#endif

  if (pFlag->status & NO_REGFILE) { // Line 1930
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1931
    D2U_UTF8_FPRINTF(stderr, _("Skipping %s, not a regular file.\n"), infile); // Line 1932
  } else if (pFlag->status & OUTPUTFILE_SYMLINK) { // Line 1933
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1934
    if (outfile) // Line 1935
      D2U_UTF8_FPRINTF(stderr, _("Skipping %s, output file %s is a symbolic link.\n"), infile, outfile); // Line 1936
    else // Line 1937
      D2U_UTF8_FPRINTF(stderr, _("Skipping symbolic link %s.\n"), infile); // Line 1938
  } else if (pFlag->status & INPUT_TARGET_NO_REGFILE) { // Line 1939
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1940
    D2U_UTF8_FPRINTF(stderr, _("Skipping symbolic link %s, target is not a regular file.\n"), infile); // Line 1941
  } else if ((pFlag->status & OUTPUT_TARGET_NO_REGFILE) && outfile) { // Line 1942
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1943
    D2U_UTF8_FPRINTF(stderr, _("Skipping %s, target of symbolic link %s is not a regular file.\n"), infile, outfile); // Line 1944
  } else if (pFlag->status & BINARY_FILE) { // Line 1945
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1946
    D2U_UTF8_FPRINTF(stderr, _("Skipping binary file %s\n"), infile); // Line 1947
  } else if (pFlag->status & WRONG_CODEPAGE) { // Line 1948
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1949
    D2U_UTF8_FPRINTF(stderr, _("code page %d is not supported.\n"), pFlag->ConvMode); // Line 1950
#ifdef D2U_UNICODE
  } else if (pFlag->status & WCHAR_T_TOO_SMALL) { // Line 1952
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1953
    D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, the size of wchar_t is %d bytes.\n"), infile, (int)sizeof(wchar_t)); // Line 1954
  } else if (pFlag->status & UNICODE_CONVERSION_ERROR) { // Line 1955
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1956
    D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, an UTF-16 conversion error occurred on line %u.\n"), infile, pFlag->line_nr); // Line 1957
#else
  } else if (pFlag->status & UNICODE_NOT_SUPPORTED) { // Line 1959
    D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1960
    D2U_UTF8_FPRINTF(stderr, _("Skipping UTF-16 file %s, UTF-16 conversion is not supported in this version of %s.\n"), infile, progname); // Line 1961
#endif
  } else { // Line 1963
    if (!conversion_error) { // Line 1964
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 1965
      if (informat[0] == '\0') { // Line 1966
        if (is_dos2unix(progname)) { // Line 1967
          if (outfile) // Line 1968
            D2U_UTF8_FPRINTF(stderr, _("converting file %s to file %s in Unix format...\n"), infile, outfile); // Line 1969
          else // Line 1970
            D2U_UTF8_FPRINTF(stderr, _("converting file %s to Unix format...\n"), infile); // Line 1971
        } else { // Line 1972
          if (pFlag->FromToMode == FROMTO_UNIX2MAC) { // Line 1973
            if (outfile) // Line 1974
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to file %s in Mac format...\n"), infile, outfile); // Line 1975
            else // Line 1976
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to Mac format...\n"), infile); // Line 1977
          } else { // Line 1978
            if (outfile) // Line 1979
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to file %s in DOS format...\n"), infile, outfile); // Line 1980
            else // Line 1981
              D2U_UTF8_FPRINTF(stderr, _("converting file %s to DOS format...\n"), infile); // Line 1982
          } // Line 1983
        } // Line 1984
      } else { // Line 1985
        if (is_dos2unix(progname)) { // Line 1986
          if (outfile) // Line 1987
    /* TRANSLATORS: // Line 1988
1st %s is encoding of input file. // Line 1989
2nd %s is name of input file. // Line 1990
3rd %s is encoding of output file. // Line 1991
4th %s is name of output file. // Line 1992
E.g.: converting UTF-16LE file in.txt to UTF-8 file out.txt in Unix format... */ // Line 1993
            D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s file %s in Unix format...\n"), informat, infile, outformat, outfile); // Line 1994
          else // Line 1995
    /* TRANSLATORS: // Line 1996
1st %s is encoding of input file. // Line 1997
2nd %s is name of input file. // Line 1998
3rd %s is encoding of output (input file is overwritten). // Line 1999
E.g.: converting UTF-16LE file foo.txt to UTF-8 Unix format... */ // Line 2000
            D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s Unix format...\n"), informat, infile, outformat); // Line 2001
        } else { // Line 2002
          if (pFlag->FromToMode == FROMTO_UNIX2MAC) { // Line 2003
            if (outfile) // Line 2004
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s file %s in Mac format...\n"), informat, infile, outformat, outfile); // Line 2005
            else // Line 2006
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s Mac format...\n"), informat, infile, outformat); // Line 2007
          } else { // Line 2008
            if (outfile) // Line 2009
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s file %s in DOS format...\n"), informat, infile, outformat, outfile); // Line 2010
            else // Line 2011
              D2U_UTF8_FPRINTF(stderr, _("converting %s file %s to %s DOS format...\n"), informat, infile, outformat); // Line 2012
          } // Line 2013
        } // Line 2014
      } // Line 2015
    } else { // Line 2016
      D2U_UTF8_FPRINTF(stderr,"%s: ",progname); // Line 2017
      if (outfile) // Line 2018
        D2U_UTF8_FPRINTF(stderr, _("problems converting file %s to file %s\n"), infile, outfile); // Line 2019
      else // Line 2020
        D2U_UTF8_FPRINTF(stderr, _("problems converting file %s\n"), infile); // Line 2021
    } // Line 2022
  } // Line 2023
} // Line 2024
