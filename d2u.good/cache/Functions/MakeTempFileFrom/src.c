FILE* MakeTempFileFrom(const char *OutFN, char **fname_ret) // Line 963
{ // Line 964
  char *cpy = strdup(OutFN); // Line 965
  char *dir = NULL; // Line 966
  size_t fname_len = 0; // Line 967
  char  *fname_str = NULL; // Line 968
  FILE *fp = NULL;  /* file pointer */ // Line 969
#ifdef NO_MKSTEMP
  char *name; // Line 971
#else
  int fd = -1;  /* file descriptor */ // Line 973
#endif

  *fname_ret = NULL; // Line 976

  if (!cpy) // Line 978
    goto make_failed; // Line 979

  dir = dirname(cpy); // Line 981

  fname_len = strlen(dir) + strlen("/d2utmpXXXXXX") + sizeof (char); // Line 983
  if (!(fname_str = (char *)malloc(fname_len))) // Line 984
    goto make_failed; // Line 985
  sprintf(fname_str, "%s%s", dir, "/d2utmpXXXXXX"); // Line 986
  *fname_ret = fname_str; // Line 987

  free(cpy); // Line 989
  cpy = NULL; // Line 990

#ifdef NO_MKSTEMP
  if ((name = d2u_mktemp(fname_str)) == NULL) // Line 993
    goto make_failed; // Line 994
  *fname_ret = name; // Line 995
  if ((fp = OpenOutFile(name)) == NULL) // Line 996
    goto make_failed; // Line 997
#else
  if ((fd = mkstemp(fname_str)) == -1) // Line 999
    goto make_failed; // Line 1000

  if ((fp=OpenOutFiled(fd)) == NULL) // Line 1002
    goto make_failed; // Line 1003
#endif

  return (fp); // Line 1006

  make_failed: // Line 1008
    if (cpy) { // Line 1009
       free(cpy); // Line 1010
       cpy = NULL; // Line 1011
    } // Line 1012
    free(*fname_ret); // Line 1013
    *fname_ret = NULL; // Line 1014
    return NULL; // Line 1015
} // Line 1016
