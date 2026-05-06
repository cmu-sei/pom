wint_t d2u_putwc(wint_t wc, FILE *f, CFlag *ipFlag, const char *progname) // Line 2817
{ // Line 2818
   static char mbs[8]; // Line 2819
   static wchar_t lead=0x01;  /* lead get's invalid value */ // Line 2820
   static wchar_t wstr[3]; // Line 2821
   size_t len; // Line 2822
#if (defined(_WIN32) && !defined(__CYGWIN__))
   DWORD dwFlags; // Line 2824
#endif

   if (ipFlag->keep_utf16) { // Line 2827
     int c_trail, c_lead; // Line 2828
     if (ipFlag->bomtype == FILE_UTF16LE) { /* UTF16 little endian */ // Line 2829
        c_trail = (int)(wc & 0xff00); // Line 2830
        c_trail >>=8; // Line 2831
        c_lead  = (int)(wc & 0xff); // Line 2832
     } else {                      /* UTF16 big endian */ // Line 2833
        c_lead = (int)(wc & 0xff00); // Line 2834
        c_lead >>=8; // Line 2835
        c_trail  = (int)(wc & 0xff); // Line 2836
     } // Line 2837
     if ((fputc(c_lead,f) == EOF)  || (fputc(c_trail,f) == EOF)) // Line 2838
       return(WEOF); // Line 2839
     return wc; // Line 2840
   } // Line 2841

   /* Note: In the new Unicode standard lead is named "high", and trail is name "low". */ // Line 2843

   /* check for lead without a trail */ // Line 2845
   if ((lead >= 0xd800) && (lead < 0xdc00) && ((wc < 0xdc00) || (wc >= 0xe000))) { // Line 2846
      D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2847
      D2U_UTF8_FPRINTF(stderr, _("error: Invalid surrogate pair. Missing low surrogate.\n")); // Line 2848
      ipFlag->status |= UNICODE_CONVERSION_ERROR ; // Line 2849
      return(WEOF); // Line 2850
   } // Line 2851

   if ((wc >= 0xd800) && (wc < 0xdc00)) {   /* Surrogate lead */ // Line 2853
      /* fprintf(stderr, "UTF-16 lead %x\n",wc); */ // Line 2854
      lead = (wchar_t)wc; /* lead (high) surrogate */ // Line 2855
      return(wc); // Line 2856
   } // Line 2857
   if ((wc >= 0xdc00) && (wc < 0xe000)) {   /* Surrogate trail */ // Line 2858
      static wchar_t trail; // Line 2859

      /* check for trail without a lead */ // Line 2861
      if ((lead < 0xd800) || (lead >= 0xdc00)) { // Line 2862
         D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 2863
         D2U_UTF8_FPRINTF(stderr, _("error: Invalid surrogate pair. Missing high surrogate.\n")); // Line 2864
         ipFlag->status |= UNICODE_CONVERSION_ERROR ; // Line 2865
         return(WEOF); // Line 2866
      } // Line 2867
      /* fprintf(stderr, "UTF-16 trail %x\n",wc); */ // Line 2868
      trail = (wchar_t)wc; /* trail (low) surrogate */ // Line 2869
#if defined(_WIN32) || defined(__CYGWIN__)
      /* On Windows (including Cygwin) wchar_t is 16 bit */ // Line 2871
      /* We cannot decode an UTF-16 surrogate pair, because it will // Line 2872
         not fit in a 16 bit wchar_t. */ // Line 2873
      wstr[0] = lead; // Line 2874
      wstr[1] = trail; // Line 2875
      wstr[2] = L'\0'; // Line 2876
      lead = 0x01; /* make lead invalid */ // Line 2877
#else
      /* On Unix wchar_t is 32 bit */ // Line 2879
      /* When we don't decode the UTF-16 surrogate pair, wcstombs() does not // Line 2880
       * produce the same UTF-8 as WideCharToMultiByte().  The UTF-8 output // Line 2881
       * produced by wcstombs() is bigger, because it just translates the wide // Line 2882
       * characters in the range 0xD800..0xDBFF individually to UTF-8 sequences // Line 2883
       * (although these code points are reserved for use only as surrogate // Line 2884
       * pairs in UTF-16). // Line 2885
       * // Line 2886
       * Some smart viewers can still display this UTF-8 correctly (like Total // Line 2887
       * Commander lister), however the UTF-8 is not readable by Windows // Line 2888
       * Notepad (on Windows 7).  When we decode the UTF-16 surrogate pairs // Line 2889
       * ourselves the wcstombs() UTF-8 output is identical to what // Line 2890
       * WideCharToMultiByte() produces, and is readable by Notepad. // Line 2891
       * // Line 2892
       * Surrogate halves in UTF-8 are invalid. See also // Line 2893
       * https://en.wikipedia.org/wiki/UTF-8#Invalid_code_points // Line 2894
       * https://tools.ietf.org/html/rfc3629#page-5 // Line 2895
       * It is a bug in (some implementations of) wcstombs(). // Line 2896
       * On Cygwin 1.7 wcstombs() produces correct UTF-8 from UTF-16 surrogate pairs. // Line 2897
       */ // Line 2898
      /* Decode UTF-16 surrogate pair */ // Line 2899
      wstr[0] = 0x10000; // Line 2900
      wstr[0] += (lead & 0x03FF) << 10; // Line 2901
      wstr[0] += (trail & 0x03FF); // Line 2902
      wstr[1] = L'\0'; // Line 2903
      lead = 0x01; /* make lead invalid */ // Line 2904
      /* fprintf(stderr, "UTF-32  %x\n",wstr[0]); */ // Line 2905
#endif
   } else { // Line 2907
      wstr[0] = (wchar_t)wc; // Line 2908
      wstr[1] = L'\0'; // Line 2909
   } // Line 2910

   if (wc == 0x0000) { // Line 2912
      if (fputc(0, f) == EOF) // Line 2913
         return(WEOF); // Line 2914
      return(wc); // Line 2915
   } // Line 2916

#if (defined(_WIN32) && !defined(__CYGWIN__))
/* The WC_ERR_INVALID_CHARS flag is available since Windows Vista (0x0600). It enables checking for // Line 2919
   invalid input characters. */ // Line 2920
#if WINVER >= 0x0600
   dwFlags = WC_ERR_INVALID_CHARS; // Line 2922
#else
   dwFlags = 0; // Line 2924
#endif
   /* On Windows we convert UTF-16 always to UTF-8 or GB18030 */ // Line 2926
   if (ipFlag->locale_target == TARGET_GB18030) { // Line 2927
     len = (size_t)(WideCharToMultiByte(54936, dwFlags, wstr, -1, mbs, sizeof(mbs), NULL, NULL) -1); // Line 2928
   } else { // Line 2929
     len = (size_t)(WideCharToMultiByte(CP_UTF8, dwFlags, wstr, -1, mbs, sizeof(mbs), NULL, NULL) -1); // Line 2930
   } // Line 2931
#else
   /* On Unix we convert UTF-16 to the locale encoding */ // Line 2933
   len = wcstombs(mbs, wstr, sizeof(mbs)); // Line 2934
   /* fprintf(stderr, "len  %d\n",len); */ // Line 2935
#endif

   if ( len == (size_t)(-1) ) { // Line 2938
      /* Stop when there is a conversion error */ // Line 2939
   /* On Windows we convert UTF-16 always to UTF-8 or GB18030 */ // Line 2940
      if (ipFlag->verbose) { // Line 2941
#if (defined(_WIN32) && !defined(__CYGWIN__))
        d2u_PrintLastError(progname); // Line 2943
#else
        const char *errstr = strerror(errno); // Line 2945
        D2U_UTF8_FPRINTF(stderr, "%s:", progname); // Line 2946
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 2947
#endif
      } // Line 2949
      ipFlag->status |= UNICODE_CONVERSION_ERROR ; // Line 2950
      return(WEOF); // Line 2951
   } else { // Line 2952
      size_t i; // Line 2953
      for (i=0; i<len; i++) { // Line 2954
         if (fputc(mbs[i], f) == EOF) // Line 2955
            return(WEOF); // Line 2956
      } // Line 2957
   } // Line 2958
   return(wc); // Line 2959
} // Line 2960
