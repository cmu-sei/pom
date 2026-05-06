FILE* OpenInFile(char *ipFN) // Line 793
{ // Line 794
#ifdef D2U_UNIFILE
  wchar_t pathw[D2U_MAX_PATH]; // Line 796

  d2u_MultiByteToWideChar(CP_UTF8, 0, ipFN, -1, pathw, D2U_MAX_PATH); // Line 798
  return _wfopen(pathw, R_CNTRLW); // Line 799
#else
  return (fopen(ipFN, R_CNTRL)); // Line 801
#endif
} // Line 803
