int symbolic_link(const char *path) // Line 373
{ // Line 374
#ifdef S_ISLNK
   struct stat buf; // Line 376

   if (STAT(path, &buf) == 0) { // Line 378
      if (S_ISLNK(buf.st_mode)) // Line 379
         return(1); // Line 380
   } // Line 381
#endif
   return(0); // Line 383
} // Line 384
