char *d2u_strncpy(char *dest, const char *src, size_t dest_size) // Line 62
{ // Line 63
    strncpy(dest,src,dest_size); // Line 64
    dest[dest_size-1] = '\0'; // Line 65
#ifdef DEBUG
    if(strlen(src) > (dest_size-1)) { // Line 67
        D2U_UTF8_FPRINTF(stderr, "Text %s has been truncated from %d to %d characters in %s to prevent a buffer overflow.\n", src, (int)strlen(src), (int)dest_size, "d2u_strncpy()"); // Line 68
    } // Line 69
#endif
    return dest; // Line 71
} // Line 72
