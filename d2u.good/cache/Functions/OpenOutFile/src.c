FILE* OpenOutFile(char *opFN) // Line 810
{ // Line 811
#ifdef D2U_UNIFILE
  wchar_t pathw[D2U_MAX_PATH]; // Line 813

  d2u_MultiByteToWideChar(CP_UTF8, 0, opFN, -1, pathw, D2U_MAX_PATH); // Line 815
  return _wfopen(pathw, W_CNTRLW); // Line 816
#else
  return (fopen(opFN, W_CNTRL)); // Line 818
#endif
} // Line 820
