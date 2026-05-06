int ResolveSymbolicLink(char *lFN, char **rFN, CFlag *ipFlag, const char *progname) // Line 1032
{ // Line 1033
  int RetVal = 0; // Line 1034
#ifdef S_ISLNK
  struct stat StatBuf; // Line 1036
  const char *errstr; // Line 1037
  char *targetFN = NULL; // Line 1038

  if (STAT(lFN, &StatBuf)) { // Line 1040
    if (ipFlag->verbose) { // Line 1041
      ipFlag->error = errno; // Line 1042
      errstr = strerror(errno); // Line 1043
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1044
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1045
    } // Line 1046
    RetVal = -1; // Line 1047
  } // Line 1048
  else if (S_ISLNK(StatBuf.st_mode)) { // Line 1049
#if USE_CANONICALIZE_FILE_NAME
    targetFN = canonicalize_file_name(lFN); // Line 1051
    if (!targetFN) { // Line 1052
      if (ipFlag->verbose) { // Line 1053
        ipFlag->error = errno; // Line 1054
        errstr = strerror(errno); // Line 1055
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1056
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1057
      } // Line 1058
      RetVal = -1; // Line 1059
    } // Line 1060
    else { // Line 1061
      *rFN = targetFN; // Line 1062
      RetVal = 1; // Line 1063
    } // Line 1064
#else
    /* Sigh. Use realpath, but realize that it has a fatal // Line 1066
     * flaw: PATH_MAX isn't necessarily the maximum path // Line 1067
     * length -- so realpath() might fail. */ // Line 1068
    targetFN = (char *) malloc(PATH_MAX * sizeof(char)); // Line 1069
    if (!targetFN) { // Line 1070
      if (ipFlag->verbose) { // Line 1071
        ipFlag->error = errno; // Line 1072
        errstr = strerror(errno); // Line 1073
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1074
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1075
      } // Line 1076
      RetVal = -1; // Line 1077
    } // Line 1078
    else { // Line 1079
      /* is there any platform with S_ISLNK that does not have realpath? */ // Line 1080
      char *rVal = realpath(lFN, targetFN); // Line 1081
      if (!rVal) { // Line 1082
        if (ipFlag->verbose) { // Line 1083
          ipFlag->error = errno; // Line 1084
          errstr = strerror(errno); // Line 1085
          D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, lFN); // Line 1086
          D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1087
        } // Line 1088
        free(targetFN); // Line 1089
        RetVal = -1; // Line 1090
      } // Line 1091
      else { // Line 1092
        *rFN = rVal; // Line 1093
        RetVal = 1; // Line 1094
      } // Line 1095
    } // Line 1096
#endif /* !USE_CANONICALIZE_FILE_NAME */
  } // Line 1098
  else // Line 1099
    *rFN = lFN; // Line 1100
#else  /* !S_ISLNK */
  *rFN = lFN; // Line 1102
#endif /* !S_ISLNK */
  return RetVal; // Line 1104
} // Line 1105
