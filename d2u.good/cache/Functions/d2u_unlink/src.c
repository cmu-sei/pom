int d2u_unlink(const char *filename) // Line 321
{ // Line 322
#ifdef D2U_UNIFILE
   wchar_t filenamew[D2U_MAX_PATH]; // Line 324
   d2u_MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenamew, D2U_MAX_PATH); // Line 325
   return _wunlink(filenamew); // Line 326
#else
   return unlink(filename); // Line 328
#endif
} // Line 330
