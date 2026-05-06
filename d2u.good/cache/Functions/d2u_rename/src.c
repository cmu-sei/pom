int d2u_rename(const char *oldname, const char *newname) // Line 304
{ // Line 305
#ifdef D2U_UNIFILE
   wchar_t oldnamew[D2U_MAX_PATH]; // Line 307
   wchar_t newnamew[D2U_MAX_PATH]; // Line 308
   d2u_MultiByteToWideChar(CP_UTF8, 0, oldname, -1, oldnamew, D2U_MAX_PATH); // Line 309
   d2u_MultiByteToWideChar(CP_UTF8, 0, newname, -1, newnamew, D2U_MAX_PATH); // Line 310
   return _wrename(oldnamew, newnamew); // Line 311
#else
   return rename(oldname, newname); // Line 313
#endif
} // Line 315
