void printInfo(CFlag *ipFlag, const char *filename, int bomtype, unsigned int lb_dos, unsigned int lb_unix, unsigned int lb_mac, int last_eol) // Line 2048
{ // Line 2049
  static int header_done = 0; // Line 2050
  char eol[6]; // Line 2051

  if (ipFlag->file_info & INFO_CONVERT) { // Line 2053
    if ((ipFlag->FromToMode == FROMTO_DOS2UNIX) && (lb_dos == 0) && (! ipFlag->add_eol || last_eol == INFO_UNIX )) // Line 2054
      return; // Line 2055
    if ((ipFlag->FromToMode == FROMTO_UNIX2DOS) && (lb_unix == 0) && (! ipFlag->add_eol || last_eol == INFO_DOS )) // Line 2056
      return; // Line 2057
    if ((ipFlag->FromToMode == FROMTO_UNIX2MAC) && (lb_unix == 0) && (! ipFlag->add_eol || last_eol == INFO_MAC )) // Line 2058
      return; // Line 2059
    if ((ipFlag->FromToMode == FROMTO_MAC2UNIX) && (lb_mac == 0) && (! ipFlag->add_eol || last_eol == INFO_UNIX )) // Line 2060
      return; // Line 2061
    if ((ipFlag->Force == 0) && (ipFlag->status & BINARY_FILE)) // Line 2062
      return; // Line 2063
  } // Line 2064

  if ((ipFlag->file_info & INFO_HEADER) && (! header_done)) { // Line 2066
    if (ipFlag->file_info & INFO_DOS) // Line 2067
      D2U_UTF8_FPRINTF(stdout, "     DOS"); // Line 2068
    if (ipFlag->file_info & INFO_UNIX) // Line 2069
      D2U_UTF8_FPRINTF(stdout, "    UNIX"); // Line 2070
    if (ipFlag->file_info & INFO_MAC) // Line 2071
      D2U_UTF8_FPRINTF(stdout, "     MAC"); // Line 2072
    if (ipFlag->file_info & INFO_BOM) // Line 2073
      D2U_UTF8_FPRINTF(stdout, "  BOM     "); // Line 2074
    if (ipFlag->file_info & INFO_TEXT) // Line 2075
      D2U_UTF8_FPRINTF(stdout, "  TXTBIN"); // Line 2076
    if ((ipFlag->add_eol && !(ipFlag->file_info & INFO_CONVERT)) || ipFlag->file_info & INFO_EOL) // Line 2077
      D2U_UTF8_FPRINTF(stdout, " LASTLN"); // Line 2078
    if (*filename != '\0') { // Line 2079
      if ((ipFlag->file_info & INFO_DEFAULT) || (ipFlag->file_info & INFO_EOL)) // Line 2080
        D2U_UTF8_FPRINTF(stdout, "  "); // Line 2081
      D2U_UTF8_FPRINTF(stdout, "FILE"); // Line 2082
    } // Line 2083
    if (ipFlag->file_info & INFO_PRINT0) // Line 2084
      (void) fputc(0, stdout); // Line 2085
    else // Line 2086
      D2U_UTF8_FPRINTF(stdout, "\n"); // Line 2087
    header_done = 1; // Line 2088
  } // Line 2089

  switch (last_eol) { // Line 2091
    case INFO_DOS: // Line 2092
      strncpy(eol,"dos  ",sizeof(eol)); // Line 2093
      break; // Line 2094
    case INFO_UNIX: // Line 2095
      strncpy(eol,"unix ",sizeof(eol)); // Line 2096
      break; // Line 2097
    case INFO_MAC: // Line 2098
      strncpy(eol,"mac  ",sizeof(eol)); // Line 2099
      break; // Line 2100
    default: // Line 2101
      strncpy(eol,"noeol",sizeof(eol)); // Line 2102
  } // Line 2103

  if (ipFlag->file_info & INFO_DOS) // Line 2105
    D2U_UTF8_FPRINTF(stdout, "  %6u", lb_dos); // Line 2106
  if (ipFlag->file_info & INFO_UNIX) // Line 2107
    D2U_UTF8_FPRINTF(stdout, "  %6u", lb_unix); // Line 2108
  if (ipFlag->file_info & INFO_MAC) // Line 2109
    D2U_UTF8_FPRINTF(stdout, "  %6u", lb_mac); // Line 2110
  if (ipFlag->file_info & INFO_BOM) // Line 2111
    print_bom_info(bomtype); // Line 2112
  if (ipFlag->file_info & INFO_TEXT) { // Line 2113
    if (ipFlag->status & BINARY_FILE) // Line 2114
      D2U_UTF8_FPRINTF(stdout, "  binary"); // Line 2115
    else // Line 2116
      D2U_UTF8_FPRINTF(stdout, "  text  "); // Line 2117
  } // Line 2118
  if ((ipFlag->add_eol && !(ipFlag->file_info & INFO_CONVERT)) || ipFlag->file_info & INFO_EOL) // Line 2119
    D2U_UTF8_FPRINTF(stdout, " %s ", eol); // Line 2120
  if (*filename != '\0') { // Line 2121
    const char *ptr; // Line 2122
    if ((ipFlag->file_info & INFO_NOPATH) && (((ptr=strrchr(filename,'/')) != NULL) || ((ptr=strrchr(filename,'\\')) != NULL)) ) // Line 2123
      ptr++; // Line 2124
    else // Line 2125
      ptr = filename; // Line 2126
    if ((ipFlag->file_info & INFO_DEFAULT) || (ipFlag->file_info & INFO_EOL)) // Line 2127
      D2U_UTF8_FPRINTF(stdout, "  "); // Line 2128
    D2U_UTF8_FPRINTF(stdout, "%s",ptr); // Line 2129
  } // Line 2130
  if (ipFlag->file_info & INFO_PRINT0) // Line 2131
    (void) fputc(0, stdout); // Line 2132
  else // Line 2133
    D2U_UTF8_FPRINTF(stdout, "\n"); // Line 2134
} // Line 2135
