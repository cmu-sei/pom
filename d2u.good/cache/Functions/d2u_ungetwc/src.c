wint_t d2u_ungetwc(wint_t wc, FILE *f, int bomtype) // Line 2796
{ // Line 2797
   int c_trail, c_lead; // Line 2798

   if (bomtype == FILE_UTF16LE) { /* UTF16 little endian */ // Line 2800
      c_trail = (int)(wc & 0xff00); // Line 2801
      c_trail >>=8; // Line 2802
      c_lead  = (int)(wc & 0xff); // Line 2803
   } else {                      /* UTF16 big endian */ // Line 2804
      c_lead = (int)(wc & 0xff00); // Line 2805
      c_lead >>=8; // Line 2806
      c_trail  = (int)(wc & 0xff); // Line 2807
   } // Line 2808

   /* push back in reverse order */ // Line 2810
   if ((ungetc(c_trail,f) == EOF)  || (ungetc(c_lead,f) == EOF)) // Line 2811
      return(WEOF); // Line 2812
   return(wc); // Line 2813
} // Line 2814
