int is_dos2unix(const char *progname) // Line 638
{ // Line 639
  if ((strncmp(progname, "dos2unix", sizeof("dos2unix")) == 0) || (strncmp(progname, "mac2unix", sizeof("mac2unix")) == 0)) // Line 640
    return 1; // Line 641
  else // Line 642
    return 0; // Line 643
} // Line 644
