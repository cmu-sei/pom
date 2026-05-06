wint_t d2u_getwc(FILE *f, int bomtype) // Line 2778
{ // Line 2779
   int c_trail, c_lead; // Line 2780
   wint_t wc; // Line 2781

   if (((c_lead=fgetc(f)) == EOF)  || ((c_trail=fgetc(f)) == EOF)) // Line 2783
      return(WEOF); // Line 2784

   if (bomtype == FILE_UTF16LE) { /* UTF16 little endian */ // Line 2786
      c_trail <<=8; // Line 2787
      wc = (wint_t)(c_trail + c_lead) ; // Line 2788
   } else {                      /* UTF16 big endian */ // Line 2789
      c_lead <<=8; // Line 2790
      wc = (wint_t)(c_trail + c_lead) ; // Line 2791
   } // Line 2792
   return(wc); // Line 2793
} // Line 2794
